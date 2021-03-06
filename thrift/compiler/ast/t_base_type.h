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

#ifndef T_BASE_TYPE_H
#define T_BASE_TYPE_H

#include <cstdlib>
#include <vector>

#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * A thrift base type, which must be one of the defined enumerated types inside
 * this definition.
 *
 */
class t_base_type : public t_type {
 public:
  /**
   * Enumeration of thrift base types
   */
  enum t_base {
    TYPE_VOID = int(type::t_void),
    TYPE_STRING = int(type::t_string),
    TYPE_BOOL = int(type::t_bool),
    TYPE_BYTE = int(type::t_byte),
    TYPE_I16 = int(type::t_i16),
    TYPE_I32 = int(type::t_i32),
    TYPE_I64 = int(type::t_i64),
    TYPE_DOUBLE = int(type::t_double),
    TYPE_FLOAT = int(type::t_float),
    TYPE_BINARY = int(type::t_binary),
  };

  static const t_base_type& t_void();
  static const t_base_type& t_string();
  static const t_base_type& t_binary();
  static const t_base_type& t_bool();
  static const t_base_type& t_byte();
  static const t_base_type& t_i16();
  static const t_base_type& t_i32();
  static const t_base_type& t_i64();
  static const t_base_type& t_double();
  static const t_base_type& t_float();

  // TODO(afuller): Disable copy constructor, and use
  // 'anonymous' typdefs instead.
  // t_base_type(const t_base_type&) = delete;

  t_base get_base() const {
    return base_;
  }

  bool is_void() const override {
    return base_ == TYPE_VOID;
  }

  bool is_string() const override {
    return base_ == TYPE_STRING;
  }

  bool is_bool() const override {
    return base_ == TYPE_BOOL;
  }

  bool is_byte() const override {
    return base_ == TYPE_BYTE;
  }

  bool is_i16() const override {
    return base_ == TYPE_I16;
  }

  bool is_i32() const override {
    return base_ == TYPE_I32;
  }

  bool is_i64() const override {
    return base_ == TYPE_I64;
  }

  bool is_float() const override {
    return base_ == TYPE_FLOAT;
  }

  bool is_double() const override {
    return base_ == TYPE_DOUBLE;
  }

  bool is_binary() const override {
    return base_ == TYPE_BINARY;
  }

  bool is_base_type() const override {
    return true;
  }

  static std::string t_base_name(t_base tbase) {
    switch (tbase) {
      case TYPE_VOID:
        return "void";
      case TYPE_STRING:
        return "string";
      case TYPE_BOOL:
        return "bool";
      case TYPE_BYTE:
        return "byte";
      case TYPE_I16:
        return "i16";
      case TYPE_I32:
        return "i32";
      case TYPE_I64:
        return "i64";
      case TYPE_DOUBLE:
        return "double";
      case TYPE_FLOAT:
        return "float";
      case TYPE_BINARY:
        return "binary";
      default:
        return "(unknown)";
    }
  }

  std::string get_full_name() const override {
    return t_base_name(base_);
  }

  type get_type_value() const override {
    return static_cast<type>(base_);
  }

  uint64_t get_type_id() const override {
    return base_;
  }

 private:
  t_base base_;

  t_base_type(std::string name, t_base base) : t_type(name), base_(base) {}
};

} // namespace compiler
} // namespace thrift
} // namespace apache

#endif
