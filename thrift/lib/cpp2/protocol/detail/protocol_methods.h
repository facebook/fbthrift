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
#include <bitset>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

#include <folly/Conv.h>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <folly/functional/Invoke.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/ProtocolReaderWireTypeInfo.h>

/**
 * Specializations of `protocol_methods` encapsulate a collection of
 * read/write/size/sizeZC methods that can be performed on Thrift
 * objects and primitives. TypeClass (see apache::thrift::type_class)
 * refers to the general type of data that Type is, and is passed around for
 * two reasons:
 *  - to provide support for generic containers which have a common interface
 *    for building collections (e.g. a `std::vector<int>` and `std::deque<int>`,
 *    which can back a Thrift list, and thus have
 *    `type_class::list<type_class::integral>`, or an
 *    `std::map<std::string, MyStruct>` would have
 *    `type_class::map<type_class::string, type_class::structure>``).
 *  - to differentiate between Thrift types that are represented with the
 *    same C++ type, e.g. both Thrift binaries and strings are represented
 *    with an `std::string`, TypeClass is used to decide which protocol
 *    method to call.
 *
 * Example:
 *
 * // MyModule.thrift:
 * struct MyStruct {
 *   1: list<set<string>> fieldA
 * }
 *
 * // C++
 *
 * using methods = protocol_methods<
 *    type_class::list<type_class::set<type_class::string>>,
 *    std::vector<std::set<std::string>>>
 *
 * MyStruct struct_instance;
 * CompactProtocolReader reader;
 * methods::read(struct_instance.fieldA, reader);
 */

