/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <errno.h>
#include <stdlib.h>
#include <cmath>
#include <cstdarg>
#include <limits>
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

// We don't use std::isspace since it's locale-dependent
bool is_white_space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// If the given defintion defines a new scope for field ids.
bool is_field_scope(DefType type) {
  switch (type) {
    case DefType::Function:
    case DefType::Struct:
    case DefType::Union:
    case DefType::Exception:
      return true;
    default:
      return false;
  }
}

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

  parser_ = std::make_unique<yy::parser>(
      *this, scanner->get_scanner(), &yylval_, &yylloc_);

  try {
    parse_file();
    result = std::move(program_bundle);
    assert(def_stack_.empty());
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
    reset_locations();
  } catch (std::runtime_error const& ex) {
    failure(ex.what());
  }

  // Create new scope and scan for includes
  verbose([&](auto& o) { o << "Scanning " << path << " for includes\n"; });
  mode = parsing_mode::INCLUDES;
  try {
    if (parser_->parse() != 0) {
      failure("Parser error during include pass.");
    }
  } catch (const std::string& x) {
    failure(x);
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
      failure([&](auto& o) {
        o << "Circular dependency found: file `" << included_program->path()
          << "` is already parsed.";
      });
    }

    // This must be after the previous circular include check, since the emitted
    // error message above is supposed to reference the parent file name.
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
    reset_locations();
  } catch (std::runtime_error const& ex) {
    failure(ex.what());
  }

  mode = parsing_mode::PROGRAM;
  verbose([&](auto& o) { o << "Parsing " << path << " for types\n"; });
  try {
    if (parser_->parse() != 0) {
      failure("Parser error during types pass.");
    }
  } catch (const std::string& x) {
    failure(x);
  }

  for (auto td : program->placeholder_typedefs()) {
    if (!td->resolve()) {
      failure(
          [&](auto& o) { o << "Type `" << td->name() << "` not defined."; });
    }
  }
}

[[noreturn]] void parsing_driver::end_parsing() {
  throw parsing_terminator{};
}

// TODO: This doesn't really need to be a member function. Move it somewhere
// else (e.g. `util.{h|cc}`) once everything gets consolidated into `parse/`.
std::string parsing_driver::directory_name(const std::string& filename) {
  boost::filesystem::path fullpath = filename;
  auto parent_path = fullpath.parent_path();
  auto result = parent_path.string();
  // No parent dir, just use the current directory
  if (result.empty()) {
    return ".";
  }
  return result;
}

std::string parsing_driver::find_include_file(const std::string& filename) {
  // Absolute path? Just try that
  boost::filesystem::path path{filename};
  if (path.has_root_directory()) {
    try {
      return boost::filesystem::canonical(path).string();
    } catch (const boost::filesystem::filesystem_error& e) {
      failure([&](auto& o) {
        o << "Could not find file: " << filename << ". Error: " << e.what();
      });
    }
  }

  // relative path, start searching
  // new search path with current dir global
  std::vector<std::string> sp = params.incl_searchpath;
  sp.insert(sp.begin(), directory_name(program->path()));
  // iterate through paths
  std::vector<std::string>::iterator it;
  for (it = sp.begin(); it != sp.end(); it++) {
    boost::filesystem::path sfilename = filename;
    if ((*it) != "." && (*it) != "") {
      sfilename = boost::filesystem::path(*(it)) / filename;
    }
    if (boost::filesystem::exists(sfilename)) {
      return sfilename.string();
    } else {
      debug([&](auto& o) { o << "Could not find: " << filename << "."; });
    }
  }
  // File was not found
  failure([&](auto& o) { o << "Could not find include file " << filename; });
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
    warning(msg);
  }
}
void parsing_driver::clear_doctext() {
  if (doctext && mode == parsing_mode::PROGRAM) {
    warning_strict([&](auto& o) {
      o << "Uncaptured doctext at on line " << doctext_lineno << ".";
    });
  }

  doctext = boost::none;
}

