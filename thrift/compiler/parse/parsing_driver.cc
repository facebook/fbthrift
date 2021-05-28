/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/parse/parsing_driver.h>

#include <cstdarg>
#include <memory>

#include <boost/filesystem.hpp>

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

namespace apache {
namespace thrift {
namespace compiler {

namespace {

class parsing_terminator : public std::runtime_error {
 public:
  parsing_terminator()
      : std::runtime_error(
            "Internal exception used to terminate the parsing process.") {}
};

} // namespace

parsing_driver::parsing_driver(
    diagnostic_context& ctx, std::string path, parsing_params parse_params)
    : params(std::move(parse_params)),
      doctext(boost::none),
      doctext_lineno(0),
      mode(parsing_mode::INCLUDES),
      ctx_(ctx) {
  auto root_program = std::make_unique<t_program>(path);
  ctx_.start_program(program = root_program.get());
  program_bundle = std::make_unique<t_program_bundle>(std::move(root_program));

  scope_cache = program->scope();
}

/**
 * The default destructor needs to be explicitly defined in the .cc file since
 * it invokes the destructor of parse_ (of type unique_ptr<yy::parser>). It
 * cannot go in the header file since yy::parser is only forward-declared there.
 */
parsing_driver::~parsing_driver() = default;

std::unique_ptr<t_program_bundle> parsing_driver::parse() {
  std::unique_ptr<t_program_bundle> result{};
  try {
    scanner = std::make_unique<yy_scanner>();
  } catch (std::system_error const&) {
    return result;
  }

  parser_ = std::make_unique<yy::parser>(*this, scanner->get_scanner());

  try {
    parse_file();
    result = std::move(program_bundle);
  } catch (const parsing_terminator&) {
    // No need to do anything here. The purpose of the exception is simply to
    // end the parsing process by unwinding to here.
  }
  return result;
}

void parsing_driver::parse_file() {
  // Get scope file path
  const std::string& path = program->path();

  // Skip on already parsed files
  if (already_parsed_paths_.count(path)) {
    return;
  } else {
    already_parsed_paths_.insert(path);
  }

  try {
    scanner->start(path);
    scanner->set_lineno(1);
  } catch (std::runtime_error const& ex) {
    failure(ex.what());
  }

  // Create new scope and scan for includes
  verbose("Scanning %s for includes\n", path.c_str());
  mode = parsing_mode::INCLUDES;
  try {
    if (parser_->parse() != 0) {
      failure("Parser error during include pass.");
    }
  } catch (const std::string& x) {
    failure(x.c_str());
  }

  // Recursively parse all the include programs
  const auto& includes = program->get_included_programs();
  // Always enable allow_neg_field_keys when parsing included files.
  // This way if a thrift file has negative keys, --allow-neg-keys doesn't have
  // to be used by everyone that includes it.
  auto old_params = params;
  auto old_program = program;
  for (auto included_program : includes) {
    circular_deps_.insert(path);

    // Fail on circular dependencies
    if (circular_deps_.count(included_program->path())) {
      failure(
          "Circular dependency found: file `%s` is already parsed.",
          included_program->path().c_str());
    }

    // This must be after the previous circular include check, since the emitted
    // error message above is supposed to reference the parent file name.
    params.allow_neg_enum_vals = true;
    params.allow_neg_field_keys = true;
    ctx_.start_program(program = included_program);
    parse_file();
    ctx_.end_program(program);

    size_t num_removed = circular_deps_.erase(path);
    (void)num_removed;
    assert(num_removed == 1);
  }
  params = old_params;
  program = old_program;

  // Parse the program file
  try {
    scanner->start(path);
    scanner->set_lineno(1);
  } catch (std::runtime_error const& ex) {
    failure(ex.what());
  }

  mode = parsing_mode::PROGRAM;
  verbose("Parsing %s for types\n", path.c_str());
  try {
    if (parser_->parse() != 0) {
      failure("Parser error during types pass.");
    }
  } catch (const std::string& x) {
    failure(x.c_str());
  }

  for (auto td : program->placeholder_typedefs()) {
    if (!td->resolve()) {
      failure("Type `%s` not defined.", td->name().c_str());
    }
  }
}

[[noreturn]] void parsing_driver::end_parsing() {
  throw parsing_terminator{};
}

// TODO: This doesn't really need to be a member function. Move it somewhere
// else (e.g. `util.{h|cc}`) once everything gets consolidated into `parse/`.
/* static */ std::string parsing_driver::directory_name(
    const std::string& filename) {
  std::string::size_type slash = filename.rfind('/');
  // No slash, just use the current directory
  if (slash == std::string::npos) {
    return ".";
  }
  return filename.substr(0, slash);
}

std::string parsing_driver::include_file(const std::string& filename) {
  // Absolute path? Just try that
  boost::filesystem::path path{filename};
  if (path.has_root_directory()) {
    try {
      auto abspath = boost::filesystem::canonical(path);
      return abspath.string();
    } catch (const boost::filesystem::filesystem_error& e) {
      failure("Could not find file: %s. Error: %s", filename.c_str(), e.what());
    }
  } else { // relative path, start searching
    // new search path with current dir global
    std::vector<std::string> sp = params.incl_searchpath;
    sp.insert(sp.begin(), directory_name(program->path()));

    // iterate through paths
    std::vector<std::string>::iterator it;
    for (it = sp.begin(); it != sp.end(); it++) {
      std::string sfilename = filename;
      if ((*it) != "." && (*it) != "") {
        sfilename = *(it) + "/" + filename;
      }
      if (boost::filesystem::exists(sfilename)) {
        return sfilename;
      } else {
        debug("Could not find: %s.", sfilename.c_str());
      }
    }

    // File was not found
    failure("Could not find include file %s", filename.c_str());
  }
}

void parsing_driver::validate_const_rec(
    std::string name, const t_type* type, t_const_value* value) {
  if (type->is_void()) {
    throw std::string("type error: cannot declare a void const: " + name);
  }

  auto as_struct = dynamic_cast<const t_struct*>(type);
  assert((as_struct != nullptr) == type->is_struct());
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        if (value->get_type() != t_const_value::CV_STRING) {
          throw std::string(
              "type error: const `" + name + "` was declared as string.");
        }
        break;
      case t_base_type::TYPE_BOOL:
        if (value->get_type() != t_const_value::CV_BOOL &&
            value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const `" + name + "` was declared as bool.");
        }
        break;
      case t_base_type::TYPE_BYTE:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const `" + name + "` was declared as byte.");
        }
        break;
      case t_base_type::TYPE_I16:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const `" + name + "` was declared as i16.");
        }
        break;
      case t_base_type::TYPE_I32:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const `" + name + "` was declared as i32.");
        }
        break;
      case t_base_type::TYPE_I64:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const `" + name + "` was declared as i64.");
        }
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        if (value->get_type() != t_const_value::CV_INTEGER &&
            value->get_type() != t_const_value::CV_DOUBLE) {
          throw std::string(
              "type error: const `" + name + "` was declared as double.");
        }
        break;
      default:
        throw std::string(
            "compiler error: no const of base type " +
            t_base_type::t_base_name(tbase) + name);
    }
  } else if (type->is_enum()) {
    if (value->get_type() != t_const_value::CV_INTEGER) {
      throw std::string(
          "type error: const `" + name + "` was declared as enum.");
    }
    const auto as_enum = dynamic_cast<const t_enum*>(type);
    assert(as_enum != nullptr);
    const auto enum_val = as_enum->find_value(value->get_integer());
    if (enum_val == nullptr) {
      warning(
          "type error: const `%s` was declared as enum `%s` with a value"
          " not of that enum.",
          name.c_str(),
          type->get_name().c_str());
    }
  } else if (as_struct && as_struct->is_union()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw std::string(
          "type error: const `" + name + "` was declared as union.");
    }
    auto const& map = value->get_map();
    if (map.size() > 1) {
      throw std::string(
          "type error: const `" + name +
          "` is a union and can't "
          "have more than one field set.");
    }
    if (!map.empty()) {
      if (map.front().first->get_type() != t_const_value::CV_STRING) {
        throw std::string(
            "type error: const `" + name +
            "` is a union and member "
            "names must be a string.");
      }
      auto const& member_name = map.front().first->get_string();
      auto const* member = as_struct->get_field_by_name(member_name);
      if (!member) {
        throw std::string(
            "type error: no member named `" + member_name +
            "` for "
            "union const `" +
            name + "`.");
      }
    }
  } else if (type->is_struct() || type->is_xception()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw std::string(
          "type error: const `" + name + "` was declared as " +
          "struct/exception.");
    }
    const auto as_struct = static_cast<const t_struct*>(type);
    for (const auto& entry : value->get_map()) {
      if (entry.first->get_type() != t_const_value::CV_STRING) {
        throw std::string(
            "type error: `" + name + "` struct key must be string.");
      }
      const t_type* field_type = nullptr;
      if (const auto* field =
              as_struct->get_field_by_name(entry.first->get_string())) {
        field_type = field->get_type();
      }
      if (field_type == nullptr) {
        throw std::string(
            "type error: `" + type->get_name() + "` has no field `" +
            entry.first->get_string() + "`.");
      }

      validate_const_rec(
          name + "." + entry.first->get_string(), field_type, entry.second);
    }
  } else if (type->is_map()) {
    const t_type* k_type = ((t_map*)type)->get_key_type();
    const t_type* v_type = ((t_map*)type)->get_val_type();
    const std::vector<std::pair<t_const_value*, t_const_value*>>& val =
        value->get_map();
    std::vector<std::pair<t_const_value*, t_const_value*>>::const_iterator
        v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      validate_const_rec(name + "<key>", k_type, v_iter->first);
      validate_const_rec(name + "<val>", v_type, v_iter->second);
    }
  } else if (type->is_list() || type->is_set()) {
    const t_type* e_type;
    if (type->is_list()) {
      e_type = ((t_list*)type)->get_elem_type();
    } else {
      e_type = ((t_set*)type)->get_elem_type();
    }
    const std::vector<t_const_value*>& val = value->get_list();
    std::vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      validate_const_rec(name + "<elem>", e_type, *v_iter);
    }
  }
}