namespace apache {
namespace thrift {
namespace detail {
namespace pm {

// This macro backports "if constexpr" into C++-pre-17. The idea is, we form a
// tuple of two lambdas (one for when the condition is true, and one for when
// it's false), and use get<> to select the lambda to actually invoke, by
// passing the condition as the index.
// There's a wrinkle; the body of the lambda still has to parse without errors,
// which it won't do if we call a method that doesn't exist within its body that
// doesn't depend on a template parameter. So we make the lambdas take an auto
// argument, and the types of anything we need to by making them depend on that
// argument. This is TypeHider.
//
// This is a fairly ugly hack. To get rid of it, we need to either:
// - Drop support for earlier versions of C++.
// - Teach all protocols, and dependent code, about context-taking methods.
// Neither of these is ideal in the short term. Eventually, we should probably
// do both.
#define THRIFT_PROTOCOL_METHODS_UNPAREN(...) __VA_ARGS__
#define THRIFT_PROTOCOL_METHODS_IF_THEN_ELSE_CONSTEXPR(cond, T, E) \
  std::get<(cond) ? 0 : 1>(std::forward_as_tuple(                  \
      [&](auto _) { THRIFT_PROTOCOL_METHODS_UNPAREN T },           \
      [&](auto _) { THRIFT_PROTOCOL_METHODS_UNPAREN E }))(TypeHider{})
struct TypeHider {
  template <typename T>
  T& operator()(T& t) {
    return t;
  }
};

template <typename T>
auto reserve_if_possible(T* t, std::uint32_t size)
    -> decltype(t->reserve(size), std::true_type{}) {
  // Workaround for libstdc++ < 7, resize to `size + 1` to avoid an extra
  // rehash: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71181.
  // TODO: Remove once libstdc++ < 7 is not supported any longer.
  std::uint32_t extra = folly::kIsGlibcxx && folly::kGlibcxxVer < 7 ? 1 : 0;
  t->reserve(size + extra);
  return {};
}

template <typename... T>
std::false_type reserve_if_possible(T&&...) {
  return {};
}

template <typename Void, typename T>
struct sorted_unique_constructible_ : std::false_type {};
template <typename T>
struct sorted_unique_constructible_<
    folly::void_t<
        decltype(T(folly::sorted_unique, typename T::container_type())),
        decltype(T(typename T::container_type()))>,
    T> : std::true_type {};
template <typename T>
struct sorted_unique_constructible : sorted_unique_constructible_<void, T> {};

FOLLY_CREATE_MEMBER_INVOKER(emplace_hint_invoker, emplace_hint);

template <typename T>
using detect_key_compare = typename T::key_compare;

template <typename T>
using map_emplace_hint_is_invocable = folly::is_invocable<
    emplace_hint_invoker,
    T,
    typename T::iterator,
    typename T::key_type,
    typename T::mapped_type>;

template <typename T>
using set_emplace_hint_is_invocable = folly::is_invocable<
    emplace_hint_invoker,
    T,
    typename T::iterator,
    typename T::value_type>;

template <typename Map, typename KeyDeserializer, typename MappedDeserializer>
typename std::enable_if<sorted_unique_constructible<Map>::value>::type
deserialize_known_length_map(
    Map& map,
    std::uint32_t map_size,
    KeyDeserializer const& kr,
    MappedDeserializer const& mr) {
  if (map_size == 0) {
    return;
  }

  bool sorted = true;
  typename Map::container_type tmp(map.get_allocator());
  reserve_if_possible(&tmp, map_size);
  tmp.emplace_back();
  kr(tmp[0].first);
  mr(tmp[0].second);
  for (size_t i = 1; i < map_size; ++i) {
    tmp.emplace_back();
    kr(tmp[i].first);
    mr(tmp[i].second);
    sorted = sorted && map.key_comp()(tmp[i - 1].first, tmp[i].first);
  }

  using folly::sorted_unique;
  map = sorted ? Map(sorted_unique, std::move(tmp)) : Map(std::move(tmp));
}

template <typename Map, typename KeyDeserializer, typename MappedDeserializer>
typename std::enable_if<
    !sorted_unique_constructible<Map>::value &&
    map_emplace_hint_is_invocable<Map>::value>::type
deserialize_known_length_map(
    Map& map,
    std::uint32_t map_size,
    KeyDeserializer const& kr,
    MappedDeserializer const& mr) {
  reserve_if_possible(&map, map_size);

  for (auto i = map_size; i--;) {
    typename Map::key_type key;
    kr(key);
    auto iter = map.emplace_hint(
        map.end(), std::move(key), typename Map::mapped_type{});
    mr(iter->second);
  }
}

template <typename Map, typename KeyDeserializer, typename MappedDeserializer>
typename std::enable_if<
    !sorted_unique_constructible<Map>::value &&
    !map_emplace_hint_is_invocable<Map>::value>::type
deserialize_known_length_map(
    Map& map,
    std::uint32_t map_size,
    KeyDeserializer const& kr,
    MappedDeserializer const& mr) {
  reserve_if_possible(&map, map_size);

  for (auto i = map_size; i--;) {
    typename Map::key_type key;
    kr(key);
    mr(map[std::move(key)]);
  }
}

template <typename Set, typename ValDeserializer>
typename std::enable_if<sorted_unique_constructible<Set>::value>::type
deserialize_known_length_set(
    Set& set,
    std::uint32_t set_size,
    ValDeserializer const& vr) {
  if (set_size == 0) {
    return;
  }

  bool sorted = true;
  typename Set::container_type tmp(set.get_allocator());
  reserve_if_possible(&tmp, set_size);
  tmp.emplace_back();
  vr(tmp[0]);
  for (size_t i = 1; i < set_size; ++i) {
    tmp.emplace_back();
    vr(tmp[i]);
    sorted = sorted && set.key_comp()(tmp[i - 1], tmp[i]);
  }

  using folly::sorted_unique;
  set = sorted ? Set(sorted_unique, std::move(tmp)) : Set(std::move(tmp));
}

template <typename Set, typename ValDeserializer>
typename std::enable_if<
    !sorted_unique_constructible<Set>::value &&
    set_emplace_hint_is_invocable<Set>::value>::type
deserialize_known_length_set(
    Set& set,
    std::uint32_t set_size,
    ValDeserializer const& vr) {
  reserve_if_possible(&set, set_size);

  for (auto i = set_size; i--;) {
    typename Set::value_type value;
    vr(value);
    set.emplace_hint(set.end(), std::move(value));
  }
}

template <typename Set, typename ValDeserializer>
typename std::enable_if<
    !sorted_unique_constructible<Set>::value &&
    !set_emplace_hint_is_invocable<Set>::value>::type
deserialize_known_length_set(
    Set& set,
    std::uint32_t set_size,
    ValDeserializer const& vr) {
  reserve_if_possible(&set, set_size);

  for (auto i = set_size; i--;) {
    typename Set::value_type value;
    vr(value);
    set.insert(std::move(value));
  }
}

/*
 * Primitive Types Specialization
 */
template <typename TypeClass, typename Type, typename Enable = void>
struct protocol_methods;

#define THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON(                          \
    Class, Type, Method, TTypeValue)                                         \
  constexpr static protocol::TType ttype_value = protocol::TTypeValue;       \
  template <typename Protocol>                                               \
  static void read(Protocol& protocol, Type& out) {                          \
    protocol.read##Method(out);                                              \
  }                                                                          \
  template <typename Protocol, typename Context>                             \
  static void readWithContext(Protocol& protocol, Type& out, Context& ctx) { \
    THRIFT_PROTOCOL_METHODS_IF_THEN_ELSE_CONSTEXPR(                          \
        Context::kAcceptsContext,                                            \
        (_(protocol).read##Method##WithContext(out, ctx);),                  \
        (_(protocol).read##Method(out);));                                   \
  }                                                                          \
  template <typename Protocol>                                               \
  static std::size_t write(Protocol& protocol, Type const& in) {             \
    return protocol.write##Method(in);                                       \
  }

#define THRIFT_PROTOCOL_METHODS_REGISTER_SS_COMMON(                       \
    Class, Type, Method, TTypeValue)                                      \
  template <bool, typename Protocol>                                      \
  static std::size_t serializedSize(Protocol& protocol, Type const& in) { \
    return protocol.serializedSize##Method(in);                           \
  }

// stamp out specializations for primitive types
// TODO: Perhaps change ttype_value to a static constexpr member function, as
// those might instantiate faster than a constexpr objects
#define THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(   \
    Class, Type, Method, TTypeValue)                 \
  template <>                                        \
  struct protocol_methods<type_class::Class, Type> { \
    THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON(      \
        Class,                                       \
        Type,                                        \
        Method,                                      \
        TTypeValue)                                  \
    THRIFT_PROTOCOL_METHODS_REGISTER_SS_COMMON(      \
        Class,                                       \
        Type,                                        \
        Method,                                      \
        TTypeValue)                                  \
  }

THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(integral, std::int8_t, Byte, T_BYTE);
THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(integral, std::int16_t, I16, T_I16);
THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(integral, std::int32_t, I32, T_I32);
THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(integral, std::int64_t, I64, T_I64);

// Macros for defining protocol_methods for unsigned integers
// Need special macros due to the casts needed
#define THRIFT_PROTOCOL_METHODS_REGISTER_RW_UI(                              \
    Class, Type, Method, TTypeValue)                                         \
  constexpr static protocol::TType ttype_value = protocol::TTypeValue;       \
  using SignedType = std::make_signed_t<Type>;                               \
  template <typename Protocol>                                               \
  static void read(Protocol& protocol, Type& out) {                          \
    SignedType tmp;                                                          \
    protocol.read##Method(tmp);                                              \
    out = folly::to_unsigned(tmp);                                           \
  }                                                                          \
  template <typename Protocol, typename Context>                             \
  static void readWithContext(Protocol& protocol, Type& out, Context& ctx) { \
    SignedType tmp;                                                          \
    THRIFT_PROTOCOL_METHODS_IF_THEN_ELSE_CONSTEXPR(                          \
        Context::kAcceptsContext,                                            \
        (_(protocol).read##Method##WithContext(tmp, ctx);),                  \
        (_(protocol).read##Method(tmp);));                                   \
    out = folly::to_unsigned(tmp);                                           \
  }                                                                          \
  template <typename Protocol>                                               \
  static std::size_t write(Protocol& protocol, Type const& in) {             \
    return protocol.write##Method(folly::to_signed(in));                     \
  }

#define THRIFT_PROTOCOL_METHODS_REGISTER_SS_UI(                           \
    Class, Type, Method, TTypeValue)                                      \
  template <bool, typename Protocol>                                      \
  static std::size_t serializedSize(Protocol& protocol, Type const& in) { \
    return protocol.serializedSize##Method(folly::to_signed(in));         \
  }

// stamp out specializations for unsigned integer primitive types
// TODO: Perhaps change ttype_value to a static constexpr member function, as
// those might instantiate faster than a constexpr objects
#define THRIFT_PROTOCOL_METHODS_REGISTER_UI(Class, Type, Method, TTypeValue) \
  template <>                                                                \
  struct protocol_methods<type_class::Class, Type> {                         \
    THRIFT_PROTOCOL_METHODS_REGISTER_RW_UI(Class, Type, Method, TTypeValue)  \
    THRIFT_PROTOCOL_METHODS_REGISTER_SS_UI(Class, Type, Method, TTypeValue)  \
  }

THRIFT_PROTOCOL_METHODS_REGISTER_UI(integral, std::uint8_t, Byte, T_BYTE);
THRIFT_PROTOCOL_METHODS_REGISTER_UI(integral, std::uint16_t, I16, T_I16);
THRIFT_PROTOCOL_METHODS_REGISTER_UI(integral, std::uint32_t, I32, T_I32);
THRIFT_PROTOCOL_METHODS_REGISTER_UI(integral, std::uint64_t, I64, T_I64);

#undef THRIFT_PROTOCOL_METHODS_REGISTER_UI
#undef THRIFT_PROTOCOL_METHODS_REGISTER_RW_UI
#undef THRIFT_PROTOCOL_METHODS_REGISTER_SS_UI

// std::vector<bool> isn't actually a container, so
// define a special overload which takes its specialized
// proxy type
template <>
struct protocol_methods<type_class::integral, bool> {
  THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON(integral, bool, Bool, T_BOOL)
  THRIFT_PROTOCOL_METHODS_REGISTER_SS_COMMON(integral, bool, Bool, T_BOOL)

  template <typename Protocol>
  static void read(Protocol& protocol, std::vector<bool>::reference out) {
    bool tmp;
    read(protocol, tmp);
    out = tmp;
  }
};

THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(
    floating_point,
    double,
    Double,
    T_DOUBLE);
THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD(
    floating_point,
    float,
    Float,
    T_FLOAT);

#undef THRIFT_PROTOCOL_METHODS_REGISTER_OVERLOAD

template <typename Type>
struct protocol_methods<type_class::string, Type> {
  THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON(string, Type, String, T_STRING)
  THRIFT_PROTOCOL_METHODS_REGISTER_SS_COMMON(string, Type, String, T_STRING)
};

template <typename Type>
struct protocol_methods<type_class::binary, Type> {
  THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON(binary, Type, Binary, T_STRING)

  template <bool ZeroCopy, typename Protocol>
  static typename std::enable_if<ZeroCopy, std::size_t>::type serializedSize(
      Protocol& protocol,
      Type const& in) {
    return protocol.serializedSizeZCBinary(in);
  }
  template <bool ZeroCopy, typename Protocol>
  static typename std::enable_if<!ZeroCopy, std::size_t>::type serializedSize(
      Protocol& protocol,
      Type const& in) {
    return protocol.serializedSizeBinary(in);
  }
};

#undef THRIFT_PROTOCOL_METHODS_REGISTER_SS_COMMON
#undef THRIFT_PROTOCOL_METHODS_REGISTER_RW_COMMON
#undef THRIFT_PROTOCOL_METHODS_IF_THEN_ELSE_CONSTEXPR
#undef THRIFT_PROTOCOL_METHODS_UNPAREN

/*
 * Enum Specialization
 */
template <typename Type>
struct protocol_methods<type_class::enumeration, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_I32;

  // Thrift enums are always read as int32_t
  using int_type = std::int32_t;
  using int_methods = protocol_methods<type_class::integral, int_type>;

  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    int_type tmp;
    int_methods::read(protocol, tmp);
    out = static_cast<Type>(tmp);
  }

  template <typename Protocol, typename Context>
  static void readWithContext(Protocol& protocol, Type& out, Context& ctx) {
    int_type tmp;
    int_methods::readWithContext(protocol, tmp, ctx);
    out = static_cast<Type>(tmp);
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& in) {
    int_type tmp = static_cast<int_type>(in);
    return int_methods::template write<Protocol>(protocol, tmp);
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& in) {
    int_type tmp = static_cast<int_type>(in);
    return int_methods::template serializedSize<ZeroCopy>(protocol, tmp);
  }
};

/*
 * List Specialization
 */
template <typename ElemClass, typename Type>
struct protocol_methods<type_class::list<ElemClass>, Type> {
  static_assert(
      !std::is_same<ElemClass, type_class::unknown>(),
      "Unable to serialize unknown list element");

