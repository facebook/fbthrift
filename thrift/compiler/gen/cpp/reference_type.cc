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

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/gen/cpp/reference_type.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {

namespace {

const std::string* find_ref_type_annot(const t_node& node) {
  return node.find_annotation_or_null({"cpp.ref_type", "cpp2.ref_type"});
}

// Since we can not include `thrift/annotation/cpp.thrift`
// This is a copy of apache::thrift::annotation::RefType
enum class RefType {
  Unique = 0,
  Shared = 1,
  SharedMutable = 2,
};

} // namespace

reference_type find_ref_type(const t_field& node) {
  if (node.has_annotation({"cpp.box", "thrift.box"})) {
    return reference_type::boxed;
  }

  // Look for a specific ref type annotation.
  if (const std::string* ref_type = find_ref_type_annot(node)) {
    if (*ref_type == "unique") {
      return reference_type::unique;
    } else if (*ref_type == "shared") {
      // TODO(afuller): The 'default' should be const, as mutable shared
      // pointers are 'dangerous' if the pointee is not thread safe.
      return reference_type::shared_mutable;
    } else if (*ref_type == "shared_const") {
      return reference_type::shared_const;
    } else if (*ref_type == "shared_mutable") {
      return reference_type::shared_mutable;
    } else {
      return reference_type::unrecognized;
    }
  }

  // Look for the generic annotations, which implies unique.
  if (node.has_annotation({"cpp.ref", "cpp2.ref"})) {
    // TODO(afuller): Seems like this should really be a 'boxed' reference type
    // (e.g. a deep copy smart pointer) by default, so both recursion and copy
    // constructors would work. Maybe that would let us also remove most or all
    // uses of 'shared' (which can lead to reference cycles).
    return reference_type::unique;
  }

  if (const t_const* anno = node.find_structured_annotation_or_null(
          "facebook.com/thrift/annotation/cpp/Ref")) {
    for (const auto& kv : anno->value()->get_map()) {
      if (kv.first->get_string() == "type") {
        switch (static_cast<RefType>(kv.second->get_integer())) {
          case RefType::Unique:
            return reference_type::unique;
          case RefType::Shared:
            return reference_type::shared_const;
          case RefType::SharedMutable:
            return reference_type::shared_mutable;
          default:
            return reference_type::unrecognized;
        }
      }
    }

    // TODO: Consider having default reference type
    throw std::runtime_error("cpp.Ref with unspecified type");
  }

  return reference_type::none;
}

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
