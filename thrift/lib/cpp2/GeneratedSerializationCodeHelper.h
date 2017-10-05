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

inline bool is_unknown_container_size(uint32_t const size) {
  return size == std::numeric_limits<decltype(size)>::max();
}

template <typename T>
inline auto reserve_if_possible(T* t, std::uint32_t size)
    -> decltype(t->reserve(size), void()) {
  // Resize to `size + 1` to avoid an extra rehash:
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71181.
  t->reserve(size + 1);
}

inline void reserve_if_possible(...){};

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
  static std::size_t read(Protocol& protocol, Type& out) {             \
    return protocol.read##Method(out);                                 \
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
  static std::size_t read(
      Protocol& protocol,
      std::vector<bool>::reference out) {
    bool tmp;
    std::size_t xfer = read(protocol, tmp);
    out = tmp;
    return xfer;
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
  static std::size_t read(Protocol& protocol, Type& out) {
    int_type tmp;
    std::size_t xfer = int_methods::read(protocol, tmp);
    out = static_cast<Type>(tmp);
    return xfer;
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
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    std::uint32_t list_size = -1;
    TType reported_type = protocol::T_STOP;

    xfer += protocol.readListBegin(reported_type, list_size);
    if (is_unknown_container_size(list_size)) {
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
        xfer += elem_methods::read(protocol, out[i]);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      out.resize(list_size);
      for (std::uint32_t i = 0; i < list_size; ++i) {
        xfer += elem_methods::read(protocol, out[i]);
      }
    }
    xfer += protocol.readListEnd();
    return xfer;
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
  static std::size_t consume_elem(Protocol& protocol, std::list<Type>& out) {
    Type tmp;
    std::size_t xfer = elem_methods::read(protocol, tmp);
    out.push_back(std::move(tmp));
    return xfer;
  }

 public:
  template <typename Protocol>
  static std::size_t read(Protocol& protocol, std::list<Type>& out) {
    std::size_t xfer = 0;
    std::uint32_t list_size = -1;
    TType reported_type = protocol::T_STOP;

    xfer += protocol.readListBegin(reported_type, list_size);
    if (is_unknown_container_size(list_size)) {
      // list size unknown, SimpleJSON protocol won't know type, either
      // so let's just hope that it spits out something that makes sense
      // (if it did set reported_type to something known)
      if (reported_type != protocol::T_STOP &&
          reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      while (protocol.peekList()) {
        xfer += consume_elem(protocol, out);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      while (list_size--) {
        xfer += consume_elem(protocol, out);
      }
    }
    xfer += protocol.readListEnd();
    return xfer;
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
  static std::size_t consume_elem(Protocol& protocol, Type& out) {
    elem_type tmp;
    std::size_t xfer = elem_methods::read(protocol, tmp);
    out.insert(std::move(tmp));
    return xfer;
  }

 public:
  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    std::uint32_t set_size = -1;
    TType reported_type = protocol::T_STOP;

    xfer += protocol.readSetBegin(reported_type, set_size);
    if (is_unknown_container_size(set_size)) {
      while (protocol.peekSet()) {
        xfer += consume_elem(protocol, out);
      }
    } else {
      if (reported_type != elem_methods::ttype_value) {
        TProtocolException::throwReportedTypeMismatch();
      }
      reserve_if_possible(&out, set_size);
      while (set_size--) {
        xfer += consume_elem(protocol, out);
      }
    }
    xfer += protocol.readSetEnd();
    return xfer;
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
  static std::size_t consume_elem(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    key_type key_tmp;
    xfer += key_methods::read(protocol, key_tmp);
    xfer += mapped_methods::read(protocol, out[std::move(key_tmp)]);
    return xfer;
  }

 public:
  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    std::uint32_t map_size = -1;
    TType rpt_key_type = protocol::T_STOP, rpt_mapped_type = protocol::T_STOP;

    xfer += protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
    if (is_unknown_container_size(map_size)) {
      while (protocol.peekMap()) {
        xfer += consume_elem(protocol, out);
      }
    } else {
      // CompactProtocol does not transmit key/mapped types if
      // the map is empty
      if (map_size > 0 && (key_methods::ttype_value != rpt_key_type ||
                           mapped_methods::ttype_value != rpt_mapped_type)) {
        TProtocolException::throwReportedTypeMismatch();
      }
      auto const kreader = [&xfer, &protocol](auto& key) {
        xfer += key_methods::read(protocol, key);
      };
      auto const vreader = [&xfer, &protocol](auto& value) {
        xfer += mapped_methods::read(protocol, value);
      };
      reserve_if_possible(&out, map_size);
      deserialize_known_length_map(out, map_size, kreader, vreader);
    }
    xfer += protocol.readMapEnd();
    return xfer;
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
uint32_t readIntegral(Protocol_& prot, TType arg_type, T& out) {
  switch (arg_type) {
    case TType::T_BOOL: {
      bool boolv;
      auto res = prot.readBool(boolv);
      out = static_cast<T>(boolv);
      return res;
    }
    case TType::T_BYTE: {
      int8_t bytev;
      auto res = prot.readByte(bytev);
      out = static_cast<T>(bytev);
      return res;
    }
    case TType::T_I16: {
      int16_t i16;
      auto res = prot.readI16(i16);
      out = static_cast<T>(i16);
      return res;
    }
    case TType::T_I32: {
      int32_t i32;
      auto res = prot.readI32(i32);
      out = static_cast<T>(i32);
      return res;
    }
    case TType::T_I64: {
      int64_t i64;
      auto res = prot.readI64(i64);
      out = static_cast<T>(i64);
      return res;
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
uint32_t readFloatingPoint(Protocol_& prot, TType arg_type, T& out) {
  switch (arg_type) {
    case TType::T_DOUBLE: {
      double dub;
      auto res = prot.readDouble(dub);
      out = static_cast<T>(dub);
      return res;
    }
    case TType::T_FLOAT: {
      float flt;
      auto res = prot.readFloat(flt);
      out = static_cast<T>(flt);
      return res;
    }
    default: {
      throw TProtocolException(
          std::string("Cannot parse floating number of ") +
          std::to_string(arg_type) + " type");
    }
  }
}

template <typename Protocol_, typename T>
struct ReadForwardCompatible {
  static uint32_t read(Protocol_& prot, TType arg_type, T& out) {
    return forward_compatibility::readIntegral(prot, arg_type, out);
  }
};

template <typename Protocol_>
struct ReadForwardCompatible<Protocol_, float> {
  static uint32_t read(Protocol_& prot, TType arg_type, float& out) {
    return forward_compatibility::readFloatingPoint(prot, arg_type, out);
  }
};

template <typename Protocol_>
struct ReadForwardCompatible<Protocol_, double> {
  static uint32_t read(Protocol_& prot, TType arg_type, double& out) {
    return forward_compatibility::readFloatingPoint(prot, arg_type, out);
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
  static std::size_t consume_elem(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    key_type key_tmp;
    xfer += key_methods::read(protocol, key_tmp);
    xfer += mapped_methods::read(protocol, out[std::move(key_tmp)]);
    return xfer;
  }

  template <typename Protocol>
  static std::size_t consume_elem_forward_compatible(
      Protocol& protocol,
      TType keyType,
      TType valueType,
      Type& out) {
    std::size_t xfer = 0;
    key_type key_tmp;
    xfer +=
        forward_compatibility::ReadForwardCompatible<Protocol, key_type>::read(
            protocol, keyType, key_tmp);
    xfer +=
        forward_compatibility::ReadForwardCompatible<Protocol, mapped_type>::
            read(protocol, valueType, out[std::move(key_tmp)]);
    return xfer;
  }

 public:
  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    std::uint32_t map_size = -1;
    TType rpt_key_type = protocol::T_STOP, rpt_mapped_type = protocol::T_STOP;

    xfer += protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
    if (is_unknown_container_size(map_size)) {
      // SimpleJSONProtocol do not save types and map length.
      if (rpt_key_type != protocol::T_STOP &&
          rpt_mapped_type != protocol::T_STOP) {
        while (protocol.peekMap()) {
          xfer += consume_elem_forward_compatible(
              protocol, rpt_key_type, rpt_mapped_type, out);
        }
      } else {
        while (protocol.peekMap()) {
          xfer += consume_elem(protocol, out);
        }
      }
    } else {
      auto const kreader = [&xfer, &protocol, &rpt_key_type](auto& key) {
        xfer +=
            forward_compatibility::ReadForwardCompatible<Protocol, key_type>::
                read(protocol, rpt_key_type, key);
      };
      auto const vreader = [&xfer, &protocol, &rpt_mapped_type](auto& value) {
        xfer += forward_compatibility::
            ReadForwardCompatible<Protocol, mapped_type>::read(
                protocol, rpt_mapped_type, value);
      };
      reserve_if_possible(&out, map_size);
      deserialize_known_length_map(out, map_size, kreader, vreader);
    }
    xfer += protocol.readMapEnd();
    return xfer;
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
  static std::size_t read(Protocol& protocol, Type& out) {
    return elem_methods::read(protocol, indirection::template get<Type&>(out));
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
  static std::size_t read(Protocol& protocol, Type& out) {
    return Cpp2Ops<Type>::read(&protocol, &out);
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