  constexpr static protocol::TType ttype_value = protocol::T_LIST;

  using elem_type = typename Type::value_type;
  using elem_methods = protocol_methods<ElemClass, elem_type>;

  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    std::uint32_t list_size = -1;
    using WireTypeInfo = ProtocolReaderWireTypeInfo<Protocol>;
    using WireType = typename WireTypeInfo::WireType;

    WireType reported_type = WireTypeInfo::defaultValue();

    protocol.readListBegin(reported_type, list_size);
    if (protocol.kOmitsContainerSizes()) {
      // list size unknown, SimpleJSON protocol won't know type, either
      // so let's just hope that it spits out something that makes sense
      while (protocol.peekList()) {
        out.emplace_back();
        elem_methods::read(protocol, out.back());
      }
    } else {
      if (reported_type != WireTypeInfo::fromTType(elem_methods::ttype_value)) {
        apache::thrift::skip_n(protocol, list_size, {reported_type});
      } else {
        if (!canReadNElements(protocol, list_size, {reported_type})) {
          protocol::TProtocolException::throwTruncatedData();
        }

        using traits = std::iterator_traits<typename Type::iterator>;
        using cat = typename traits::iterator_category;
        if (reserve_if_possible(&out, list_size) ||
            std::is_same<cat, std::bidirectional_iterator_tag>::value) {
          // use bidi as a hint for doubly linked list containers like std::list
          while (list_size--) {
            out.emplace_back();
            elem_methods::read(protocol, out.back());
          }
        } else {
          out.resize(list_size);
          for (auto&& elem : out) {
            elem_methods::read(protocol, elem);
          }
        }
      }
    }
    protocol.readListEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeListBegin(
        elem_methods::ttype_value,
        folly::to_narrow(folly::to_unsigned(out.size())));
    for (auto const& elem : out) {
      xfer += elem_methods::write(protocol, elem);
    }
    xfer += protocol.writeListEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.serializedSizeListBegin(
        elem_methods::ttype_value,
        folly::to_narrow(folly::to_unsigned(out.size())));
    for (auto const& elem : out) {
      xfer += elem_methods::template serializedSize<ZeroCopy>(protocol, elem);
    }
    xfer += protocol.serializedSizeListEnd();
    return xfer;
  }
};

/*
 * Set Specialization
 */
template <typename ElemClass, typename Type>
struct protocol_methods<type_class::set<ElemClass>, Type> {
  static_assert(
      !std::is_same<ElemClass, type_class::unknown>(),
      "Unable to serialize unknown type");