void parsing_driver::validate_const_type(t_const* c) {
  validate_const_rec(c->get_name(), c->get_type(), c->get_value());
}

void parsing_driver::validate_field_value(t_field* field, t_const_value* cv) {
  validate_const_rec(field->get_name(), field->get_type(), cv);
}

void parsing_driver::validate_not_ambiguous_enum(const std::string& name) {
  if (scope_cache->is_ambiguous_enum_value(name)) {
    std::string possible_enums =
        scope_cache->get_fully_qualified_enum_value_names(name).c_str();
    std::string msg = "The ambiguous enum `" + name +
        "` is defined in more than one place. " +
        "Please refer to this enum using ENUM_NAME.ENUM_VALUE.";
    if (!possible_enums.empty()) {
      msg += (" Possible options: " + possible_enums);
    }
    warning(msg.c_str());
  }
}
void parsing_driver::clear_doctext() {
  if (doctext) {
    warning_strict("Uncaptured doctext at on line %d.", doctext_lineno);
  }

  doctext = boost::none;
}

t_doc parsing_driver::clean_up_doctext(std::string docstring) {
  // Convert to C++ string, and remove Windows's carriage returns.
  docstring.erase(
      remove(docstring.begin(), docstring.end(), '\r'), docstring.end());

  // Separate into lines.
  std::vector<std::string> lines;
  std::string::size_type pos = std::string::npos;
  std::string::size_type last;
  while (true) {
    last = (pos == std::string::npos) ? 0 : pos + 1;
    pos = docstring.find('\n', last);
    if (pos == std::string::npos) {
      // First bit of cleaning.  If the last line is only whitespace, drop it.
      std::string::size_type nonwhite =
          docstring.find_first_not_of(" \t", last);
      if (nonwhite != std::string::npos) {
        lines.push_back(docstring.substr(last));
      }
      break;
    }
    lines.push_back(docstring.substr(last, pos - last));
  }

  // A very profound docstring.
  if (lines.empty()) {
    return boost::none;
  }

  // Clear leading whitespace from the first line.
  pos = lines.front().find_first_not_of(" \t");
  lines.front().erase(0, pos);

  // If every nonblank line after the first has the same number of spaces/tabs,
  // then a comment prefix, remove them.
  enum Prefix {
    None = 0,
    Star = 1, // length of '*'
    Slashes = 3, // length of '///'
  };
  Prefix found_prefix = None;
  std::string::size_type prefix_len = 0;
  std::vector<std::string>::iterator l_iter;

  // Start searching for prefixes from second line, since lexer already removed
  // initial prefix/suffix.
  for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }

    pos = l_iter->find_first_not_of(" \t");
    if (found_prefix == None) {
      if (pos != std::string::npos) {
        if (l_iter->at(pos) == '*') {
          found_prefix = Star;
          prefix_len = pos;
        } else if (l_iter->compare(pos, 3, "///") == 0) {
          found_prefix = Slashes;
          prefix_len = pos;
        } else {
          found_prefix = None;
          break;
        }
      } else {
        // Whitespace-only line.  Truncate it.
        l_iter->clear();
      }
    } else if (
        l_iter->size() > pos && pos == prefix_len &&
        ((found_prefix == Star && l_iter->at(pos) == '*') ||
         (found_prefix == Slashes && l_iter->compare(pos, 3, "///") == 0))) {
      // Business as usual
    } else if (pos == std::string::npos) {
      // Whitespace-only line.  Let's truncate it for them.
      l_iter->clear();
    } else {
      // The pattern has been broken.
      found_prefix = None;
      break;
    }
  }

  // If our prefix survived, delete it from every line.
  if (found_prefix != None) {
    // Get the prefix too.
    prefix_len += found_prefix;
    for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Now delete the minimum amount of leading whitespace from each line.
  prefix_len = std::string::npos;
  for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }
    pos = l_iter->find_first_not_of(" \t");
    if (pos != std::string::npos &&
        (prefix_len == std::string::npos || pos < prefix_len)) {
      prefix_len = pos;
    }
  }

  // If our whitespace prefix survived, delete it from every line.
  if (prefix_len != std::string::npos) {
    for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Remove trailing whitespace from every line.
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    pos = l_iter->find_last_not_of(" \t");
    if (pos != std::string::npos && pos != l_iter->length() - 1) {
      l_iter->erase(pos + 1);
    }
  }

  // If the first line is empty, remove it.
  // Don't do this earlier because a lot of steps skip the first line.
  if (lines.front().empty()) {
    lines.erase(lines.begin());
  }

  // Now rejoin the lines and copy them back into doctext.
  docstring.clear();
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    docstring += *l_iter;
    docstring += '\n';
  }

  return docstring;
}

