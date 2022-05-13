/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/op/Hash.h>
#include <thrift/lib/cpp2/type/BaseType.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/detail/Wrap.h>
#include <thrift/lib/thrift/gen-cpp2/type_rep_types.h>
#include <thrift/lib/thrift/gen-cpp2/type_rep_types_custom_protocol.h>

namespace apache {
namespace thrift {
namespace type {

// A class that can represent any concrete Thrift type.
//
// Can be constructed directly from ThriftType tags:
//
//   auto listOfStringType1 = Type::create<list<string_t>>();
//
// Or built up from other type values:
//
//   auto stringType = Type::create<string_t>();
//   auto listOfStringType2 = Type::create<list_c>(stringType);
//
// Both of these result in the same type:
//
//   listOfStringType1 == listOfStringType2 -> true
//
class Type : public detail::Wrap<TypeStruct> {
  using Base = detail::Wrap<TypeStruct>;

 public:
  Type() = default; // The 'void' type.
  Type(const Type&) = default;
  Type(Type&&) noexcept = default;

  explicit Type(const TypeStruct& type) : Base(type) {}
  explicit Type(TypeStruct&& type) noexcept : Base(std::move(type)) {}

  template <typename Tag, typename = std::enable_if_t<is_concrete_v<Tag>>>
  /* implicit */ Type(Tag) : Base(makeType<Tag>(Tag{})) {}

  // Named types.
  Type(enum_c, std::string name) : Type{makeNamed<enum_c>(std::move(name))} {}
  Type(struct_c, std::string name)
      : Type{makeNamed<struct_c>(std::move(name))} {}
  Type(union_c, std::string name) : Type{makeNamed<union_c>(std::move(name))} {}
  Type(exception_c, std::string name)
      : Type{makeNamed<exception_c>(std::move(name))} {}

  // Parameterized types.
  Type(list_c, Type val) : Type(makeParamed<list_c>(std::move(val.data_))) {}
  Type(set_c, Type key) : Type(makeParamed<set_c>(std::move(key.data_))) {}
  Type(map_c, Type key, Type val)
      : Type(makeParamed<map_c>(std::move(key.data_), std::move(val.data_))) {}

  // Constructs an Type for the given Thrift type Tag, using the given
  // arguments. If the type Tag is not concrete, the additional parameters must
  // be passed in. For example:
  //
  //   Type::create<list<i32>>() ==
  //       Type::create<list_c>(Type::create<i32>());
  //
  //   //Create the type for an IDL-defined, named struct.
  //   Type::create<struct_c>("mydomain.com/my/package/MyStruct");
  //
  template <typename Tag, typename... Args>
  static Type create(Args&&... args) {
    return {Tag{}, std::forward<Args>(args)...};
  }

  BaseType base_type() const noexcept {
    return static_cast<BaseType>(data_.id()->getType());
  }

  Type& operator=(const Type&) = default;
  Type& operator=(Type&&) noexcept = default;

 private:
  template <typename Tag>
  static const std::string& getUri() {
    return ::apache::thrift::uri<standard_type<Tag>>();
  }

  friend bool operator==(Type lhs, Type rhs) noexcept {
    return lhs.data_ == rhs.data_;
  }
  friend bool operator!=(Type lhs, Type rhs) noexcept {
    return lhs.data_ != rhs.data_;
  }

  static void checkName(const std::string& name);

  template <typename CTag, typename T>
  static decltype(auto) getId(T&& result) {
    constexpr auto id = static_cast<FieldId>(base_type_v<CTag>);
    return op::getById<type::union_t<T>, id>(*std::forward<T>(result).id());
  }

  template <typename Tag>
  static TypeStruct makeConcrete() {
    TypeStruct result;
    getId<Tag>(result).ensure();
    return result;
  }

  template <typename CTag>
  static TypeStruct makeNamed(std::string uri) {
    TypeStruct result;
    checkName(uri);
    getId<CTag>(result).ensure().uri_ref() = std::move(uri);
    return result;
  }

  template <typename CTag, typename... TArgs>
  static TypeStruct makeParamed(TArgs&&... paramType) {
    TypeStruct result;
    getId<CTag>(result).ensure();
    result.params()->insert(result.params()->end(), {paramType...});
    return result;
  }

  template <typename Tag>
  static TypeStruct makeType(all_c) {
    return makeConcrete<Tag>();
  }
  template <typename Tag>
  static TypeStruct makeType(void_t) {
    return {};
  }
  template <typename Tag>
  static TypeStruct makeType(structured_c) {
    return makeNamed<Tag>(getUri<Tag>());
  }
  template <typename Tag>
  static TypeStruct makeType(enum_c) {
    return makeNamed<Tag>(getUri<Tag>());
  }
  template <typename Tag>
  struct Helper;
  template <typename Tag>
  static TypeStruct makeType(container_c) {
    return Helper<Tag>::makeType();
  }

  template <typename CTag, typename... PTags>
  struct ParamedTypeHelper {
    static TypeStruct makeType() {
      return makeParamed<CTag>(Type::makeType<PTags>(PTags{})...);
    }
  };
  template <typename ValTag>
  struct Helper<list<ValTag>> : ParamedTypeHelper<list_c, ValTag> {};
  template <typename KeyTag>
  struct Helper<set<KeyTag>> : ParamedTypeHelper<set_c, KeyTag> {};
  template <typename KeyTag, typename ValTag>
  struct Helper<map<KeyTag, ValTag>>
      : ParamedTypeHelper<map_c, KeyTag, ValTag> {};

  // Skip through adapters, cpp_type, etc.
  template <typename Adapter, typename Tag>
  struct Helper<adapted<Adapter, Tag>> : Helper<Tag> {};
  template <typename T, typename Tag>
  struct Helper<cpp_type<T, Tag>> : Helper<Tag> {};
};

} // namespace type
} // namespace thrift
} // namespace apache

// The custom specialization of std::hash can be injected in namespace std.
namespace std {
template <>
struct hash<apache::thrift::type::Type> {
  using Type = apache::thrift::type::Type;
  size_t operator()(const Type& type) const noexcept {
    return apache::thrift::op::hash<Type::underlying_tag>(type.toThrift());
  }
};
} // namespace std