  constexpr static protocol::TType ttype_value = protocol::T_SET;

  using elem_type = typename Type::value_type;
  using elem_methods = protocol_methods<ElemClass, elem_type>;

 private:
  template <typename Protocol>
  static void consume_elem(Protocol& protocol, Type& out) {
    elem_type tmp;
    elem_methods::read(protocol, tmp);
    out.insert(std::move(tmp));
  }

 public:
  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    std::uint32_t set_size = -1;

    using WireTypeInfo = ProtocolReaderWireTypeInfo<Protocol>;
    using WireType = typename WireTypeInfo::WireType;

    WireType reported_type = WireTypeInfo::defaultValue();

    protocol.readSetBegin(reported_type, set_size);
    if (protocol.kOmitsContainerSizes()) {
      while (protocol.peekSet()) {
        consume_elem(protocol, out);
      }
    } else {
      if (reported_type != WireTypeInfo::fromTType(elem_methods::ttype_value)) {
        apache::thrift::skip_n(protocol, set_size, {reported_type});
      } else {
        if (!canReadNElements(protocol, set_size, {reported_type})) {
          protocol::TProtocolException::throwTruncatedData();
        }
        auto const vreader = [&protocol](auto& value) {
          elem_methods::read(protocol, value);
        };
        deserialize_known_length_set(out, set_size, vreader);
      }
    }
    protocol.readSetEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeSetBegin(
        elem_methods::ttype_value,
        folly::to_narrow(folly::to_unsigned(out.size())));

