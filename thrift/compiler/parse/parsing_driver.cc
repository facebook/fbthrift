/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <thrift/compiler/parse/parsing_driver.h>

#include <cstdarg>

#include <boost/filesystem.hpp>

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

namespace apache {
namespace thrift {

namespace {

class parsing_terminator : public std::runtime_error {
 public:
  parsing_terminator()
      : std::runtime_error(
            "Internal exception used to terminate the parsing process.") {}
};

} // namespace

parsing_driver::parsing_driver(std::string path, parsing_params parse_params)
    : params(std::move(parse_params)),
      doctext(nullptr),
      doctext_lineno(0),
      mode(parsing_mode::INCLUDES) {
  program = new t_program(path);
  scope_cache = program->scope();
}

/**
 * The default destructor needs to be explicitly defined in the .cc file since
 * it invokes the destructor of parse_ (of type unique_ptr<yy::parser>). It
 * cannot go in the header file since yy::parser is only forward-declared there.
 */
parsing_driver::~parsing_driver() = default;

std::unique_ptr<t_program> parsing_driver::parse(
    std::vector<diagnostic_message>& messages) {
  std::unique_ptr<t_program> result{};

  try {
    scanner = std::make_unique<yy_scanner>();
  } catch (std::system_error const&) {
    return result;
  }

  parser_ = std::make_unique<apache::thrift::yy::parser>(
      *this, scanner->get_scanner());

  try {
    parse_file();
    result.reset(program);
  } catch (const parsing_terminator& e) {
    // No need to do anything here. The purpose of the exception is simply to
    // end the parsing process by unwinding to here.
  }

  std::swap(messages, diagnostic_messages_);
  diagnostic_messages_.clear();

  return result;
}

void parsing_driver::parse_file() {
  // Get scope file path
  std::string path = program->get_path();

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
  mode = apache::thrift::parsing_mode::INCLUDES;
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
    if (circular_deps_.count(included_program->get_path())) {
      failure(
          "Circular dependency found: file %s is already parsed.",
          included_program->get_path().c_str());
    }

    // This must be after the previous circular include check, since the emitted
    // error message above is supposed to reference the parent file name.
    program = included_program;
    params.allow_neg_enum_vals = true;
    params.allow_neg_field_keys = true;
    parse_file();

    size_t num_removed = circular_deps_.erase(path);
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

  mode = apache::thrift::parsing_mode::PROGRAM;
  verbose("Parsing %s for types\n", path.c_str());
  try {
    if (parser_->parse() != 0) {
      failure("Parser error during types pass.");
    }
  } catch (const std::string& x) {
    failure(x.c_str());
  }

  for (auto td : program->get_named_placeholder_typedefs()) {
    if (!td->resolve_placeholder()) {
      failure("Type \"%s\" not defined.", td->get_symbolic().c_str());
    }
  }
}

[[noreturn]] void parsing_driver::end_parsing() { throw parsing_terminator{}; }

// TODO: This doesn't really need to be a member function. Move it somewhere
// else (e.g. `util.{h|cc}`) once everything gets consolidated into `parse/`.
/* static */ std::string
    parsing_driver::directory_name(const std::string& filename) {
  std::string::size_type slash = filename.rfind("/");
  // No slash, just use the current directory
  if (slash == std::string::npos) {
    return ".";
  }
  return filename.substr(0, slash);
}

std::string parsing_driver::include_file(const std::string& filename) {
  // Absolute path? Just try that
  if (filename[0] == '/') {
    boost::filesystem::path abspath{filename};
    try {
      abspath = boost::filesystem::canonical(abspath);
      return abspath.string();
    } catch (const boost::filesystem::filesystem_error& e) {
      failure("Could not find file: %s. Error: %s", filename.c_str(), e.what());
    }
  } else { // relative path, start searching
    // new search path with current dir global
    std::vector<std::string> sp = params.incl_searchpath;
    sp.insert(sp.begin(), directory_name(program->get_path()));

    // iterate through paths
    std::vector<std::string>::iterator it;
    for (it = sp.begin(); it != sp.end(); it++) {
      std::string sfilename = *(it) + "/" + filename;
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
    std::string name,
    t_type* type,
    t_const_value* value) {
  if (type->is_void()) {
    throw std::string("type error: cannot declare a void const: " + name);
  }

  auto as_struct = dynamic_cast<t_struct*>(type);
  assert((as_struct != nullptr) == type->is_struct());
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
        if (value->get_type() != t_const_value::CV_STRING) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as string");
        }
        break;
      case t_base_type::TYPE_BOOL:
        if (value->get_type() != t_const_value::CV_BOOL &&
            value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as bool");
        }
        break;
      case t_base_type::TYPE_BYTE:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as byte");
        }
        break;
      case t_base_type::TYPE_I16:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as i16");
        }
        break;
      case t_base_type::TYPE_I32:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as i32");
        }
        break;
      case t_base_type::TYPE_I64:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as i64");
        }
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        if (value->get_type() != t_const_value::CV_INTEGER &&
            value->get_type() != t_const_value::CV_DOUBLE) {
          throw std::string(
              "type error: const \"" + name + "\" was declared as double");
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
          "type error: const \"" + name + "\" was declared as enum");
    }
    const auto as_enum = dynamic_cast<t_enum*>(type);
    assert(as_enum != nullptr);
    const auto enum_val = as_enum->find_value(value->get_integer());
    if (enum_val == nullptr) {
      warning(
          0,
          "type error: const \"%s\" was declared as enum \"%s\" with a value"
          " not of that enum",
          name.c_str(),
          type->get_name().c_str());
    }
  } else if (as_struct && as_struct->is_union()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw std::string(
          "type error: const \"" + name + "\" was declared as union");
    }
    auto const& map = value->get_map();
    if (map.size() > 1) {
      throw std::string(
          "type error: const \"" + name +
          "\" is a union and can't "
          "have more than one field set");
    }
    if (!map.empty()) {
      if (map.front().first->get_type() != t_const_value::CV_STRING) {
        throw std::string(
            "type error: const \"" + name +
            "\" is a union and member "
            "names must be a string");
      }
      auto const& member_name = map.front().first->get_string();
      auto const& member = as_struct->get_member(member_name);
      if (!member) {
        throw std::string(
            "type error: no member named \"" + member_name +
            "\" for "
            "union const \"" +
            name + "\"");
      }
    }
  } else if (type->is_struct() || type->is_xception()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw std::string(
          "type error: const \"" + name + "\" was declared as " +
          "struct/exception");
    }
    const std::vector<t_field*>& fields = ((t_struct*)type)->get_members();
    std::vector<t_field*>::const_iterator f_iter;

    const std::vector<std::pair<t_const_value*, t_const_value*>>& val =
        value->get_map();
    std::vector<std::pair<t_const_value*, t_const_value*>>::const_iterator
        v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      if (v_iter->first->get_type() != t_const_value::CV_STRING) {
        throw std::string("type error: " + name + " struct key must be string");
      }
      t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw std::string(
            "type error: " + type->get_name() + " has no field " +
            v_iter->first->get_string());
      }

      validate_const_rec(
          name + "." + v_iter->first->get_string(), field_type, v_iter->second);
    }
  } else if (type->is_map()) {
    t_type* k_type = ((t_map*)type)->get_key_type();
    t_type* v_type = ((t_map*)type)->get_val_type();
    const std::vector<std::pair<t_const_value*, t_const_value*>>& val =
        value->get_map();
    std::vector<std::pair<t_const_value*, t_const_value*>>::const_iterator
        v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      validate_const_rec(name + "<key>", k_type, v_iter->first);
      validate_const_rec(name + "<val>", v_type, v_iter->second);
    }
  } else if (type->is_list() || type->is_set()) {
    t_type* e_type;
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

void parsing_driver::clear_doctext() {
  if (doctext != nullptr) {
    warning(2, "Uncaptured doctext at on line %d.", doctext_lineno);
  }
  free(doctext);
  doctext = nullptr;
}

char* parsing_driver::clean_up_doctext(char* doctext) {
  // Convert to C++ string, and remove Windows's carriage returns.
  std::string docstring = doctext;
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
    return nullptr;
  }

  // Clear leading whitespace from the first line.
  pos = lines.front().find_first_not_of(" \t");
  lines.front().erase(0, pos);

  // If every nonblank line after the first has the same number of spaces/tabs,
  // then a star, remove them.
  bool have_prefix = true;
  bool found_prefix = false;
  std::string::size_type prefix_len = 0;
  std::vector<std::string>::iterator l_iter;
  for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }

    pos = l_iter->find_first_not_of(" \t");
    if (!found_prefix) {
      if (pos != std::string::npos) {
        if (l_iter->at(pos) == '*') {
          found_prefix = true;
          prefix_len = pos;
        } else {
          have_prefix = false;
          break;
        }
      } else {
        // Whitespace-only line.  Truncate it.
        l_iter->clear();
      }
    } else if (
        l_iter->size() > pos && l_iter->at(pos) == '*' && pos == prefix_len) {
      // Business as usual.
    } else if (pos == std::string::npos) {
      // Whitespace-only line.  Let's truncate it for them.
      l_iter->clear();
    } else {
      // The pattern has been broken.
      have_prefix = false;
      break;
    }
  }

  // If our prefix survived, delete it from every line.
  if (have_prefix) {
    // Get the star too.
    prefix_len++;
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

  // If our prefix survived, delete it from every line.
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

  assert(docstring.length() <= strlen(doctext));
  strcpy(doctext, docstring.c_str());
  return doctext;
}

} // namespace thrift
} // namespace apache