bool parsing_driver::require_experimental_feature(const char* feature) {
  assert(feature != std::string("all"));
  if (params.allow_experimental_features.count("all") ||
      params.allow_experimental_features.count(feature)) {
    warning_strict("'%s' is an experimental feature.", feature);
    return true;
  }
  failure("'%s' is an experimental feature.", feature);
  return false;
}

void parsing_driver::set_annotations(
    t_node* node, std::unique_ptr<t_annotations> annotations) {
  if (annotations != nullptr) {
    node->reset_annotations(annotations->strings, annotations->last_lineno);
  }
}

void parsing_driver::set_attributes(
    t_named* node,
    std::unique_ptr<t_def_attrs> attrs,
    std::unique_ptr<t_annotations> annotations) {
  if (attrs != nullptr) {
    if (attrs->doc) {
      node->set_doc(std::move(*attrs->doc));
    }
    if (attrs->struct_annotations != nullptr) {
      for (auto& an : *attrs->struct_annotations) {
        node->add_structured_annotation(std::move(an));
      }
    }
  }
  set_annotations(node, std::move(annotations));
}

void parsing_driver::start_node(LineType lineType) {
  lineno_stack_.emplace(lineType, scanner->get_lineno());
}

int parsing_driver::pop_node(LineType lineType) {
  if (lineType != lineno_stack_.top().first) {
    throw std::logic_error("Popping wrong type from line number stack");
  }
  int lineno = lineno_stack_.top().second;
  lineno_stack_.pop();
  return lineno;
}

