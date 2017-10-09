/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/compiler/parse/t_scope.h>
#include <thrift/compiler/parse/t_type.h>
#include <string>

/**
 * A typedef is a mapping from a symbolic name to another type. In dymanically
 * typed languages (i.e. php/python) the code generator can actually usually
 * ignore typedefs and just use the underlying type directly, though in C++
 * the symbolic naming can be quite useful for code clarity.
 *
 */
class t_typedef : public t_type {
 public:
  t_typedef(
      t_program* program,
      t_type* type,
      const std::string& symbolic,
      t_scope* scope)
      : t_type(program, symbolic),
        type_(type),
        symbolic_(symbolic),
        scope_(scope),
        defined_(true) {}

  /**
   * This constructor is used to refer to a type that is lazily
   * resolved at a later time, like for forward declarations or
   * recursive types.
   */
  t_typedef(t_program* program, const std::string& symbolic, t_scope* scope)
      : t_type(program, symbolic),
        type_(nullptr),
        symbolic_(symbolic),
        scope_(scope),
        defined_(false) {}

  ~t_typedef() override {}

  t_type* get_type() const;

  const std::string& get_symbolic() const {
    return symbolic_;
  }

  bool is_typedef() const override { return true; }

  std::string get_full_name() const override {
    return get_type()->get_full_name();
  }

  std::string get_impl_full_name() const override {
    return get_type()->get_impl_full_name();
  }

  TypeValue get_type_value() const override {
    return get_type()->get_type_value();
  }

  uint64_t get_type_id() const override { return get_type()->get_type_id(); }

  bool is_defined() const {
    return defined_;
  }

 private:
  mutable t_type* type_;
  std::string symbolic_;
  t_scope* scope_;
  bool defined_{true};
};
