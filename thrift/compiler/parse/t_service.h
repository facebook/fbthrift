/*
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

#ifndef T_SERVICE_H
#define T_SERVICE_H

#include <thrift/compiler/parse/t_function.h>
#include <vector>
#include <algorithm>

class t_program;

/**
 * A service consists of a set of functions.
 *
 */
class t_service : public t_type {
 public:
  explicit t_service(t_program* program) :
    t_type(program),
    extends_(nullptr) {}

  bool is_service() const override { return true; }

  void set_extends(t_service* extends) {
    extends_ = extends;
  }

  void add_function(t_function* func) {
    _throw_if_function_duplicate(func);
    functions_.push_back(func);
  }

  const std::vector<t_function*>& get_functions() const {
    return functions_;
  }

  t_service* get_extends() const {
    return extends_;
  }

  /**
   * Throws an exception if the parameter function is already in the vector
   *
   * @param t_function* Function struct pointer
   */
  void _throw_if_function_duplicate(t_function* func) {
    if (std::any_of(
            functions_.begin(), functions_.end(), [func](t_function* other) {
              return func->get_name() == other->get_name();
            })) {
      throw std::runtime_error(
          "Duplicate function defined (" + func->get_name() +
          "). Thrift's wire protocol does not support this.");
    }
  }

  TypeValue get_type_value() const override { return t_types::TYPE_SERVICE; }

  std::string get_full_name() const override {
    return make_full_name("service");
  }

  std::string get_impl_full_name() const override {
    return make_full_name("service");
  }

 private:
  std::vector<t_function*> functions_;
  t_service* extends_;
};

#endif
