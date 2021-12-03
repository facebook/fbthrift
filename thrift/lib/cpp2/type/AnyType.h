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

#include <type_traits>

#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/detail/AnyType.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {

// A class that can represent any concrete Thrift type.
//
// Can be constructed directly from ThriftType tags:
//
//   auto listOfStringType1 = AnyType::create<list<string_t>>();
//
// Or built up from other type values:
//
//   auto stringType = AnyType::create<string_t>();
//   auto listOfStringType2 = AnyType::create<list_c>(stringType);
//
// Both of these result in the same type:
//
//   listOfStringType1 == listOfStringType2 -> true
//
// TODO(afuller): Support extracting the type name (e.g. thrift.uri) from
// concrete named types (i.e. enum_t, struct_t, union_t, exception_t).
// See thrift/conformance/cpp2/ThriftTypeInfo.h
class AnyType {
 public:
  constexpr AnyType() noexcept = default; // The 'void' type.

  AnyType(const AnyType&) noexcept = default;
  AnyType(AnyType&&) noexcept = default;

  template <typename Tag, typename = std::enable_if_t<is_concrete_v<Tag>>>
  /* implicit */ AnyType(Tag) noexcept
      : type_(detail::AnyTypeHelper<Tag>::make_type()) {}

  // Named types.
  AnyType(enum_c t, std::string name)
      : type_{detail::make_named(t, checkName(std::move(name)))} {}
  AnyType(struct_c t, std::string name)
      : type_{detail::make_named(t, checkName(std::move(name)))} {}
  AnyType(union_c t, std::string name)
      : type_{detail::make_named(t, checkName(std::move(name)))} {}
  AnyType(exception_c t, std::string name)
      : type_{detail::make_named(t, checkName(std::move(name)))} {}

  // Parameterized types.
  AnyType(list_c t, AnyType valueType) noexcept
      : type_{detail::make_parameterized(t, std::move(valueType))} {}
  AnyType(set_c t, AnyType keyType) noexcept
      : type_{detail::make_parameterized(t, std::move(keyType))} {}
  AnyType(map_c t, AnyType keyType, AnyType valueType) noexcept
      : type_{detail::make_parameterized(
            t, std::move(keyType), std::move(valueType))} {}

  // Constructs an AnyType for the given Thrift type Tag, using the given
  // arguments. If the type Tag is not concrete, the additional parameters must
  // be passed in. For example:
  //
  //   AnyType::create<list<i32>>() ==
  //       AnyType::create<list_c>(AnyType::create<i32>());
  //
  //   //Create the type for an IDL-defined, named struct.
  //   AnyType::create<struct_c>("mydomain.com/my/package/MyStruct");
  //
  template <typename Tag, typename... Args>
  static AnyType create(Args&&... args) {
    return {Tag{}, std::forward<Args>(args)...};
  }

  constexpr BaseType base_type() const noexcept {
    return static_cast<BaseType>(type_.index());
  }

  AnyType& operator=(const AnyType&) noexcept = default;
  AnyType& operator=(AnyType&&) noexcept = default;

 private:
  explicit AnyType(detail::AnyTypeHolder&& type) : type_(std::move(type)) {}

  template <typename Tag>
  friend struct detail::AnyTypeHelper;

  friend bool operator==(AnyType lhs, AnyType rhs) noexcept {
    return lhs.type_ == rhs.type_;
  }
  friend bool operator!=(AnyType lhs, AnyType rhs) noexcept {
    return !(lhs.type_ == rhs.type_);
  }

  detail::AnyTypeHolder type_;

  static std::string&& checkName(std::string&& name);
};

namespace detail {

template <typename ValTag, template <typename...> typename ListT>
struct AnyTypeHelper<list<ValTag, ListT>> {
  static AnyTypeHolder make_type() {
    return {make_parameterized(
        list_c{}, AnyType(AnyTypeHelper<ValTag>::make_type()))};
  }
};

template <typename KeyTag, template <typename...> typename SetT>
struct AnyTypeHelper<set<KeyTag, SetT>> {
  static AnyTypeHolder make_type() {
    return {make_parameterized(
        set_c{}, AnyType(AnyTypeHelper<KeyTag>::make_type()))};
  }
};

template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct AnyTypeHelper<map<KeyTag, ValTag, MapT>> {
  static AnyTypeHolder make_type() {
    return {make_parameterized(
        map_c{},
        AnyType(AnyTypeHelper<KeyTag>::make_type()),
        AnyType(AnyTypeHelper<ValTag>::make_type()))};
  }
};

} // namespace detail
} // namespace apache::thrift::type
