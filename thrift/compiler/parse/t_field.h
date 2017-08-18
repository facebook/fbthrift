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

#include <string>
#include <sstream>

#include <thrift/compiler/parse/t_doc.h>

class t_const_value;
class t_struct;

/**
 * class t_field
 *
 * Class to represent a field in a thrift structure. A field has a data type,
 * a symbolic name, and a numeric identifier.
 *
 */
class t_field : public t_doc {
 public:

  /**
   * Constructor for t_field
   *
   * @param type - A field based on thrift types
   * @param name - The symbolic name of the field
   */
  t_field(t_type* type, std::string name) :
    type_(type),
    name_(name) {}

  /**
   * Constructor for t_field
   *
   * @param type - A field based on thrift types
   * @param name - The symbolic name of the field
   * @param key  - The numeric identifier of the field
   */
  t_field(t_type* type, std::string name, int32_t key) :
    type_(type),
    name_(name),
    key_(key) {}

  t_field(const t_field&) = default;

  ~t_field() {}

  /**
   * Determines if the field is required in the thrift object
   */
  enum e_req {
    T_REQUIRED = 0,
    T_OPTIONAL = 1,
    T_OPT_IN_REQ_OUT = 2,
  };

  /**
   * t_field setters
   */
  void set_value(t_const_value* value) {
    value_ = value;
  }

  void set_req(e_req req) { req_ = req; }

  /**
   * t_field getters
   */
  t_type* get_type() const { return type_; }

  const std::string& get_name() const { return name_; }

  int32_t get_key() const { return key_; }

  const t_const_value* get_value() const { return value_; }

  t_const_value* get_value() { return value_; }

  e_req get_req() const { return req_; }

  std::map<std::string, std::string> annotations_;

 private:
  t_type* type_;
  std::string name_;
  int32_t key_{0};
  t_const_value* value_{nullptr};

  e_req req_{T_OPT_IN_REQ_OUT};
};

/**
 * A simple struct for the parser to use to store a field ID, and whether or
 * not it was specified by the user or automatically chosen.
 */
struct t_field_id {
  int64_t value;
  bool auto_assigned;
};