    if (!folly::is_detected_v<detect_key_compare, Type> &&
        protocol.kSortKeys()) {
      std::vector<typename Type::const_iterator> iters;
      iters.reserve(out.size());
      for (auto it = out.begin(); it != out.end(); ++it) {
        iters.push_back(it);
      }
      std::sort(
          iters.begin(), iters.end(), [](auto a, auto b) { return *a < *b; });
      for (auto it : iters) {
        xfer += elem_methods::write(protocol, *it);
      }
    } else {
      // Support containers with defined but non-FIFO iteration order.
      using folly::order_preserving_reinsertion_view;
      for (auto const& elem : order_preserving_reinsertion_view(out)) {
        xfer += elem_methods::write(protocol, elem);
      }
    }
    xfer += protocol.writeSetEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.serializedSizeSetBegin(
        elem_methods::ttype_value,
        folly::to_narrow(folly::to_unsigned(out.size())));
    for (auto const& elem : out) {
      xfer += elem_methods::template serializedSize<ZeroCopy>(protocol, elem);
    }
    xfer += protocol.serializedSizeSetEnd();
    return xfer;
  }
};

/*
 * Map Specialization
 */
template <typename KeyClass, typename MappedClass, typename Type>
struct protocol_methods<type_class::map<KeyClass, MappedClass>, Type> {
  static_assert(
      !std::is_same<KeyClass, type_class::unknown>(),
      "Unable to serialize unknown key type in map");
  static_assert(
      !std::is_same<MappedClass, type_class::unknown>(),
      "Unable to serialize unknown mapped type in map");

