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

#include <initializer_list>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_container.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_stream.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/gen/cpp/namespace_resolver.h>
#include <thrift/compiler/gen/cpp/reference_type.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace cpp2 {

inline std::vector<std::string> get_gen_namespace_components(
    t_program const& program) {
  return gen::cpp::namespace_resolver::gen_namespace_components(&program);
}

inline std::string get_gen_namespace(t_program const& program) {
  return gen::cpp::namespace_resolver::gen_namespace(&program);
}

/*
 * This determines if a type can be ordered.
 * If the type is using any annotation for cpp2.type or cpp2.template
 * its not considered orderable, and we don't need to generate operator< methods
 */
bool is_orderable(
    std::unordered_set<t_type const*>& seen,
    std::unordered_map<t_type const*, bool>& memo,
    t_type const& type);
bool is_orderable(t_type const& type);

/**
 * Return the cpp.type/cpp2.type attribute or empty string if nothing set.
 */
// TODO(afuller): Replace with type_resolver::get_type_name.
std::string const& get_type(const t_type* type);

/**
 * If the cpp_type is std::unique_ptr<folly::IOBuf> the C++ compiler implicitly
 * assumes this is optional.
 */
bool is_implicit_ref(const t_type* type);

/**
 * If the field has cpp.ref/cpp2.ref/cpp.ref_type/cpp2.ref_type.
 */
// TODO(afuller): Remove by actually inlining function.
inline bool is_explicit_ref(const t_field* f) {
  return gen::cpp::find_ref_type(*f) != gen::cpp::reference_type::none;
}

inline bool is_ref(const t_field* f) {
  return is_explicit_ref(f) || is_implicit_ref(f->get_type());
}

inline bool field_has_isset(const t_field* field) {
  return field->get_req() != t_field::e_req::required &&
      !is_explicit_ref(field);
}

inline bool is_lazy(const t_field* field) {
  return field->has_annotation("cpp.experimental.lazy") ||
      field->find_structured_annotation_or_null(
          "facebook.com/thrift/annotation/cpp/Lazy") != nullptr;
}

inline bool is_lazy_ref(const t_field* field) {
  if (const t_const* anno = field->find_structured_annotation_or_null(
          "facebook.com/thrift/annotation/cpp/Lazy")) {
    for (const auto& kv : anno->value()->get_map()) {
      if (kv.first->get_string() == "ref") {
        return kv.second->get_bool();
      }
    }
  }
  return false;
}

bool field_transitively_refers_to_unique(const t_field* field);

/**
 * Determines if the operations on the C++ representation of type can be
 * constexpr and, in particular, if the move constructor and assignment
 * operator can be defined as
 *   constexpr T(T&&) = default;
 *   constexpr T& operator=(T&&) = default;
 */
class is_eligible_for_constexpr {
 public:
  bool operator()(const t_type* type);

 private:
  std::unordered_map<const t_type*, bool> cache_;
};

// Invokes f on each field of s and nested structs. The traversal is performed
// transitively in a depth-first order and interrupted if f returns false.
template <typename F>
void for_each_transitive_field(const t_struct* s, F f) {
  struct field_info {
    const t_struct* owner;
    size_t index;
  };
  auto fields = std::vector<field_info>{1, {s, 0}};
  while (!fields.empty()) {
    auto& fi = fields.back();
    if (fi.index == fi.owner->fields().size()) {
      fields.pop_back();
      continue;
    }
    const t_field* field = fi.owner->get_field(fi.index);
    if (!f(field)) {
      return;
    }
    ++fi.index;
    if (const auto* sub = dynamic_cast<const t_struct*>(field->get_type())) {
      fields.push_back({sub, 0});
    }
  }
}

/**
 * If the field has cpp.ref/cpp2.ref or cpp.ref_type/cpp2.ref_type == "unique".
 */
// TODO(afuller): Remove by actually inlining function.
inline bool is_unique_ref(const t_field* f) {
  return gen::cpp::find_ref_type(*f) == gen::cpp::reference_type::unique;
}

template <typename Node>
const std::string& get_name(const Node* node) {
  return node->get_annotation("cpp.name", &node->get_name());
}

bool is_stack_arguments(
    std::map<std::string, std::string> const& options,
    t_function const& function);

int32_t get_split_count(std::map<std::string, std::string> const& options);

// return client name -- client split count
std::unordered_map<std::string, int32_t> get_client_name_to_split_count(
    std::map<std::string, std::string> const& options);

bool is_mixin(const t_field& field);

inline const t_const* packed_isset(const t_struct& s) {
  return s.find_structured_annotation_or_null(
      "facebook.com/thrift/annotation/cpp/PackIsset");
}

bool has_ref_annotation(const t_field& f);

struct mixin_member {
  const t_field* mixin;
  const t_field* member;
};

/**
 * Returns a list of pairs of mixin and mixin's members
 * e.g. for Thrift IDL
 *
 * struct Mixin1 { 1: i32 m1; }
 * struct Mixin2 { 1: i32 m2; }
 * struct Strct {
 *   1: mixin Mixin1 f1;
 *   2: mixin Mixin2 f2;
 *   3: i32 m3;
 * }
 *
 * this returns {{.mixin="f1", .member="m1"}, {.mixin="f2", .member="m2"}}
 */
std::vector<mixin_member> get_mixins_and_members(const t_struct& strct);

//  get_gen_type_class
//  get_gen_type_class_with_indirection
//
//  Returns a string with the fully-qualified name of the C++ type class type
//  representing the given type.
//
//  The _with_indirection variant intersperses indirection_tag wherever the
//  annotation cpp.indirection appears in the corresponding definitions.
std::string get_gen_type_class(t_type const& type);
std::string get_gen_type_class_with_indirection(t_type const& type);

std::string sha256_hex(std::string const& in);

} // namespace cpp2
} // namespace compiler
} // namespace thrift
} // namespace apache