t_doc parsing_driver::capture_doctext() {
  if (mode != parsing_mode::PROGRAM) {
    return boost::none;
  }
  return std::exchange(doctext, boost::none);
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
    warning_strict([&](auto& o) {
      o << "'" << feature << "' is an experimental feature.";
    });
    return true;
  }
  failure(
      [&](auto& o) { o << "'" << feature << "' is an experimental feature."; });
  return false;
}

void parsing_driver::set_annotations(
    t_node* node, std::unique_ptr<t_annotations> annotations) {
  if (annotations != nullptr) {
    node->reset_annotations(annotations->strings);
  }
}

void parsing_driver::set_attributes(
    t_named& node,
    const YYLTYPE& loc,
    std::unique_ptr<t_def_attrs> attrs,
    std::unique_ptr<t_annotations> annotations) const {
  node.set_src_range(get_source_range(loc));
  if (attrs != nullptr) {
    if (attrs->doc) {
      node.set_doc(std::move(*attrs->doc));
    }
    if (attrs->struct_annotations != nullptr) {
      for (auto& an : *attrs->struct_annotations) {
        node.add_structured_annotation(std::move(an));
      }
    }
  }
  set_annotations(&node, std::move(annotations));
}

void parsing_driver::set_attributes(
    t_named& node,
    YYLTYPE& loc,
    std::unique_ptr<t_def_attrs> attrs,
    const YYLTYPE& attrs_loc,
    std::unique_ptr<t_annotations> annots,
    const YYLTYPE& annots_loc) const {
  // Adjust the defintions source location to include attributes and
  // annotations, if present.
  if (attrs != nullptr) {
    loc.begin = attrs_loc.begin;
  }
  if (annots != nullptr) {
    loc.end = annots_loc.end;
  }
  set_attributes(node, loc, std::move(attrs), std::move(annots));
}

source_range parsing_driver::get_source_range(const YYLTYPE& loc) const {
  return source_range(
      *program, loc.begin.line, loc.begin.column, loc.end.line, loc.end.column);
}

void parsing_driver::reset_locations() {
  yylloc_.begin.line = 1;
  yylloc_.begin.column = 1;
  yylloc_.end.line = 1;
  yylloc_.end.column = 1;
  yylval_ = 0;
}

void parsing_driver::compute_location(
    YYLTYPE& yylloc, YYSTYPE& yylval, const char* text) {
  /* Only computing locations during second pass. */
  if (mode != parsing_mode::PROGRAM) {
    return;
  }

  int i = 0;

  // Updating current begin to previous end.
  yylloc.begin = yylloc.end;

  // Getting rid of useless whitespaces on begin position.
  for (; is_white_space(text[i]); i++) {
    yylval++;
    if (text[i] == '\n') {
      yylloc.begin.line++;
      yylloc.begin.column = 1;
      program->add_line_offset(yylval);
    } else {
      yylloc.begin.column++;
    }
  }

  // Avoid scanning whitespaces twice.
  yylloc.end = yylloc.begin;

  // Updating current end position.
  for (; text[i] != '\0'; i++) {
    yylval++;
    if (text[i] == '\n') {
      yylloc.end.line++;
      yylloc.end.column = 1;
      program->add_line_offset(yylval);
    } else {
      yylloc.end.column++;
    }
  }
}

std::unique_ptr<t_const> parsing_driver::new_struct_annotation(
    std::unique_ptr<t_const_value> const_struct) {
  auto ttype = const_struct->ttype().value(); // Copy the t_type_ref.
  auto result = std::make_unique<t_const>(
      program, std::move(ttype), "", std::move(const_struct));
  result->set_lineno(scanner->get_lineno());
  return result;
}

std::unique_ptr<t_throws> parsing_driver::new_throws(
    std::unique_ptr<t_field_list> exceptions) {
  assert(exceptions != nullptr);
  auto result = std::make_unique<t_throws>();
  append_fields(*result, std::move(*exceptions));
  return result;
}