void parsing_driver::finish_node(
    t_node* node,
    LineType lineType,
    std::unique_ptr<t_annotations> annotations) {
  node->set_lineno(pop_node(lineType));
  set_annotations(node, std::move(annotations));
}

void parsing_driver::finish_node(
    t_named* node,
    LineType lineType,
    std::unique_ptr<t_def_attrs> attrs,
    std::unique_ptr<t_annotations> annotations) {
  node->set_lineno(pop_node(lineType));
  set_attributes(node, std::move(attrs), std::move(annotations));
}

void parsing_driver::finish_node(
    t_structured* node,
    LineType lineType,
    std::unique_ptr<t_def_attrs> attrs,
    std::unique_ptr<t_field_list> fields,
    std::unique_ptr<t_annotations> annotations) {
  append_fields(*node, std::move(*fields));
  finish_node(node, lineType, std::move(attrs), std::move(annotations));
}

void parsing_driver::finish_node(
    t_interface* node,
    LineType lineType,
    std::unique_ptr<t_def_attrs> attrs,
    std::unique_ptr<t_function_list> functions,
    std::unique_ptr<t_annotations> annotations) {
  if (functions != nullptr) {
    node->set_functions(std::move(*functions));
  }
  finish_node(node, lineType, std::move(attrs), std::move(annotations));
}

