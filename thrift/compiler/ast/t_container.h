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

#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_container : public t_type {
 public:
  /**
   * The subset of t_type::type values that are containers.
   */
  enum class type {
    t_list = int(t_type::type::t_list),
    t_set = int(t_type::type::t_set),
    t_map = int(t_type::type::t_map),
  };

  using t_type::type_name;
  static const std::string& type_name(type container_type) {
    return type_name(static_cast<t_type::type>(container_type));
  }

  t_container() = default;

  type container_type() const {
    return static_cast<type>(get_type_value());
  }

  bool is_set() const final {
    return container_type() == type::t_set;
  }

  bool is_list() const final {
    return container_type() == type::t_list;
  }

  bool is_map() const final {
    return container_type() == type::t_map;
  }

  void set_cpp_name(std::string cpp_name) {
    cpp_name_ = std::move(cpp_name);
    has_cpp_name_ = true;
  }

  bool has_cpp_name() const {
    return has_cpp_name_;
  }

  const std::string& get_cpp_name() const {
    return cpp_name_;
  }

  bool is_container() const override {
    return true;
  }

 private:
  std::string cpp_name_;
  bool has_cpp_name_ = false;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