void parsing_driver::append_fields(
    t_structured& tstruct, t_field_list&& fields) {
  for (auto& field : fields) {
    if (!tstruct.try_append_field(std::move(field))) {
      // Since we process root_program twice, we need to check parsing mode to
      // avoid double reporting
      if (mode == parsing_mode::PROGRAM ||
          program != program_bundle->root_program()) {
        ctx_.failure(*field, [&](auto& o) {
          o << "Field identifier " << field->get_key() << " for \""
            << field->get_name() << "\" has already been used.";
        });
      }
    }
  }
}

t_type_ref parsing_driver::new_type_ref(
    const t_type& type, std::unique_ptr<t_annotations> annotations) {
  if (annotations == nullptr) {
    return type;
  }

  // Make a copy of the node to hold the annotations.
  // TODO(afuller): Remove the need for copying the underlying type by making
  // t_type_ref annotatable directly.
  if (const auto* tbase_type = dynamic_cast<const t_base_type*>(&type)) {
    // base types can be copy constructed.
    auto node = std::make_unique<t_base_type>(*tbase_type);
    set_annotations(node.get(), std::move(annotations));
    t_type_ref result(*node);
    if (mode == parsing_mode::INCLUDES) {
      delete_at_the_end(node.release());
    } else {
      program->add_unnamed_type(std::move(node));
    }
    return result;
  }

  // Containers always use a new type, so should never show up here.
  assert(!type.is_container());
  // For all other types, we can just create a dummy typedef node with
  // the same name.
  // NOTE(afuller): This is not a safe assumption as it breaks all
  // dynamic casts and t_type::is_* calls.
  return *add_unnamed_typedef(
      std::make_unique<t_typedef>(
          const_cast<t_program*>(type.program()), type.get_name(), type),
      std::move(annotations));
}

t_type_ref parsing_driver::new_type_ref(
    std::unique_ptr<t_templated_type> node,
    std::unique_ptr<t_annotations> annotations) {
  assert(node != nullptr);
  const t_type* type = node.get();
  set_annotations(node.get(), std::move(annotations));
  if (mode == parsing_mode::INCLUDES) {
    delete_at_the_end(node.release());
  } else {
    program->add_type_instantiation(std::move(node));
  }
  return *type;
}

