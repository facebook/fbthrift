/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <thrift/compiler/parse/t_const_value.h>
#include <thrift/compiler/parse/t_list.h>
#include <thrift/compiler/parse/t_map.h>
#include <thrift/compiler/parse/t_set.h>

class t_program;

/**
 *  class t_const
 *
 * A const is a constant value defined across languages that has a type and
 * a value. The trick here is that the declared type might not match the type
 * of the value object, since that is not determined until after parsing the
 * whole thing out.
 *
 */
class t_const : public t_doc {
 public:

  /**
   * Constructor for t_const
   *
   * @param program - An entire thrift program
   * @param type    - A thrift type
   * @param name    - The name of the constant variable
   * @param value   - The constant value
   */
  t_const(
      t_program* program,
      t_type* type,
      std::string name,
      t_const_value* value) :
        program_(program), type_(type), name_(name), value_(value) {
    if (value) {
      value->set_owner(this);
      set_values_type(type, value);
    }
  }

  /**
   * t_const getters
   */
  t_program* get_program() const { return program_; }

  t_type* get_type() const { return type_; }

  std::string get_name() const { return name_; }

  t_const_value* get_value() const { return value_; }

  void set_values_type(t_type* type, t_const_value* value) {
    value->set_ttype(type);
    if (type->is_list()) {
      auto* elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
      for (auto list_val : value->get_list()) {
        set_values_type(elem_type, list_val);
      }
    }
    if (type->is_set()) {
      auto* elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
      for (auto set_val : value->get_list()) {
        set_values_type(elem_type, set_val);
      }
    }
    if (type->is_map()) {
      auto* key_type = dynamic_cast<const t_map*>(type)->get_key_type();
      auto* val_type = dynamic_cast<const t_map*>(type)->get_val_type();
      for (auto map_val : value->get_map()) {
        set_values_type(key_type, map_val.first);
        set_values_type(val_type, map_val.second);
      }
    }
  }

 private:
  t_program* program_;
  t_type* type_;
  std::string name_;
  t_const_value* value_;
};
