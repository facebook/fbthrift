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

#pragma once

#include <memory>
#include <vector>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * An enumerated type. A list of constant objects with a name for the type.
 *
 */
class t_enum : public t_type {
 public:
  explicit t_enum(t_program* program) : t_type(program) {}

  void set_name(const std::string& name) override {
    name_ = name;
  }

  void append(
      std::unique_ptr<t_enum_value> enum_value,
      std::unique_ptr<t_const> constant) {
    enum_values_raw_.push_back(enum_value.get());
    enum_values_.push_back(std::move(enum_value));

    constants_.push_back(std::move(constant));
  }

  const std::vector<t_enum_value*>& get_enum_values() const {
    return enum_values_raw_;
  }

  const t_enum_value* find_value(const int32_t enum_value) const {
    for (auto const& it : enum_values_) {
      if (it->get_value() == enum_value) {
        return it.get();
      }
    }
    return nullptr;
  }

  bool is_enum() const override {
    return true;
  }

  std::string get_full_name() const override {
    return make_full_name("enum");
  }

  std::string get_impl_full_name() const override {
    return make_full_name("enum");
  }

  TypeValue get_type_value() const override {
    return TypeValue::TYPE_ENUM;
  }

 private:
  std::vector<std::unique_ptr<t_enum_value>> enum_values_;
  std::vector<std::unique_ptr<t_const>> constants_;

  std::vector<t_enum_value*> enum_values_raw_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
