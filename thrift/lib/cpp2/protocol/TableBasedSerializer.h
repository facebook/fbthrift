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

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <folly/CPortability.h>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>

namespace apache {
namespace thrift {
namespace detail {

using FieldID = std::int16_t;
using VoidFuncPtr = void (*)(void*);
// MSVC cannot
// reinterpret_cast an overloaded function to another function pointer,
// but piping the function through an identity function before reinterpret_cast
// works.
template <typename T>
FOLLY_ALWAYS_INLINE constexpr T identity(T t) {
  return t;
}

template <typename T>
struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
using is_smart_ptr =
    folly::bool_constant<is_unique_ptr<T>::value || is_shared_ptr<T>::value>;

template <typename T, typename U = void>
using enable_if_smart_ptr_t = std::enable_if_t<is_smart_ptr<T>::value, U>;

template <typename T, typename U = void>
using enable_if_not_smart_ptr_t = std::enable_if_t<!is_smart_ptr<T>::value, U>;

template <typename T, typename Enable = void>
struct maybe_get_element_type {
  using type = T;
};

template <typename T>
struct maybe_get_element_type<T, enable_if_smart_ptr_t<T>>
    : maybe_get_element_type<typename T::element_type> {};

template <typename T>
using maybe_get_element_type_t = typename maybe_get_element_type<T>::type;

enum class StringFieldType {
  IOBuf,
  IOBufPtr,
  String,
};

struct TypeInfo {
  protocol::TType type;

  // A function to set an object of a specific type, so deserialization logic
  // can modify or initialize the object accordingly.
  // This function helps us support cpp.type for primitive fields.
  // It should take a Thrift object pointer and optionally the value to set.
  // For container types, the function is the initialization function to clear
  // the container before deserializing into the container.
  VoidFuncPtr set;

  // For primitive number types (float, i64, ...):
  // A function to read an object pointer and return the Thrift type for
  // serializing thrift.
  // The function casts the object pointer to the object's type, optionally
  // deref the smart pointer if applicable, and construct the underlying thrift
  // type from the object. The signature is:
  //
  // UnderlyingThriftType (*)(const void* object)
  //
  // Where UnderlyingThriftType is the type that Thrift write functions can take
  // as parameters (writeBinary, writeI64, ...).
  //
  // For others (string, struct, ...):
  // A field may get annotated with cpp.ref_type = "unique".
  // In that case we need to deref the smart pointer
  // before we get to the actual address of the object.
  VoidFuncPtr get;
  // A pointer to additional type information, e.g. `MapFieldExt` for a map.
  const void* typeExt;
};

struct FieldInfo {
  // Field id in thrift definition.
  FieldID id;
  const char* name;
  // Offset into the data member of the field in the struct.
  ptrdiff_t memberOffset;

  // 0 means that the field does not have __isset.
  ptrdiff_t issetOffset;

  const TypeInfo* typeInfo;
};

struct UnionExt {
  // Clear union before setting a field.
  VoidFuncPtr clear;

  ptrdiff_t unionTypeOffset;