  constexpr static protocol::TType ttype_value = protocol::T_MAP;

  using key_type = typename Type::key_type;
  using mapped_type = typename Type::mapped_type;
  using key_methods = protocol_methods<KeyClass, key_type>;
  using mapped_methods = protocol_methods<MappedClass, mapped_type>;

 protected:
  template <typename Protocol>
  static void consume_elem(Protocol& protocol, Type& out) {
    key_type key_tmp;
    key_methods::read(protocol, key_tmp);
    mapped_methods::read(protocol, out[std::move(key_tmp)]);
  }

 public:
  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    std::uint32_t map_size = -1;
    using WireTypeInfo = ProtocolReaderWireTypeInfo<Protocol>;
    using WireType = typename WireTypeInfo::WireType;

    WireType rpt_key_type = WireTypeInfo::defaultValue(),
             rpt_mapped_type = WireTypeInfo::defaultValue();

    protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
    if (protocol.kOmitsContainerSizes()) {
      while (protocol.peekMap()) {
        consume_elem(protocol, out);
      }
    } else {
      // CompactProtocol does not transmit key/mapped types if
      // the map is empty
      if (map_size > 0 &&
          (WireTypeInfo::fromTType(key_methods::ttype_value) != rpt_key_type ||
           WireTypeInfo::fromTType(mapped_methods::ttype_value) !=
               rpt_mapped_type)) {
        apache::thrift::skip_n(
            protocol, map_size, {rpt_key_type, rpt_mapped_type});
      } else {
        if (!canReadNElements(
                protocol, map_size, {rpt_key_type, rpt_mapped_type})) {
          protocol::TProtocolException::throwTruncatedData();
        }
        auto const kreader = [&protocol](auto& key) {
          key_methods::read(protocol, key);
        };
        auto const vreader = [&protocol](auto& value) {
          mapped_methods::read(protocol, value);
        };
        deserialize_known_length_map(out, map_size, kreader, vreader);
      }
    }
    protocol.readMapEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeMapBegin(
        key_methods::ttype_value, mapped_methods::ttype_value, out.size());

