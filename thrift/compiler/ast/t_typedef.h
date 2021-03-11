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
#include <string>

#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

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
      t_type_ref type,
      std::string symbolic,
      t_scope* scope)
      : t_type(program, symbolic),
        type_(std::move(type)),
        symbolic_(std::move(symbolic)),
        scope_(scope),
        defined_(true) {}

  /**
   * This constructor is used to refer to a type that is lazily
   * resolved at a later time, like for forward declarations or
   * recursive types.
   */
  t_typedef(t_program* program, const std::string& symbolic, t_scope* scope)
      : t_type(program, symbolic),
        symbolic_(symbolic),
        scope_(scope),
        defined_(false) {}

  /**
   * For placeholder typedef only, resolve and find the actual type that the
   * symbolic name refers to. Return true iff the type exists in the scope.
   */
  bool resolve_placeholder();

  const t_type* get_type() const {
    return type_.get_type();
  }

  const std::string& get_symbolic() const {
    return symbolic_;
  }

  bool is_typedef() const override {
    return true;
  }

  std::string get_full_name() const override {
    return get_type()->get_full_name();
  }

  type get_type_value() const override {
    return get_type()->get_type_value();
  }

  uint64_t get_type_id() const override {
    return get_type()->get_type_id();
  }

  bool is_defined() const {
    return defined_;
  }

  // Returns the first type, in the typedef type hierarchy, matching the
  // given predicate or nullptr.
  template <typename UnaryPredicate>
  static const t_type* find_type_if(const t_type* type, UnaryPredicate&& pred) {
    while (true) {
      if (pred(type)) {
        return type;
      }
      if (const auto* as_typedef = dynamic_cast<const t_typedef*>(type)) {
        type = as_typedef->get_type();
      } else {
        return nullptr;
      }
    };
  }

  // Finds the first matching annoation in the typdef's type hierarchy.
  // Return null if not found.
  static const std::string* get_first_annotation_or_null(
      const t_type* type,
      alias_span name);

  // Finds the first matching annoation in the typdef's type heiarchy.
  // Return default_value or "" if not found.
  template <typename D = const std::string*>
  static auto get_first_annotation(
      const t_type* type,
      alias_span name,
      D&& default_value = nullptr) {
    return annotation_or(
        get_first_annotation_or_null(type, name),
        std::forward<D>(default_value));
  }

 private:
  t_type_ref type_;
  std::string symbolic_;
  t_scope* scope_;
  bool defined_{true};

 public:
  // TODO(afuller): Remove everything below here, as it is just provided for
  // backwards compatibility.

  t_typedef(
      t_program* program,
      const t_type* type,
      std::string symbolic,
      t_scope* scope)
      : t_typedef(program, t_type_ref(type), std::move(symbolic), scope) {}
};

} // namespace compiler
} // namespace thrift
} // namespace apache
