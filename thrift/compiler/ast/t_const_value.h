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

#ifndef T_CONST_VALUE_H
#define T_CONST_VALUE_H

#include <stdint.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <thrift/compiler/ast/t_doc.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_const;
class t_enum;
class t_enum_value;
class t_type;

/**
 * A const value is something parsed that could be a map, set, list, struct
 * or whatever.
 *
 */
class t_const_value {
 public:
  enum t_const_value_type {
    CV_BOOL,
    CV_INTEGER,
    CV_DOUBLE,
    CV_STRING,
    CV_MAP,
    CV_LIST
  };

  t_const_value() {}

  explicit t_const_value(int64_t val) {
    set_integer(val);
  }

  explicit t_const_value(std::string val) {
    set_string(val);
  }

  t_const_value(const t_const_value&) = delete;
  t_const_value(t_const_value&&) = delete;
  t_const_value& operator=(const t_const_value&) = delete;

  std::unique_ptr<t_const_value> clone() const;

  void assign(t_const_value&& value) {
    *this = std::move(value);
  }

  void set_string(std::string val) {
    valType_ = CV_STRING;
    stringVal_ = val;
  }

  const std::string& get_string() const {
    check_val_type({CV_STRING});
    return stringVal_;
  }

  void set_integer(int64_t val) {
    valType_ = CV_INTEGER;
    intVal_ = val;
  }

  int64_t get_integer() const {
    check_val_type({CV_INTEGER, CV_BOOL});
    return intVal_;
  }

  void set_double(double val) {
    valType_ = CV_DOUBLE;
    doubleVal_ = val;
  }

  double get_double() const {
    check_val_type({CV_INTEGER, CV_DOUBLE});
    return doubleVal_;
  }

  void set_bool(bool val) {
    valType_ = CV_BOOL;
    boolVal_ = val;
    // Added to support backward compatibility with generators that
    // look for the integer value to determine the boolean value
    intVal_ = val;
  }

  bool get_bool() const {
    check_val_type({CV_BOOL});
    return boolVal_;
  }

  void set_map() {
    valType_ = CV_MAP;
  }

  void add_map(
      std::unique_ptr<t_const_value> key,
      std::unique_ptr<t_const_value> val) {
    mapVal_raw_.emplace_back(key.get(), val.get());
    mapVal_.emplace_back(std::move(key), std::move(val));
  }

  const std::vector<std::pair<t_const_value*, t_const_value*>>& get_map()
      const {
    return mapVal_raw_;
  }

  void set_list() {
    valType_ = CV_LIST;
  }

  void add_list(std::unique_ptr<t_const_value> val) {
    listVal_raw_.push_back(val.get());
    listVal_.push_back(std::move(val));
  }

  const std::vector<t_const_value*>& get_list() const {
    return listVal_raw_;
  }

  t_const_value_type get_type() const {
    return valType_;
  }

  bool is_empty() const;

  void set_owner(t_const* owner) {
    owner_ = owner;
  }

  t_const* get_owner() const {
    return owner_;
  }

  void set_ttype(t_type* type) {
    type_ = type;
  }

  t_type* get_ttype() const {
    return type_;
  }

  void set_is_enum(bool value = true) {
    is_enum_ = value;
  }

  bool is_enum() const {
    return is_enum_;
  }

  void set_enum(t_enum const* tenum) {
    tenum_ = tenum;
  }

  const t_enum* get_enum() const {
    return tenum_;
  }

  void set_enum_value(t_enum_value const* tenum_val) {
    tenum_val_ = tenum_val;
  }

  const t_enum_value* get_enum_value() const {
    return tenum_val_;
  }

 private:
  void check_val_type(std::initializer_list<t_const_value_type> types) const;

  // Use a vector of pairs to store the contents of the map so that we
  // preserve thrift-file ordering when generating per-language source.
  std::vector<
      std::pair<std::unique_ptr<t_const_value>, std::unique_ptr<t_const_value>>>
      mapVal_;
  std::vector<std::unique_ptr<t_const_value>> listVal_;
  std::string stringVal_;
  bool boolVal_ = false;
  int64_t intVal_ = 0;
  double doubleVal_ = 0.0;

  std::vector<std::pair<t_const_value*, t_const_value*>> mapVal_raw_;
  std::vector<t_const_value*> listVal_raw_;

  t_const_value_type valType_ = CV_BOOL;
  t_const* owner_ = nullptr;
  t_type* type_ = nullptr;

  bool is_enum_ = false;
  t_enum const* tenum_ = nullptr;
  t_enum_value const* tenum_val_ = nullptr;

  t_const_value& operator=(t_const_value&&) = default;
};

} // namespace compiler
} // namespace thrift
} // namespace apache

#endif
