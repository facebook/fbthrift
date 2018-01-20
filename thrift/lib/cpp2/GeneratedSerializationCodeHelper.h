/*
 * Copyright 2016-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <bitset>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include <folly/Utility.h>
#include <folly/functional/Invoke.h>

#include <thrift/lib/cpp2/TypeClass.h>

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

template <typename T>
inline auto reserve_if_possible(T* t, std::uint32_t size)
    -> decltype(t->reserve(size), void()) {
  // Resize to `size + 1` to avoid an extra rehash:
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71181.
  t->reserve(size + 1);
}

inline void reserve_if_possible(...){};

template <typename T>
using presorted_constructible_from_vector_value_type = std::is_constructible<
    T,
    folly::presorted_t,
    std::vector<typename T::value_type>&&>;

FOLLY_CREATE_MEMBER_INVOKE_TRAITS(emplace_hint_traits, emplace_hint);

template <typename T>
using map_emplace_hint_is_invocable = emplace_hint_traits::is_invocable<
    T,
    typename T::iterator,
    typename T::key_type,
    typename T::mapped_type>;

template <typename T>
using set_emplace_hint_is_invocable = emplace_hint_traits::
    is_invocable<T, typename T::iterator, typename T::value_type>;

template <typename Map, typename KeyDeserializer, typename MappedDeserializer>
typename std::enable_if<
    presorted_constructible_from_vector_value_type<Map>::value>::type
deserialize_known_length_map(
    Map& map,
    std::uint32_t map_size,
    KeyDeserializer const& kr,
    MappedDeserializer const& mr) {
  if (map_size == 0) {
    return;
  }

  bool sorted = true;
  std::vector<typename Map::value_type> tmp(map_size);
  kr(tmp[0].first);
  mr(tmp[0].second);
  for (size_t i = 1; i < map_size; ++i) {
    kr(tmp[i].first);
    mr(tmp[i].second);
    sorted = sorted && map.key_comp()(tmp[i - 1].first, tmp[i].first);
  }

  if (LIKELY(map.empty())) {
    if (!sorted) {
      std::sort(tmp.begin(), tmp.end(), map.value_comp());
    }
    map = Map(folly::presorted, std::move(tmp));
  } else {
    map.insert(
        std::make_move_iterator(tmp.begin()),
        std::make_move_iterator(tmp.end()));
  }
}

template <typename Map, typename KeyDeserializer, typename MappedDeserializer>
typename std::enable_if<
    !presorted_constructible_from_vector_value_type<Map>::value &&
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
    !presorted_constructible_from_vector_value_type<Map>::value &&
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
typename std::enable_if<
    presorted_constructible_from_vector_value_type<Set>::value>::type
deserialize_known_length_set(
    Set& set,
    std::uint32_t set_size,
    ValDeserializer const& vr) {
  if (set_size == 0) {
    return;
  }

  bool sorted = true;
  std::vector<typename Set::value_type> tmp(set_size);
  vr(tmp[0]);
  for (size_t i = 1; i < set_size; ++i) {
    vr(tmp[i]);
    sorted = sorted && set.key_comp()(tmp[i - 1], tmp[i]);
  }

  if (LIKELY(set.empty())) {
    if (!sorted) {
      std::sort(tmp.begin(), tmp.end(), set.value_comp());
    }
    set = Set(folly::presorted, std::move(tmp));
  } else {
    set.insert(
        std::make_move_iterator(tmp.begin()),
        std::make_move_iterator(tmp.end()));
  }
}

template <typename Set, typename ValDeserializer>
typename std::enable_if<
    !presorted_constructible_from_vector_value_type<Set>::value &&
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
    !presorted_constructible_from_vector_value_type<Set>::value &&
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

template <typename Indirection, typename TypeClass>
struct IndirectionTag;

/*
 * Primitive Types Specialization
 */
template <typename TypeClass, typename Type, typename Enable = void>
struct protocol_methods;

#define REGISTER_RW_COMMON(Class, Type, Method, TTypeValue)            \
  constexpr static protocol::TType ttype_value = protocol::TTypeValue; \
  template <typename Protocol>                                         \
  static void read(Protocol& protocol, Type& out) {                    \
    protocol.read##Method(out);                                        \
  }                                                                    \
  template <typename Protocol>                                         \
  static std::size_t write(Protocol& protocol, Type const& in) {       \
    return protocol.write##Method(in);                                 \
  }

#define REGISTER_SS_COMMON(Class, Type, Method, TTypeValue)               \
  template <bool, typename Protocol>                                      \
  static std::size_t serializedSize(Protocol& protocol, Type const& in) { \
    return protocol.serializedSize##Method(in);                           \
  }