std::unique_ptr<t_const> parsing_driver::new_struct_annotation(
    std::unique_ptr<t_const_value> const_struct) {
  auto* ttype = const_struct->get_ttype();
  auto result =
      std::make_unique<t_const>(program, ttype, "", std::move(const_struct));
  result->set_lineno(scanner->get_lineno());
  if (mode == parsing_mode::PROGRAM) {
    validate_const_type(result.get());
  }
  return result;
}

std::unique_ptr<t_throws> parsing_driver::new_throws(
    std::unique_ptr<t_field_list> exceptions) {
  auto result = std::make_unique<t_throws>();
  if (exceptions != nullptr) {
    append_fields(*result, std::move(*exceptions));
  }
  return result;
}

void parsing_driver::append_fields(
    t_structured& tstruct, t_field_list&& fields) {
  for (auto& field : fields) {
    if (!tstruct.try_append_field(std::move(field))) {
      failure(
          "Field identifier %d for \"%s\" has already been used",
          field->get_key(),
          field->get_name().c_str());
      break;
    }
  }
}

std::unique_ptr<t_type_ref> parsing_driver::new_type_ref(
    const t_type* type, std::unique_ptr<t_annotations> annotations) {
  if (annotations != nullptr) {
    // Make a copy of the node to hold the annotations.
    // TODO(afuller): Remove the need for copying the underlying type by making
    // t_type_ref annotatable directly.
    if (const auto* tbase_type = dynamic_cast<const t_base_type*>(type)) {
      // base types can be copy constructed.
      type = add_unnamed_type(
          std::make_unique<t_base_type>(*tbase_type), std::move(annotations));
    } else {
      // Containers always use a new type, so should never show up here.
      assert(!type->is_container());
      // For all other types, we can just create a dummy typedef node with
      // the same name.
      // NOTE(afuller): This is not a safe assumption as it breaks all
      // dynamic casts and t_type::is_* calls.
      type = add_unnamed_typedef(
          std::make_unique<t_typedef>(
              const_cast<t_program*>(type->program()),
              type,
              type->get_name(),
              scope_cache),
          std::move(annotations));
    }
  }
  return std::make_unique<t_type_ref>(type);
}

std::unique_ptr<t_type_ref> parsing_driver::new_type_ref(
    std::unique_ptr<t_type> node, std::unique_ptr<t_annotations> annotations) {
  const t_type* type = node.get();
  set_annotations(node.get(), std::move(annotations));
  if (mode == parsing_mode::INCLUDES) {
    delete_at_the_end(node.release());
  } else {
    program->add_unnamed_type(std::move(node));
  }
  return std::make_unique<t_type_ref>(type);
}

std::unique_ptr<t_type_ref> parsing_driver::new_type_ref(
    std::string name,
    std::unique_ptr<t_annotations> annotations,
    bool is_const) {
  if (mode == parsing_mode::INCLUDES) {
    // Ignore identifier-based type references in include mode
    return std::make_unique<t_type_ref>();
  }

  // Try to resolve the type
  const t_type* type = scope_cache->get_type(name);
  if (type == nullptr) {
    type = scope_cache->get_type(program->name() + "." + name);
  }
  if (type == nullptr) {
    // TODO(afuller): Remove this special case for const, which requires a
    // specific declaration order.
    if (is_const) {
      failure(
          "The type '%s' is not defined yet. Types must be "
          "defined before the usage in constant values.",
          name.c_str());
    }
    // TODO(afuller): Why are interactions special? They should just be another
    // declared type.
    type = scope_cache->get_interaction(name);
    if (type == nullptr) {
      type = scope_cache->get_interaction(program->name() + "." + name);
    }
  }

  if (type != nullptr) {
    // We found the type!
    return new_type_ref(type, std::move(annotations));
  }

  /*
   Either this type isn't yet declared, or it's never
   declared. Either way allow it and we'll figure it out
   during generation.
  */
  // NOTE(afuller): This assumes that, since the type was referenced by name, it
  // is safe to create a dummy typedef to use as a proxy for the original type.
  // However, this actually breaks dynamic casts and t_type::is_* calls.
  // TODO(afuller): Resolve *all* types in a second pass.
  return std::make_unique<t_type_ref>(add_placeholder_typedef(
      std::make_unique<t_placeholder_typedef>(
          program, std::move(name), scope_cache),
      std::move(annotations)));
}

