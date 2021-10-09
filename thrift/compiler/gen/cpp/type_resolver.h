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
#include <utility>

#include <boost/optional.hpp>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_container.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_sink.h>
#include <thrift/compiler/ast/t_stream.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/gen/cpp/detail/gen.h>
#include <thrift/compiler/gen/cpp/namespace_resolver.h>
#include <thrift/compiler/gen/cpp/reference_type.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {

// A class that resolves c++ type names from thrift types and caches the
// results.
class type_resolver {
 public:
  // Returns c++ type name for the given thrift type.
  const std::string& get_type_name(const t_type* node) {
    return detail::get_or_gen(type_cache_, node, [&]() {
      return gen_type(node, find_adapter(node));
    });
  }

  // Returns the c++ type that the runtime knows how to handle.
  const std::string& get_standard_type_name(const t_type* node) {
    return detail::get_or_gen(
        standard_type_cache_, node, [&]() { return gen_standard_type(node); });
  }

  // Checks whether a t_type could resolve to a scalar.
  //
  // A type could resolve to a scalar if t_type is
  // a scalar, or has an 'cpp.type' and 'cpp.adapter' annotation.
  // Note, the 'cpp.template' annotation only applies to container
  // types, so it could never resolve to a scalar.
  // In C++, scalars sometimes require a special treatment. For
  // example, they are not initialized unless the default
  // constructor is expicitly invoked. So if we declare
  // two variables, like:
  //   int i, j{};
  // At this point, i could be any value, as it has not been
  // explicitly initalized, while j is guarnteed to be 0, the
  // intrinsic default for an int. On the other hand, if i
  // had not been a scalar, it would have been initalized
  // implicitly.
  static bool can_resolve_to_scalar(const t_type* node);

  // Returns the c++ type that should be used to store the field's value.
  //
  // This differs from the type name, when a 'cpp.ref' or 'cpp.ref_type'
  // annotation is applied to the field.
  const std::string& get_storage_type_name(const t_field* node);

  static const std::string* find_type(const t_type* node) {
    return node->find_annotation_or_null({"cpp.type", "cpp2.type"});
  }
  static const std::string* find_adapter(const t_type* node) {
    return node->find_annotation_or_null("cpp.adapter");
  }
  static const std::string* find_first_type(const t_type* node) {
    return t_typedef::get_first_annotation_or_null(
        node, {"cpp.type", "cpp2.type"});
  }
  static const std::string* find_first_adapter(const t_type* node) {
    return t_typedef::get_first_annotation_or_null(node, {"cpp.adapter"});
  }
  static const std::string* find_first_adapter(const t_field* node);
  static const std::string* find_template(const t_type* node) {
    return node->find_annotation_or_null({"cpp.template", "cpp2.template"});
  }

 private:
  using type_resolve_fn =
      const std::string& (type_resolver::*)(const t_type* node);

  namespace_resolver namespaces_;
  std::unordered_map<const t_type*, std::string> type_cache_;
  std::unordered_map<const t_type*, std::string> standard_type_cache_;
  // TODO(afuller): Use a custom key type with a std::unordered_map (std::pair
  // does not have an std::hash specialization).
  std::map<std::pair<const t_type*, reference_type>, std::string>
      storage_type_cache_;
  std::unordered_map<const t_field*, std::string> adapter_storage_type_cache_;

  static const std::string& default_type(t_base_type::type btype);
  static const std::string& default_template(t_container::type ctype);

  const std::string& get_namespace(const t_program* program);

  // Generatating functions.
  std::string gen_type(
      const t_type* node,
      const std::string* adapter,
      boost::optional<int16_t> field_id = {});
  std::string gen_standard_type(const t_type* node);
  std::string gen_storage_type(
      const std::pair<const t_type*, reference_type>& ref_type);

  std::string gen_raw_type_name(const t_type* node, type_resolve_fn resolve_fn);
  std::string gen_container_type(
      const t_container* node, type_resolve_fn resolve_fn);
  std::string gen_sink_type(const t_sink* node, type_resolve_fn resolve_fn);
  std::string gen_stream_resp_type(
      const t_stream_response* node, type_resolve_fn resolve_fn);

  static std::string gen_adapted_type(
      const std::string& adapter,
      const std::string& standard_type,
      boost::optional<int16_t> field_id);

  const std::string& resolve(type_resolve_fn resolve_fn, const t_type* node) {
    return (this->*resolve_fn)(node);
  }
};

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