  // Value initializes using placement new into the member.
  // Generated code should order this list by fields key order.
  VoidFuncPtr initMember[];
};

// Templatized version to const initialize with the exact array length.
template <std::int16_t NumFields>
struct UnionExtN {
  VoidFuncPtr clear;
  ptrdiff_t unionTypeOffset;
  VoidFuncPtr initMember[NumFields];
};

struct StructInfo {
  std::int16_t numFields;
  const char* name;
  // This should be set to nullptr when not a union.
  const UnionExt* unionExt = nullptr;
  FieldInfo fieldInfos[];
};

// Templatized version to const initialize with the exact array length.
template <std::int16_t NumFields>
struct StructInfoN {
  std::int16_t numFields = NumFields;
  const char* name;
  const void* unionExt = nullptr;
  FieldInfo fieldInfos[NumFields];
};

struct MapFieldExt {
  const TypeInfo* keyInfo;
  const TypeInfo* valInfo;
  std::uint32_t (*size)(const void* object);
  void (*consumeElem)(
      const void* context,
      void* object,
      void (*keyReader)(const void* context, void* key),
      void (*valueReader)(const void* context, void* val));
  void (*readMap)(
      const void* context,
      void* object,
      std::uint32_t mapSize,
      void (*keyReader)(const void* context, void* key),
      void (*valueReader)(const void* context, void* val));
  size_t (*writeMap)(
      const void* context,
      const void* object,
      bool protocolSortKeys,
      size_t (*writer)(
          const void* context,
          const void* keyElem,
          const void* valueElem));
};

struct ListFieldExt {
  const TypeInfo* valInfo;
  std::uint32_t (*size)(const void* object);
  void (*consumeElem)(
      const void* context,
      void* object,
      void (*reader)(const void* context, void* val));
  void (*readList)(
      const void* context,
      void* object,
      std::uint32_t listSize,
      void (*reader)(const void* context, void* val));
  size_t (*writeList)(
      const void* context,
      const void* val,
      size_t (*writer)(const void* context, const void* val));
};

struct SetFieldExt {
  const TypeInfo* valInfo;
  std::uint32_t (*size)(const void* object);
  void (*consumeElem)(
      const void* context,
      void* object,
      void (*reader)(const void* context, void* val));
  void (*readSet)(
      const void* context,
      void* object,
      std::uint32_t setSize,
      void (*reader)(const void* context, void* val));
  size_t (*writeSet)(
      const void* context,
      const void* object,
      bool protocolSortKeys,
      size_t (*writer)(const void* context, const void* val));
};

template <typename Type>
enable_if_not_smart_ptr_t<Type> initialize(void* object) {
  *static_cast<Type*>(object) = Type();
}

template <typename Type>
std::enable_if_t<is_shared_ptr<Type>::value> initialize(void* object) {
  *static_cast<Type*>(object) = std::make_shared<typename Type::element_type>();
}

template <typename Type>
std::enable_if_t<is_unique_ptr<Type>::value> initialize(void* object) {
  *static_cast<Type*>(object) = std::make_unique<typename Type::element_type>();
}

template <typename SmartPtr>
const void* derefSmartPointer(const void* object) {
  return static_cast<const SmartPtr*>(object)->get();
}

template <typename ReturnType, typename ObjectType>
enable_if_not_smart_ptr_t<ObjectType, ReturnType> get(const void* object) {
  return static_cast<ReturnType>(*static_cast<const ObjectType*>(object));
}

template <typename ReturnType, typename SmartPtr>
enable_if_smart_ptr_t<SmartPtr, ReturnType> get(const void* object) {
  return static_cast<ReturnType>(**static_cast<const SmartPtr*>(object));
}

template <typename ObjectType, typename ValueType>
enable_if_not_smart_ptr_t<ObjectType> set(void* object, const ValueType& val) {
  *static_cast<ObjectType*>(object) = static_cast<const ObjectType&>(val);
}

template <typename SmartPtr, typename ValueType>
std::enable_if_t<is_unique_ptr<SmartPtr>::value> set(
    void* object,
    const ValueType& val) {
  using Element = typename SmartPtr::element_type;
  *static_cast<SmartPtr*>(object) =
      std::make_unique<Element>(static_cast<Element>(val));
}

template <typename SmartPtr, typename ValueType>
std::enable_if_t<is_shared_ptr<SmartPtr>::value> set(
    void* object,
    const ValueType& val) {
  using Element = typename SmartPtr::element_type;
  *static_cast<SmartPtr*>(object) =
      std::make_shared<Element>(static_cast<Element>(val));
}

template <typename ValueType>
enable_if_not_smart_ptr_t<ValueType> placementNewUnionValue(void* object) {
  ::new (object) ValueType();
}

template <typename SmartPtr>
enable_if_smart_ptr_t<SmartPtr> placementNewUnionValue(void* object) {
  ::new (object) SmartPtr(new typename SmartPtr::element_type());
}

template <typename List>
size_t writeList(
    const void* context,
    const void* object,
    size_t (*writer)(const void* /*context*/, const void* /*val*/)) {
  const List& out = *static_cast<const List*>(object);
  size_t written = 0;
  for (auto& elem : out) {
    written += writer(context, &elem);
  }
  return written;
}

template <typename Set>
size_t writeSet(
    const void* context,
    const void* object,
    bool protocolSortKeys,
    size_t (*writer)(const void* /*context*/, const void* /*val*/)) {
  const Set& out = *static_cast<const Set*>(object);
  size_t written = 0;

  if (!folly::is_detected_v<
          ::apache::thrift::detail::pm::detect_key_compare,
          Set> &&
      protocolSortKeys) {
    std::vector<typename Set::const_iterator> iters;
    iters.reserve(out.size());
    for (auto it = out.begin(); it != out.end(); ++it) {
      iters.push_back(it);
    }
    std::sort(
        iters.begin(), iters.end(), [](auto a, auto b) { return *a < *b; });
    for (auto it : iters) {
      written += writer(context, &(*it));
    }
  } else {
    // Support containers with defined but non-FIFO iteration order.
    using folly::order_preserving_reinsertion_view;
    for (auto& elem : order_preserving_reinsertion_view(out)) {
      written += writer(context, &elem);
    }
  }
  return written;
}

template <typename Map>
size_t writeMap(
    const void* context,
    const void* object,
    bool protocolSortKeys,
    size_t (*writer)(
        const void* /*context*/,
        const void* /*keyElem*/,
        const void* /*valueElem*/)) {
  const Map& out = *static_cast<const Map*>(object);
  size_t written = 0;
  if (!folly::is_detected_v<
          ::apache::thrift::detail::pm::detect_key_compare,
          Map> &&
      protocolSortKeys) {
    std::vector<typename Map::const_iterator> iters;
    iters.reserve(out.size());
    for (auto it = out.begin(); it != out.end(); ++it) {
      iters.push_back(it);
    }
    std::sort(iters.begin(), iters.end(), [](auto a, auto b) {
      return a->first < b->first;
    });
    for (auto it : iters) {
      written += writer(context, &it->first, &it->second);
    }
  } else {
    // Support containers with defined but non-FIFO iteration order.
    using folly::order_preserving_reinsertion_view;
    for (auto& elem_pair : order_preserving_reinsertion_view(out)) {
      written += writer(context, &elem_pair.first, &elem_pair.second);
    }
  }
  return written;
}

template <typename Container>
std::uint32_t containerSize(const void* object) {
  return folly::to_narrow(
      folly::to_unsigned(static_cast<const Container*>(object)->size()));
}

template <typename Map>
void consumeMapElem(
    const void* context,
    void* object,
    void (*keyReader)(const void* /*context*/, void* /*key*/),
    void (*valueReader)(const void* /*context*/, void* /*val*/)) {
  Map& out = *static_cast<Map*>(object);
  typename Map::key_type key;
  keyReader(context, &key);
  valueReader(context, &out[key]);
}

template <typename Map>
void readMap(
    const void* context,
    void* object,
    std::uint32_t mapSize,
    void (*keyReader)(const void* /*context*/, void* /*key*/),
    void (*valueReader)(const void* /*context*/, void* /*val*/)) {
  Map& out = *static_cast<Map*>(object);
  ::apache::thrift::detail::pm::reserve_if_possible(&out, mapSize);

  for (auto i = mapSize; i--;) {
    typename Map::key_type key;
    keyReader(context, &key);
    valueReader(context, &out[key]);
  }
}

template <typename List>
void consumeListElem(
    const void* context,
    void* object,
    void (*reader)(const void* /*context*/, void* /*val*/)) {
  List& out = *static_cast<List*>(object);
  out.emplace_back();
  reader(context, &out.back());
}

template <typename Set>
void consumeSetElem(
    const void* context,
    void* object,
    void (*reader)(const void* /*context*/, void* /*val*/)) {
  Set& out = *static_cast<Set*>(object);
  typename Set::value_type tmp;
  reader(context, &tmp);
  out.insert(std::move(tmp));
}

template <typename Set>
void readKnownLengthSet(
    const void* context,
    void* object,
    std::uint32_t setSize,
    void (*reader)(const void* /*context*/, void* /*val*/)) {
  ::apache::thrift::detail::pm::reserve_if_possible(
      static_cast<Set*>(object), setSize);

  while (setSize--) {
    consumeSetElem<Set>(context, object, reader);
  }
}

template <typename List>
void readList(
    const void* context,
    void* object,
    std::uint32_t listSize,
    void (*reader)(const void* /*context*/, void* /*val*/)) {
  List& out = *static_cast<List*>(object);
  using traits = std::iterator_traits<typename List::iterator>;
  using cat = typename traits::iterator_category;
  if (::apache::thrift::detail::pm::reserve_if_possible(&out, listSize) ||
      std::is_same<cat, std::bidirectional_iterator_tag>::value) {
    while (listSize--) {
      consumeListElem<List>(context, object, reader);
    }
  } else {
    out.resize(listSize);
    for (auto& elem : out) {
      reader(context, &elem);
    }
  }
}

template <typename TypeClass, typename T, typename Enable = void>
struct TypeToInfo;

#define THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(      \
    TypeClass, Type, ThriftType, TTypeValue)       \
  template <>                                      \
  struct TypeToInfo<type_class::TypeClass, Type> { \
    using underlying_type = ThriftType;            \
    static const TypeInfo typeInfo;                \
  }

// Specializations for numbers.
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int8_t,
    std::int8_t,
    T_BYTE);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int16_t,
    std::int16_t,
    T_I16);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int32_t,
    std::int32_t,
    T_I32);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int64_t,
    std::int64_t,
    T_I64);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint8_t,
    std::int8_t,
    T_BYTE);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint16_t,
    std::int16_t,
    T_I16);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint32_t,
    std::int32_t,
    T_I32);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint64_t,
    std::int64_t,
    T_I64);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(integral, bool, bool, T_BOOL);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(floating_point, float, float, T_FLOAT);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(floating_point, double, double, T_DOUBLE);

