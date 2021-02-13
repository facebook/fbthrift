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

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace cpp2 {

std::vector<std::string> get_gen_namespace_components(t_program const& program);

std::string get_gen_namespace(t_program const& program);

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
std::string const& get_type(const t_type* type);

/**
 * If the cpp_type is std::unique_ptr<folly::IOBuf> the C++ compiler implicitly
 * assumes this is optional.
 */
bool is_implicit_ref(const t_type* type);

/**
 * If the field has cpp.ref/cpp2.ref/cpp.ref_type/cpp2.ref_type.
 */
bool is_explicit_ref(const t_field* f);

inline bool is_ref(const t_field* f) {
  return is_explicit_ref(f) || is_implicit_ref(f->get_type());
}

/**
 * The value of cpp.ref_type/cpp2.ref_type.
 */
std::string const& get_ref_type(const t_field* f);

/**
 * If the field has cpp.ref/cpp2.ref or cpp.ref_type/cpp2.ref_type == "unique".
 */
bool is_unique_ref(const t_field* f);

template <typename Node>
const std::string& get_name(const Node* node) {
  return node->get_annotation("cpp.name", &node->get_name());
}

bool is_stack_arguments(
    std::map<std::string, std::string> const& options,
    t_function const& function);

int32_t get_split_count(std::map<std::string, std::string> const& options);

bool is_mixin(const t_field& field);

struct mixin_member {
  t_field* mixin;
  t_field* member;
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