t_type_ref parsing_driver::new_type_ref(
    std::string name,
    std::unique_ptr<t_annotations> annotations,
    bool is_const) {
  if (mode == parsing_mode::INCLUDES) {
    // Ignore identifier-based type references in include mode
    return {};
  }

  // Try to resolve the type.
  const t_type* type = scope_cache->find_type(name);
  if (type == nullptr) {
    type = scope_cache->find_type(program->name() + "." + name);
  }
  if (type == nullptr) {
    // TODO(afuller): Remove this special case for const, which requires a
    // specific declaration order.
    if (is_const) {
      failure([&](auto& o) {
        o << "The type '" << name << "' is not defined yet. Types must be "
          << "defined before the usage in constant values.";
      });
    }
    // TODO(afuller): Why are interactions special? They should just be another
    // declared type.
    type = scope_cache->find_interaction(name);
    if (type == nullptr) {
      type = scope_cache->find_interaction(program->name() + "." + name);
    }
  }

  if (type != nullptr) {
    // We found the type!
    return new_type_ref(*type, std::move(annotations));
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
  return t_type_ref(*add_placeholder_typedef(
      std::make_unique<t_placeholder_typedef>(
          program, std::move(name), scope_cache),
      std::move(annotations)));
}

void parsing_driver::begin_def(DefType type) {
  def_stack_.push({type, scanner->get_lineno()});
  if (is_field_scope(type)) {
    next_id_stack_.push(-1);
  }
}

void parsing_driver::end_def(t_named& node) {
  node.set_lineno(def_stack_.top().lineno);
  if (is_field_scope(def_stack_.top().type)) {
    next_id_stack_.pop();
  }
  def_stack_.pop();
}

void parsing_driver::end_def(
    t_structured& node, std::unique_ptr<t_field_list> fields) {
  append_fields(node, std::move(*fields));
  end_def(node);
}

void parsing_driver::end_def(
    t_interface& node, std::unique_ptr<t_function_list> functions) {
  if (functions != nullptr) {
    node.set_functions(std::move(*functions));
  }
  end_def(node);
}

t_ref<t_named> parsing_driver::add_def(std::unique_ptr<t_named> node) {
  t_ref<t_named> result(node.get());
  if (should_add_node(node)) {
    // Add to scope.
    // TODO(afuller): Move program level scope management to t_program.
    if (auto* tnode = dynamic_cast<t_interaction*>(node.get())) {
      scope_cache->add_interaction(scoped_name(*node), tnode);
    } else if (auto* tnode = dynamic_cast<t_service*>(node.get())) {
      scope_cache->add_service(scoped_name(*node), tnode);
    } else if (auto* tnode = dynamic_cast<t_const*>(node.get())) {
      scope_cache->add_constant(scoped_name(*node), tnode);
    } else if (auto* tnode = dynamic_cast<t_enum*>(node.get())) {
      scope_cache->add_type(scoped_name(*node), tnode);
      // Register enum value names in scope.
      for (const auto& value : tnode->consts()) {
        // TODO(afuller): Remove ability to access unscoped enum values.
        scope_cache->add_constant(scoped_name(value), &value);
        scope_cache->add_constant(scoped_name(*node, value), &value);
      }
    } else if (auto* tnode = dynamic_cast<t_type*>(node.get())) {
      scope_cache->add_type(scoped_name(*node), tnode);
    } else {
      throw std::logic_error("Unsupported declaration.");
    }
    // Add to program.
    program->add_definition(std::move(node));
  }
  return result;
}

void parsing_driver::add_include(std::string name) {
  if (mode != parsing_mode::INCLUDES) {
    return;
  }

  std::string path = find_include_file(name);
  assert(!path.empty()); // Should have throw an exception if not found.

  if (program_cache.find(path) == program_cache.end()) {
    auto included_program =
        program->add_include(path, name, scanner->get_lineno());
    program_cache[path] = included_program.get();
    program_bundle->add_program(std::move(included_program));
  } else {
    auto include = std::make_unique<t_include>(program_cache[path]);
    include->set_lineno(scanner->get_lineno());
    program->add_include(std::move(include));
  }
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

t_field_id parsing_driver::to_field_id(int64_t int_const) {
  using limits = std::numeric_limits<t_field_id>;
  if (int_const < limits::min() || int_const > limits::max()) {
    // Not representable as a field id.
    failure([&](auto& o) {
      o << "Integer constant (" << int_const
        << ") outside the range of field ids ([" << limits::min() << ", "
        << limits::max() << "]).";
    });
  }
  return int_const;
}

t_field_id parsing_driver::allocate_field_id(const std::string& name) {
  assert(!next_id_stack_.empty());
  if (params.strict >= 192) {
    failure("Implicit field keys are deprecated and not allowed with -strict");
  }
  if (next_id_stack_.top() < t_field::min_id) {
    failure(
        "Cannot allocate an id for `" + name +
        "`. Automatic field ids are exhausted.");
  }
  return next_id_stack_.top()--;
}

void parsing_driver::reserve_field_id(t_field_id id) {
  assert(!next_id_stack_.empty());
  if (id < 0) {
    /*
     * Update next field id to be one less than the value.
     * The FieldList parsing will catch any duplicate id values.
     */
    next_id_stack_.top() = id - 1;
  }
}

t_field_id parsing_driver::maybe_allocate_field_id(
    boost::optional<t_field_id>& idlId, const std::string& name) {
  if (idlId == boost::none) {
    // Auto assign an id.
    return allocate_field_id(name);
  }

  t_field_id id = *idlId;
  if (id <= 0) {
    // TODO(afuller): Move this validation to ast_validator.
    if (params.allow_neg_field_keys) {
      /*
       * allow_neg_field_keys exists to allow users to add explicitly
       * specified id values to old .thrift files without breaking
       * protocol compatibility.
       */
      if (id != next_field_id()) {
        /*
         * warn if the user-specified negative value isn't what
         * thrift would have auto-assigned.
         */
        warning([&](auto& o) {
          o << "Nonpositive field id (" << id << ") differs from what would "
            << "be auto-assigned by thrift (" << next_field_id() << ").";
        });
      }
    } else if (id == next_field_id()) {
      warning([&](auto& o) {
        o << "Nonpositive value (" << id << ") not allowed as a field id.";
      });
    } else {
      // TODO(afuller): Make ignoring the user provided value a failure.
      warning([&](auto& o) {
        o << "Nonpositive field id (" << id
          << ") differs from what is auto-"
             "assigned by thrift. The id must positive or "
          << next_field_id() << ".";
      });
      // Ignore user provided value and auto assign an id.
      id = allocate_field_id(name);
      idlId = boost::none;
    }
    reserve_field_id(id);
  }
  return id;
}

uint64_t parsing_driver::parse_integer(const char* text, int offset, int base) {
  errno = 0;
  uint64_t val = strtoull(text + offset, nullptr, base);
  if (errno == ERANGE) {
    failure([&](auto& o) { o << "This integer is too big: " << text << "\n"; });
  }
  return val;
}

int64_t parsing_driver::to_int(uint64_t val, bool negative) {
  constexpr uint64_t i64max = std::numeric_limits<int64_t>::max();
  if (negative) {
    if (val > i64max + 1) { // Can store one more negative number.
      failure(
          [&](auto& o) { o << "This integer is too small: -" << val << "\n"; });
    }
    return -val;
  }
  if (val > i64max) {
    failure([&](auto& o) { o << "This integer is too big: " << val << "\n"; });
  }
  return val;
}

double parsing_driver::parse_double(const char* text) {
  errno = 0;
  double val = strtod(text, nullptr);
  if (errno == ERANGE) {
    if (val == 0) {
      failure([&](auto& o) {
        o << "This number is too infinitesimal: " << text << "\n";
      });
    } else if (val == HUGE_VAL) {
      failure(
          [&](auto& o) { o << "This number is too big: " << text << "\n"; });
    } else if (val == -HUGE_VAL) {
      failure(
          [&](auto& o) { o << "This number is too small: " << text << "\n"; });
    }
    // Allow subnormals.
  }
  return val;
}

void parsing_driver::parse_doctext(const char* text, int lineno) {
  if (mode != apache::thrift::compiler::parsing_mode::PROGRAM) {
    return;
  }

  // Deal with prefix/suffix.
  std::string str{text};
  if (str.compare(0, 3, "/**") == 0) {
    str = str.substr(3, str.length() - 3 - 2);
  } else if (str.compare(0, 3, "///") == 0) {
    str = str.substr(3, str.length() - 3);
  }

  clear_doctext();
  doctext = clean_up_doctext(str);
  doctext_lineno = lineno;
}

const t_service* parsing_driver::find_service(const std::string& name) {
  if (mode != parsing_mode::PROGRAM) {
    return nullptr;
  }
  if (auto* result = scope_cache->find_service(name)) {
    return result;
  }
  if (auto* result = scope_cache->find_service(scoped_name(name))) {
    return result;
  }
  failure([&](auto& o) {
    o << "Service \"" << name << "\" has not been defined.";
  });
}

const t_const* parsing_driver::find_const(const std::string& name) {
  validate_not_ambiguous_enum(name);
  if (const t_const* constant = scope_cache->find_constant(name)) {
    return constant;
  }
  if (const t_const* constant = scope_cache->find_constant(scoped_name(name))) {
    validate_not_ambiguous_enum(scoped_name(name));
    return constant;
  }
  return nullptr;
}

std::unique_ptr<t_const_value> parsing_driver::copy_const_value(
    const std::string& name) {
  if (const t_const* constant = find_const(name)) {
    // Copy const_value to perform isolated mutations
    auto result = constant->get_value()->clone();
    // We only want to clone the value, while discarding all real type
    // information.
    result->set_ttype(boost::none);
    result->set_is_enum(false);
    result->set_enum(nullptr);
    result->set_enum_value(nullptr);
    return result;
  }

  // TODO(afuller): Make this an error.
  if (mode == parsing_mode::PROGRAM) {
    warning(
        [&](auto& o) { o << "Constant strings should be quoted: " << name; });
  }
  return std::make_unique<t_const_value>(name);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