const t_type* parsing_driver::add_unnamed_type(
    std::unique_ptr<t_type> node, std::unique_ptr<t_annotations> annotations) {
  const t_type* result(node.get());
  set_annotations(node.get(), std::move(annotations));
  if (mode == parsing_mode::INCLUDES) {
    delete_at_the_end(node.release());
  } else {
    program->add_unnamed_type(std::move(node));
  }
  return result;
}

const t_type* parsing_driver::add_unnamed_typedef(
    std::unique_ptr<t_typedef> node,
    std::unique_ptr<t_annotations> annotations) {
  const t_type* result(node.get());
  set_annotations(node.get(), std::move(annotations));
  program->add_unnamed_typedef(std::move(node));
  return result;
}

const t_type* parsing_driver::add_placeholder_typedef(
    std::unique_ptr<t_placeholder_typedef> node,
    std::unique_ptr<t_annotations> annotations) {
  const t_type* result(node.get());
  set_annotations(node.get(), std::move(annotations));
  program->add_placeholder_typedef(std::move(node));
  return result;
}

t_ref<t_const> parsing_driver::add_def(std::unique_ptr<t_const> node) {
  t_ref<t_const> result(node.get());
  if (should_add_node(node)) {
    validate_const_type(node.get());
    scope_cache->add_constant(scoped_name(*node), node.get());
    program->add_const(std::move(node));
  }
  return result;
}
t_ref<t_interaction> parsing_driver::add_def(
    std::unique_ptr<t_interaction> node) {
  t_ref<t_interaction> result(node.get());
  if (should_add_node(node)) {
    scope_cache->add_interaction(scoped_name(*node), node.get());
    program->add_interaction(std::move(node));
  }
  return result;
}

t_ref<t_service> parsing_driver::add_def(std::unique_ptr<t_service> node) {
  t_ref<t_service> result(node.get());
  assert(!node->is_interaction());
  if (should_add_node(node)) {
    scope_cache->add_service(scoped_name(*node), node.get());
    program->add_service(std::move(node));
  }
  return result;
}

t_ref<t_typedef> parsing_driver::add_def(std::unique_ptr<t_typedef> node) {
  t_ref<t_typedef> result(node.get());
  if (should_add_type(node)) {
    program->add_typedef(std::move(node));
  }
  return result;
}

t_ref<t_struct> parsing_driver::add_def(std::unique_ptr<t_struct> node) {
  t_ref<t_struct> result(node.get());
  if (should_add_type(node)) {
    program->add_struct(std::move(node));
  }
  return result;
}

t_ref<t_union> parsing_driver::add_def(std::unique_ptr<t_union> node) {
  t_ref<t_union> result(node.get());
  if (should_add_type(node)) {
    program->add_struct(std::move(node));
  }
  return result;
}

t_ref<t_exception> parsing_driver::add_def(std::unique_ptr<t_exception> node) {
  t_ref<t_exception> result(node.get());
  if (should_add_type(node)) {
    program->add_xception(std::move(node));
  }
  return result;
}

t_ref<t_enum> parsing_driver::add_def(std::unique_ptr<t_enum> node) {
  t_ref<t_enum> result(node.get());

  if (should_add_type(node)) {
    // Register enum value names in scope.
    std::string type_prefix = ".";
    for (const auto* value : node->consts()) {
      // TODO(afuller): Remove ability to access unscoped enum values.
      scope_cache->add_constant(scoped_name(*value), value);
      scope_cache->add_constant(scoped_name(*node, *value), value);
    }
    program->add_enum(std::move(node));
  }
  return result;
}
} // namespace compiler
} // namespace thrift
} // namespace apache
