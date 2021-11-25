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

#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache::thrift::type {
class AnyType;

namespace detail {

// A concrete type.
template <typename Tag>
struct ConcreteType {
 private:
  friend constexpr bool operator==(const ConcreteType&, const ConcreteType&) {
    return true; // Always equal to itself.
  }
};

// An IDL-defined, named type.
template <typename CTag>
struct NamedType {
  std::string name;

 private:
  friend bool operator==(const NamedType& lhs, const NamedType& rhs) {
    return lhs.name == rhs.name;
  }
};

template <typename CTag>
NamedType<CTag> make_named(CTag, std::string&& name) noexcept {
  return {std::move(name)};
}

// A paramaterized type (a.k.a template, generic, what ever you want to call
// it), with N type parameters
//
// CTag is the type class for the parametrized type (list_c, set_c, etc).
//
// Since this is used to hold a dynamic type, all the parameter types are
// stored using an AnyType.
template <typename CTag, std::size_t N>
struct ParamType {
  // TODO(afuller): ParamType has to be copyable and AnyType has to be 'boxed'
  // because it can store a ParamType, but it doesn't need to be shared.
  // Consider using a `boxed` smart pointer instead (e.g. non-nullable and
  // copyable with value semantics).
  std::shared_ptr<const std::array<AnyType, N>> params;
};

// Note: This must be defined outside of ParamType, because
// both AnyType and operator==(AnyType,AnyType) are not defined yet.
template <typename CTag, size_t N>
bool operator==(const ParamType<CTag, N>& lhs, const ParamType<CTag, N>& rhs) {
  return *lhs.params == *rhs.params;
}

template <typename CTag, typename... AnyTypes>
constexpr ParamType<CTag, sizeof...(AnyTypes)> make_parameterized(
    CTag, AnyTypes&&... types) noexcept {
  using DataType = std::array<AnyType, sizeof...(AnyTypes)>;
  return {std::make_shared<const DataType>(
      DataType{std::forward<AnyTypes>(types)...})};
}

using AnyTypeData = std::variant<
    ConcreteType<void_t>,
    ConcreteType<bool_t>,
    ConcreteType<byte_t>,
    ConcreteType<i16_t>,
    ConcreteType<i32_t>,
    ConcreteType<i64_t>,
    ConcreteType<float_t>,
    ConcreteType<double_t>,
    ConcreteType<string_t>,
    ConcreteType<binary_t>,
    NamedType<enum_c>,
    NamedType<struct_c>,
    NamedType<union_c>,
    NamedType<exception_c>,
    ParamType<list_c, 1>,
    ParamType<set_c, 1>,
    ParamType<map_c, 2>>;

// Give the compiler a better typename, to make error messages sane.
struct AnyTypeHolder : AnyTypeData {
  using AnyTypeData::AnyTypeData;
};

template <typename Tag>
struct AnyTypeHelper {
  static AnyTypeHolder make_type() { return ConcreteType<Tag>{}; }
};

// Skip through adapters.
template <typename Adapter, typename Tag>
struct AnyTypeHelper<adapted<Adapter, Tag>> : AnyTypeHelper<Tag> {};

} // namespace detail
} // namespace apache::thrift::type
