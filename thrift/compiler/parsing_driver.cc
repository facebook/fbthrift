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
#include <thrift/compiler/parsing_driver.h>

#include <cstdarg>

#include <boost/filesystem.hpp>

namespace apache {
namespace thrift {

void parsing_driver::debug(const char* fmt, ...) const {
  if (!params.debug) {
    return;
  }
  va_list args;
  printf("[PARSE:%d] ", yylineno);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void parsing_driver::verbose(const char* fmt, ...) const {
  if (!params.verbose) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void parsing_driver::warning(int level, const char* fmt, ...) const {
  if (params.warn < level) {
    return;
  }
  va_list args;
  fprintf(stderr, "[WARNING:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

[[noreturn]] void parsing_driver::failure(const char* fmt, ...) const {
  va_list args;
  fprintf(stderr, "[FAILURE:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

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
    vector<std::string> sp = params.incl_searchpath;
    sp.insert(sp.begin(), curdir_);

    // iterate through paths
    vector<std::string>::iterator it;
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
    throw string("type error: cannot declare a void const: " + name);
  }

  auto as_struct = dynamic_cast<t_struct*>(type);
  assert((as_struct != nullptr) == type->is_struct());
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
        if (value->get_type() != t_const_value::CV_STRING) {
          throw string(
              "type error: const \"" + name + "\" was declared as string");
        }
        break;
      case t_base_type::TYPE_BOOL:
        if (value->get_type() != t_const_value::CV_BOOL &&
            value->get_type() != t_const_value::CV_INTEGER) {
          throw string(
              "type error: const \"" + name + "\" was declared as bool");
        }
        break;
      case t_base_type::TYPE_BYTE:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw string(
              "type error: const \"" + name + "\" was declared as byte");
        }
        break;
      case t_base_type::TYPE_I16:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw string(
              "type error: const \"" + name + "\" was declared as i16");
        }
        break;
      case t_base_type::TYPE_I32:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw string(
              "type error: const \"" + name + "\" was declared as i32");
        }
        break;
      case t_base_type::TYPE_I64:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          throw string(
              "type error: const \"" + name + "\" was declared as i64");
        }
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        if (value->get_type() != t_const_value::CV_INTEGER &&
            value->get_type() != t_const_value::CV_DOUBLE) {
          throw string(
              "type error: const \"" + name + "\" was declared as double");
        }
        break;
      default:
        throw string(
            "compiler error: no const of base type " +
            t_base_type::t_base_name(tbase) + name);
    }
  } else if (type->is_enum()) {
    if (value->get_type() != t_const_value::CV_INTEGER) {
      throw string("type error: const \"" + name + "\" was declared as enum");
    }
    const auto as_enum = dynamic_cast<t_enum*>(type);
    assert(as_enum != nullptr);
    const auto enum_val = as_enum->find_value(value->get_integer());
    if (enum_val == nullptr) {
      pwarning(
          0,
          "type error: const \"%s\" was declared as enum \"%s\" with a value"
          " not of that enum",
          name.c_str(),
          type->get_name().c_str());
    }
  } else if (as_struct && as_struct->is_union()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw string("type error: const \"" + name + "\" was declared as union");
    }
    auto const& map = value->get_map();
    if (map.size() > 1) {
      throw string(
          "type error: const \"" + name +
          "\" is a union and can't "
          "have more than one field set");
    }
    if (!map.empty()) {
      if (map.front().first->get_type() != t_const_value::CV_STRING) {
        throw string(
            "type error: const \"" + name +
            "\" is a union and member "
            "names must be a string");
      }
      auto const& member_name = map.front().first->get_string();
      auto const& member = as_struct->get_member(member_name);
      if (!member) {
        throw string(
            "type error: no member named \"" + member_name +
            "\" for "
            "union const \"" +
            name + "\"");
      }
    }
  } else if (type->is_struct() || type->is_xception()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw string(
          "type error: const \"" + name + "\" was declared as " +
          "struct/exception");
    }
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;

    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      if (v_iter->first->get_type() != t_const_value::CV_STRING) {
        throw string("type error: " + name + " struct key must be string");
      }
      t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw string(
            "type error: " + type->get_name() + " has no field " +
            v_iter->first->get_string());
      }

      validate_const_rec(
          name + "." + v_iter->first->get_string(), field_type, v_iter->second);
    }
  } else if (type->is_map()) {
    t_type* k_type = ((t_map*)type)->get_key_type();
    t_type* v_type = ((t_map*)type)->get_val_type();
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
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
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
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

} // namespace thrift
} // namespace apache