// stamp out specializations for primitive types
// TODO: Perhaps change ttype_value to a static constexpr member function, as
// those might instantiate faster than a constexpr objects
#define REGISTER_OVERLOAD(Class, Type, Method, TTypeValue) \
  template <>                                              \
  struct protocol_methods<type_class::Class, Type> {       \
    REGISTER_RW_COMMON(Class, Type, Method, TTypeValue)    \
    REGISTER_SS_COMMON(Class, Type, Method, TTypeValue)    \
  }

#define REGISTER_INTEGRAL(...) REGISTER_OVERLOAD(integral, __VA_ARGS__)
REGISTER_INTEGRAL(std::int8_t, Byte, T_BYTE);
REGISTER_INTEGRAL(std::int16_t, I16, T_I16);
REGISTER_INTEGRAL(std::int32_t, I32, T_I32);
REGISTER_INTEGRAL(std::int64_t, I64, T_I64);
#undef REGISTER_INTEGRAL

// std::vector<bool> isn't actually a container, so
// define a special overload which takes its specialized
// proxy type
template <>
struct protocol_methods<type_class::integral, bool> {
  REGISTER_RW_COMMON(integral, bool, Bool, T_BOOL)
  REGISTER_SS_COMMON(integral, bool, Bool, T_BOOL)

  template <typename Protocol>
  static void read(Protocol& protocol, std::vector<bool>::reference out) {
    bool tmp;
    read(protocol, tmp);
    out = tmp;
  }
};

#define REGISTER_FP(...) REGISTER_OVERLOAD(floating_point, __VA_ARGS__)
REGISTER_FP(double, Double, T_DOUBLE);
REGISTER_FP(float, Float, T_FLOAT);
#undef REGISTER_FP

// TODO: might not need to specialize on std::string, but rather
// just the string typeclass
REGISTER_OVERLOAD(string, std::string, String, T_STRING);
REGISTER_OVERLOAD(string, folly::fbstring, String, T_STRING);

#undef REGISTER_OVERLOAD

#define REGISTER_ZC(Class, Type, Method, TTypeValue)             \
  template <>                                                    \
  struct protocol_methods<type_class::Class, Type> {             \
    REGISTER_RW_COMMON(Class, Type, Method, TTypeValue)          \
    template <bool ZeroCopy, typename Protocol>                  \
    static typename std::enable_if<ZeroCopy, std::size_t>::type  \
    serializedSize(Protocol& protocol, Type const& in) {         \
      return protocol.serializedSizeZC##Method(in);              \
    }                                                            \
    template <bool ZeroCopy, typename Protocol>                  \
    static typename std::enable_if<!ZeroCopy, std::size_t>::type \
    serializedSize(Protocol& protocol, Type const& in) {         \
      return protocol.serializedSize##Method(in);                \
    }                                                            \
  };

#define REGISTER_BINARY(...) REGISTER_ZC(binary, __VA_ARGS__)
REGISTER_BINARY(std::string, Binary, T_STRING);
REGISTER_BINARY(folly::IOBuf, Binary, T_STRING);
REGISTER_BINARY(std::unique_ptr<folly::IOBuf>, Binary, T_STRING);
REGISTER_BINARY(folly::fbstring, Binary, T_STRING);
#undef REGISTER_BINARY
#undef REGISTER_ZC // this naming sure isn't confusing, no sir
#undef REGISTER_SS_COMMON
#undef REGISTER_RW_COMMON

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
    TType reported_type = protocol::T_STOP;

    protocol.readListBegin(reported_type, list_size);
    if (protocol.kOmitsContainerSizes()) {
      // list size unknown, SimpleJSON protocol won't know type, either
      // so let's just hope that it spits out something that makes sense
      // (if it did set reported_type to something known)
      if (reported_type != protocol::T_STOP &&
          reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      for (std::uint32_t i = 0; protocol.peekList(); ++i) {
        // TODO: Grow this better (e.g. 1.5x each time)
        out.resize(i + 1);
        elem_methods::read(protocol, out[i]);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      out.resize(list_size);
      for (auto&& elem : out) {
        elem_methods::read(protocol, elem);
      }
    }
    protocol.readListEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeListBegin(elem_methods::ttype_value, out.size());
    for (auto const& elem : out) {
      xfer += elem_methods::write(protocol, elem);
    }
    xfer += protocol.writeListEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer +=
        protocol.serializedSizeListBegin(elem_methods::ttype_value, out.size());
    for (auto const& elem : out) {
      xfer += elem_methods::template serializedSize<ZeroCopy>(protocol, elem);
    }
    xfer += protocol.serializedSizeListEnd();
    return xfer;
  }
};

/*
 * std::list Specialization
 */
template <typename ElemClass, typename Type>
struct protocol_methods<type_class::list<ElemClass>, std::list<Type>> {
  static_assert(
      !std::is_same<ElemClass, type_class::unknown>(),
      "Unable to serialize unknown list element");

  constexpr static protocol::TType ttype_value = protocol::T_LIST;