#undef THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO

// Specialization for string.
#define THRIFT_DEFINE_STRING_TYPE_TO_INFO(TypeClass, T, ExtVal) \
  template <>                                                   \
  struct TypeToInfo<type_class::TypeClass, T> {                 \
    static const StringFieldType ext;                           \
    static const TypeInfo typeInfo;                             \
  }

THRIFT_DEFINE_STRING_TYPE_TO_INFO(string, std::string, StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    string,
    folly::fbstring,
    StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(binary, std::string, StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    binary,
    folly::fbstring,
    StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(binary, folly::IOBuf, StringFieldType::IOBuf);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    binary,
    std::unique_ptr<folly::IOBuf>,
    StringFieldType::IOBufPtr);

#undef THRIFT_DEFINE_STRING_TYPE_TO_INFO

template <typename T>
constexpr VoidFuncPtr getDerefFuncPtr(enable_if_not_smart_ptr_t<T, void*>) {
  return nullptr;
}

template <typename T>
constexpr VoidFuncPtr getDerefFuncPtr(enable_if_smart_ptr_t<T, void*>) {
  return reinterpret_cast<VoidFuncPtr>(identity(derefSmartPointer<T>));
}

// Specialization for set.
template <typename ElemTypeClass, typename T>
struct TypeToInfo<type_class::set<ElemTypeClass>, T> {
  using set_type = maybe_get_element_type_t<T>;
  static const SetFieldExt ext;
  static const TypeInfo typeInfo;
};

template <typename ElemTypeClass, typename T>
const SetFieldExt TypeToInfo<type_class::set<ElemTypeClass>, T>::ext = {
    /* .valInfo */ &TypeToInfo<ElemTypeClass, typename set_type::value_type>::
        typeInfo,
    /* .size */ containerSize<set_type>,
    /* .consumeElem */ consumeSetElem<set_type>,
    /* .readSet */ readKnownLengthSet<set_type>,
    /* .writeSet */ writeSet<set_type>,
};

template <typename ElemTypeClass, typename T>
const TypeInfo TypeToInfo<type_class::set<ElemTypeClass>, T>::typeInfo = {
    /* .type */ protocol::TType::T_SET,
    /* .set */ initialize<T>,
    /* .get */ getDerefFuncPtr<T>(nullptr),
    /* .typeExt */
    &TypeToInfo<type_class::set<ElemTypeClass>, T>::ext,
};

// Specialization for list.
template <typename ElemTypeClass, typename T>
struct TypeToInfo<type_class::list<ElemTypeClass>, T> {
  using list_type = maybe_get_element_type_t<T>;
  static const ListFieldExt ext;
  static const TypeInfo typeInfo;
};
template <typename ElemTypeClass, typename T>
const ListFieldExt TypeToInfo<type_class::list<ElemTypeClass>, T>::ext = {
    /* .valInfo */ &TypeToInfo<ElemTypeClass, typename list_type::value_type>::
        typeInfo,
    /* .size */ containerSize<list_type>,
    /* .consumeElem */ consumeListElem<list_type>,
    /* .readList */ readList<list_type>,
    /* .writeList */ writeList<list_type>,
};
template <typename ElemTypeClass, typename T>
const TypeInfo TypeToInfo<type_class::list<ElemTypeClass>, T>::typeInfo = {
    /* .type */ protocol::TType::T_LIST,
    /* .set */ initialize<T>,
    /* .get */ getDerefFuncPtr<T>(nullptr),
    /* .typeExt */ &ext,
};

// Specialization for map
template <typename KeyTypeClass, typename ValTypeClass, typename T>
struct TypeToInfo<type_class::map<KeyTypeClass, ValTypeClass>, T> {
  using map_type = maybe_get_element_type_t<T>;
  static const MapFieldExt ext;
  static const TypeInfo typeInfo;
};
template <typename KeyTypeClass, typename ValTypeClass, typename T>
const MapFieldExt
    TypeToInfo<type_class::map<KeyTypeClass, ValTypeClass>, T>::ext = {
        /* .keyInfo */ &TypeToInfo<KeyTypeClass, typename map_type::key_type>::
            typeInfo,
        /* .valInfo */
        &TypeToInfo<ValTypeClass, typename map_type::mapped_type>::typeInfo,
        /* .size */ containerSize<map_type>,
        /* .consumeElem */ consumeMapElem<map_type>,
        /* .readMap */ readMap<map_type>,
        /* .writeMap */ writeMap<map_type>,
};
template <typename KeyTypeClass, typename ValTypeClass, typename T>
const TypeInfo
    TypeToInfo<type_class::map<KeyTypeClass, ValTypeClass>, T>::typeInfo = {
        /* .type */ protocol::TType::T_MAP,
        /* .set */ initialize<T>,
        /* .get */ getDerefFuncPtr<T>(nullptr),
        /* .typeExt */ &ext,
};

// Specialization for smart pointers of type class struct and union.
#define THRIFT_DEFINE_STRUCT_PTR_TYPE_INFO(TypeClass)                       \
  template <typename T>                                                     \
  struct TypeToInfo<type_class::TypeClass, T, enable_if_smart_ptr_t<T>> {   \
    static const TypeInfo typeInfo;                                         \
  };                                                                        \
                                                                            \
  template <typename T>                                                     \
  const TypeInfo TypeToInfo<                                                \
      type_class::TypeClass,                                                \
      T,                                                                    \
      enable_if_smart_ptr_t<T>>::typeInfo = {                               \
      TypeToInfo<type_class::TypeClass, typename T::element_type>::typeInfo \
          .type,                                                            \
      TypeToInfo<type_class::TypeClass, typename T::element_type>::typeInfo \
          .set,                                                             \
      reinterpret_cast<VoidFuncPtr>(identity(derefSmartPointer<T>)),        \
      TypeToInfo<type_class::TypeClass, typename T::element_type>::typeInfo \
          .typeExt,                                                         \
  }

THRIFT_DEFINE_STRUCT_PTR_TYPE_INFO(structure);
THRIFT_DEFINE_STRUCT_PTR_TYPE_INFO(variant);
#undef THRIFT_DEFINE_STRUCT_PTR_TYPE_INFO

template <class ThriftStruct>
constexpr ptrdiff_t fieldOffset(std::int16_t fieldIndex);

template <class ThriftStruct>
constexpr ptrdiff_t issetOffset(std::int16_t fieldIndex);

template <class ThriftUnion>
constexpr ptrdiff_t unionTypeOffset();

template <class Protocol_>
void read(Protocol_* iprot, const StructInfo& structInfo, void* object);

template <class Protocol_>
size_t
write(Protocol_* iprot, const StructInfo& structInfo, const void* object);

} // namespace detail
} // namespace thrift
} // namespace apache