    if (!folly::is_detected_v<detect_key_compare, Type> &&
        protocol.kSortKeys()) {
      std::vector<typename Type::const_iterator> iters;
      iters.reserve(out.size());
      for (auto it = out.begin(); it != out.end(); ++it) {
        iters.push_back(it);
      }
      std::sort(iters.begin(), iters.end(), [](auto a, auto b) {
        return a->first < b->first;
      });
      for (auto it : iters) {
        xfer += key_methods::write(protocol, it->first);
        xfer += mapped_methods::write(protocol, it->second);
      }
    } else {
      // Support containers with defined but non-FIFO iteration order.
      using folly::order_preserving_reinsertion_view;
      for (auto const& elem_pair : order_preserving_reinsertion_view(out)) {
        xfer += key_methods::write(protocol, elem_pair.first);
        xfer += mapped_methods::write(protocol, elem_pair.second);
      }
    }
    xfer += protocol.writeMapEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.serializedSizeMapBegin(
        key_methods::ttype_value,
        mapped_methods::ttype_value,
        folly::to_narrow(folly::to_unsigned(out.size())));
    for (auto const& elem_pair : out) {
      xfer += key_methods::template serializedSize<ZeroCopy>(
          protocol, elem_pair.first);
      xfer += mapped_methods::template serializedSize<ZeroCopy>(
          protocol, elem_pair.second);
    }
    xfer += protocol.serializedSizeMapEnd();
    return xfer;
  }
};

/*
 * Struct with Indirection Specialization
 */
template <typename ElemClass, typename Indirection, typename Type>
struct protocol_methods<indirection_tag<ElemClass, Indirection>, Type> {
  using indirection = Indirection;
  using elem_type = std::remove_reference_t<decltype(
      indirection::template get(std::declval<Type&>()))>;
  using elem_methods = protocol_methods<ElemClass, elem_type>;

  // Forward the ttype_value from the internal element
  constexpr static protocol::TType ttype_value = elem_methods::ttype_value;

  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    elem_methods::read(protocol, indirection::template get<Type&>(out));
  }

  template <typename Protocol, typename Context>
  static void readWithContext(Protocol& protocol, Type& out, Context& ctx) {
    elem_methods::readWithContext(
        protocol, indirection::template get<Type&>(out), ctx);
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& in) {
    return elem_methods::write(
        protocol, indirection::template get<const Type&>(in));
  }
  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& in) {
    return elem_methods::template serializedSize<ZeroCopy>(
        protocol, indirection::template get<const Type&>(in));
  }
};

/*
 * Struct Specialization
 * Forwards to Cpp2Ops wrapper around member read/write/etc.
 */
template <typename Type>
struct protocol_methods<type_class::structure, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_STRUCT;

  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    Cpp2Ops<Type>::read(&protocol, &out);
  }
  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& in) {
    return Cpp2Ops<Type>::write(&protocol, &in);
  }
  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& in) {
    if (ZeroCopy) {
      return Cpp2Ops<Type>::serializedSizeZC(&protocol, &in);
    } else {
      return Cpp2Ops<Type>::serializedSize(&protocol, &in);
    }
  }
};

/*
 * Union Specialization
 * Forwards to Cpp2Ops wrapper around member read/write/etc.
 */
template <typename Type>
struct protocol_methods<type_class::variant, Type>
    : protocol_methods<type_class::structure, Type> {};

} // namespace pm
} // namespace detail
} // namespace thrift
} // namespace apache