  using elem_methods = protocol_methods<ElemClass, Type>;

 private:
  template <typename Protocol>
  static void consume_elem(Protocol& protocol, std::list<Type>& out) {
    Type tmp;
    elem_methods::read(protocol, tmp);
    out.push_back(std::move(tmp));
  }

 public:
  template <typename Protocol>
  static void read(Protocol& protocol, std::list<Type>& out) {
    std::uint32_t list_size = -1;
    TType reported_type = protocol::T_STOP;

    protocol.readListBegin(reported_type, list_size);
    if (protocol.kOmitsContainerSizes()) {
      // list size unknown, SimpleJSON protocol won't know type, either
      // so let's just hope that it spits out something that makes sense
      // (if it did set reported_type to something known)
      if (reported_type != protocol::T_STOP &&
          reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      while (protocol.peekList()) {
        consume_elem(protocol, out);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      while (list_size--) {
        consume_elem(protocol, out);
      }
    }
    protocol.readListEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, std::list<Type> const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeListBegin(elem_methods::ttype_value, out.size());
    for (auto const& elem : out) {
      xfer += elem_methods::write(protocol, elem);
    }
    xfer += protocol.writeListEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(
      Protocol& protocol,
      std::list<Type> const& out) {
    std::size_t xfer = 0;

    xfer +=
        protocol.serializedSizeListBegin(elem_methods::ttype_value, out.size());
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
    TType reported_type = protocol::T_STOP;

    protocol.readSetBegin(reported_type, set_size);
    if (protocol.kOmitsContainerSizes()) {
      while (protocol.peekSet()) {
        consume_elem(protocol, out);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      auto const vreader = [&protocol](auto& value) {
        elem_methods::read(protocol, value);
      };
      deserialize_known_length_set(out, set_size, vreader);
    }
    protocol.readSetEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& in) {
    std::size_t xfer = 0;

    xfer += protocol.writeSetBegin(elem_methods::ttype_value, in.size());
    for (auto const& elem : in) {
      xfer += elem_methods::write(protocol, elem);
    }
    xfer += protocol.writeSetEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer +=
        protocol.serializedSizeSetBegin(elem_methods::ttype_value, out.size());
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

 private:
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
    TType rpt_key_type = protocol::T_STOP, rpt_mapped_type = protocol::T_STOP;

    protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
    if (protocol.kOmitsContainerSizes()) {
      while (protocol.peekMap()) {
        consume_elem(protocol, out);
      }
    } else {
      // CompactProtocol does not transmit key/mapped types if
      // the map is empty
      if (map_size > 0 && (key_methods::ttype_value != rpt_key_type ||
                           mapped_methods::ttype_value != rpt_mapped_type)) {
        TProtocolException::throwReportedTypeMismatch();
      }
      auto const kreader = [&protocol](auto& key) {
        key_methods::read(protocol, key);
      };
      auto const vreader = [&protocol](auto& value) {
        mapped_methods::read(protocol, value);
      };
      deserialize_known_length_map(out, map_size, kreader, vreader);
    }
    protocol.readMapEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeMapBegin(
        key_methods::ttype_value, mapped_methods::ttype_value, out.size());
    for (auto const& elem_pair : out) {
      xfer += key_methods::write(protocol, elem_pair.first);
      xfer += mapped_methods::write(protocol, elem_pair.second);
    }
    xfer += protocol.writeMapEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.serializedSizeMapBegin(
        key_methods::ttype_value, mapped_methods::ttype_value, out.size());
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

namespace forward_compatibility {
// TODO(denplusplus): remove.
// DO NOT USE.
template <typename Protocol_, typename T>
void readIntegral(Protocol_& prot, TType arg_type, T& out) {
  switch (arg_type) {
    case TType::T_BOOL: {
      bool boolv;
      prot.readBool(boolv);
      out = static_cast<T>(boolv);
      return;
    }
    case TType::T_BYTE: {
      int8_t bytev;
      prot.readByte(bytev);
      out = static_cast<T>(bytev);
      return;
    }
    case TType::T_I16: {
      int16_t i16;
      prot.readI16(i16);
      out = static_cast<T>(i16);
      return;
    }
    case TType::T_I32: {
      int32_t i32;
      prot.readI32(i32);
      out = static_cast<T>(i32);
      return;
    }
    case TType::T_I64: {
      int64_t i64;
      prot.readI64(i64);
      out = static_cast<T>(i64);
      return;
    }
    default: {
      throw TProtocolException(
          std::string("Cannot parse integral number of ") +
          std::to_string(arg_type) + " type");
    }
  }
}

// TODO(denplusplus): remove.
// DO NOT USE.
template <typename Protocol_, typename T>
void readFloatingPoint(Protocol_& prot, TType arg_type, T& out) {
  switch (arg_type) {
    case TType::T_DOUBLE: {
      double dub;
      prot.readDouble(dub);
      out = static_cast<T>(dub);
      return;
    }
    case TType::T_FLOAT: {
      float flt;
      prot.readFloat(flt);
      out = static_cast<T>(flt);
      return;
    }
    default: {
      throw TProtocolException(
          std::string("Cannot parse floating number of ") +
          std::to_string(arg_type) + " type");
    }
  }
}

template <typename Protocol_, typename TypeClass_, typename TType_>
struct ReadForwardCompatible {
  static void read(Protocol_& prot, TType, TType_& out) {
    protocol_methods<TypeClass_, TType_>::read(prot, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, bool> {
  static void read(Protocol_& prot, TType arg_type, bool& out) {
    forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, int8_t> {
  static void read(Protocol_& prot, TType arg_type, int8_t& out) {
    forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, int16_t> {
  static void read(Protocol_& prot, TType arg_type, int16_t& out) {
    forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, int32_t> {
  static void read(Protocol_& prot, TType arg_type, int32_t& out) {
    forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, int64_t> {
  static void read(Protocol_& prot, TType arg_type, int64_t& out) {
    forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, float> {
  static void read(Protocol_& prot, TType arg_type, float& out) {
    forward_compatibility::readFloatingPoint(prot, arg_type, out);
  }
};

template <typename Protocol_, typename TypeClass_>
struct ReadForwardCompatible<Protocol_, TypeClass_, double> {
  static void read(Protocol_& prot, TType arg_type, double& out) {
    forward_compatibility::readFloatingPoint(prot, arg_type, out);
  }
};

} // namespace forward_compatibility

// TODO(@denplusplus, by 11/4/2017) Remove.
template <typename KeyClass, typename MappedClass, typename Type>
struct protocol_methods<
    type_class::map_forward_compatibility<KeyClass, MappedClass>,
    Type> {
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

 private:
  template <typename Protocol>
  static void consume_elem(Protocol& protocol, Type& out) {
    key_type key_tmp;
    key_methods::read(protocol, key_tmp);
    mapped_methods::read(protocol, out[std::move(key_tmp)]);
  }

  template <typename Protocol>
  static void consume_elem_forward_compatible(
      Protocol& protocol,
      TType keyType,
      TType valueType,
      Type& out) {
    key_type key_tmp;
    forward_compatibility::ReadForwardCompatible<Protocol, KeyClass, key_type>::
        read(protocol, keyType, key_tmp);
    forward_compatibility::
        ReadForwardCompatible<Protocol, MappedClass, mapped_type>::read(
            protocol, valueType, out[std::move(key_tmp)]);
  }

 public:
  template <typename Protocol>
  static void read(Protocol& protocol, Type& out) {
    std::uint32_t map_size = -1;
    TType rpt_key_type = protocol::T_STOP, rpt_mapped_type = protocol::T_STOP;

    protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
    if (protocol.kOmitsContainerSizes()) {
      // SimpleJSONProtocol do not save types and map length.
      if (rpt_key_type != protocol::T_STOP &&
          rpt_mapped_type != protocol::T_STOP) {
        while (protocol.peekMap()) {
          consume_elem_forward_compatible(
              protocol, rpt_key_type, rpt_mapped_type, out);
        }
      } else {
        while (protocol.peekMap()) {
          consume_elem(protocol, out);
        }
      }
    } else {
      auto const kreader = [&protocol, &rpt_key_type](auto& key) {
        forward_compatibility::
            ReadForwardCompatible<Protocol, KeyClass, key_type>::read(
                protocol, rpt_key_type, key);
      };
      auto const vreader = [&protocol, &rpt_mapped_type](auto& value) {
        forward_compatibility::
            ReadForwardCompatible<Protocol, MappedClass, mapped_type>::read(
                protocol, rpt_mapped_type, value);
      };
      deserialize_known_length_map(out, map_size, kreader, vreader);
    }
    protocol.readMapEnd();
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.writeMapBegin(
        key_methods::ttype_value, mapped_methods::ttype_value, out.size());
    for (auto const& elem_pair : out) {
      xfer += key_methods::write(protocol, elem_pair.first);
      xfer += mapped_methods::write(protocol, elem_pair.second);
    }
    xfer += protocol.writeMapEnd();
    return xfer;
  }

  template <bool ZeroCopy, typename Protocol>
  static std::size_t serializedSize(Protocol& protocol, Type const& out) {
    std::size_t xfer = 0;

    xfer += protocol.serializedSizeMapBegin(
        key_methods::ttype_value, mapped_methods::ttype_value, out.size());
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
template <typename Indirection, typename ElemClass, typename Type>
struct protocol_methods<IndirectionTag<Indirection, ElemClass>, Type> {
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
}
}
}
} // namespace apache::thrift::detail::pm
