/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp2/fatal/reflection.h>
#include <thrift/lib/cpp2/fatal/container_traits.h>
#include <vector>
#include <array>
#include <iostream>
#include <type_traits>
#include <iterator>

#include <fatal/type/string_lookup.h>
#include <fatal/type/call_traits.h>
#include <fatal/type/map.h>

namespace apache { namespace thrift {

namespace detail {
  inline bool is_unknown_container_size(uint32_t const size) {
    return size == std::numeric_limits<decltype(size)>::max();
  }
} // namespace detail {

/**
 * Specializations of `protocol_methods` encapsulate a collection of
 * read/write/size/sizeZC methods that can be performed on Thrift
 * objects and primitives. TypeClass (see apache::thrift::type_class)
 * refers to the general type of data that Type is, and is passed around for
 * two reasons:
 *  - to provide support for generic containers which have a common interface
 *    for building collections (e.g. a std::vector and std::deque, which
 *    can back a Thrift list, and thus have type_class::list)
 *  - to differentiate between Thrift types that are represented with the
 *    same C++ type, e.g. both Thrift binaries and strings are represented
 *    with an std::string, TypeClass is used to decide which protocol
 *    method to call.
 *
 * Example:
 *
 * // MyModule.thrift:
 * struct MyStruct {
 *   1: i32 fieldA
 *   2: string fieldB
 *   3: binary fieldC
 * }
 *
 * // C++
 *
 * using methods = protocol_methods<type_class::structure, MyStruct>;
 *
 * MyStruct struct_instance;
 * CompactProtocolReader reader;
 * methods::read(struct_instance, reader);
 *
 * @author: Dylan Knutson <dymk@fb.com>
 */
template <typename TypeClass, typename Type>
struct protocol_methods;

// stamp out specializations for primitive types
// TODO: Perhaps change ttype_value to a static constexpr member function, as
// those might instantiate faster than a constexpr objects
#define REGISTER_OVERLOAD(Class, Type, Method, TTypeValue) \
  template <> struct protocol_methods<type_class::Class, Type> { \
    constexpr static protocol::TType ttype_value = protocol::TTypeValue; \
    template <typename Protocol>              \
    static std::size_t read(Protocol& protocol, Type& out) {       \
      DLOG(INFO) << "read primitive " << #Class << ": " << #Type; \
      return protocol.read##Method(out); \
    } \
    template <typename Protocol>              \
    static std::size_t write(Protocol& protocol, Type const& in) { \
      return protocol.write##Method(in); \
    } \
  }

#define REGISTER_INTEGRAL(...) REGISTER_OVERLOAD(integral, __VA_ARGS__)
REGISTER_INTEGRAL(bool,         Bool,   T_BOOL);
REGISTER_INTEGRAL(std::int8_t,  Byte,   T_BYTE);
REGISTER_INTEGRAL(std::int16_t, I16,    T_I16);
REGISTER_INTEGRAL(std::int32_t, I32,    T_I32);
REGISTER_INTEGRAL(std::int64_t, I64,    T_I64);
#undef REGISTER_INTEGRAL

#define REGISTER_FP(...) REGISTER_OVERLOAD(floating_point, __VA_ARGS__)
REGISTER_FP(double,       Double, T_DOUBLE);
REGISTER_FP(float,        Float,  T_FLOAT);
#undef REGISTER_FP

// TODO: might not need to specialize on std::string, but rather
// just the string typeclass
REGISTER_OVERLOAD(string, std::string, String, T_STRING);

#define REGISTER_BINARY(...) REGISTER_OVERLOAD(binary, __VA_ARGS__)
REGISTER_BINARY(std::string,                   Binary, T_STRING);
REGISTER_BINARY(folly::IOBuf,                  Binary, T_STRING);
REGISTER_BINARY(std::unique_ptr<folly::IOBuf>, Binary, T_STRING);
#undef REGISTER_BINARY
#undef REGISTER_OVERLOAD

// special case: must dereference contents within unique_ptr
// TODO: probably also need to have a specialization for
// std::shared_ptr
template <typename TypeClass, typename Type>
struct protocol_methods<TypeClass, std::unique_ptr<Type>> {
  using pointer_type = std::unique_ptr<Type>;
  using type_methods = protocol_methods<TypeClass, Type>;
  constexpr static protocol::TType ttype_value = type_methods::ttype_value;

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, pointer_type& out) {
    return type_methods::read(protocol, *out);
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, pointer_type& in) {
    return type_methods::write(protocol, *in);
  }
};

// Enumerations
template<typename Type>
struct protocol_methods<type_class::enumeration, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_I32;

  using int_type = typename std::underlying_type<Type>::type;
  using int_methods = protocol_methods<type_class::integral, int_type>;

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    int_type tmp;
    std::size_t xfer = int_methods::template read<Protocol>(protocol, tmp);
    out = static_cast<Type>(tmp);
    return xfer;
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type& in) {
    int_type tmp = static_cast<int_type>(in);
    return int_methods::template write<Protocol>(protocol, tmp);
  }
};

// Lists
template<typename ElemClass, typename Type>
struct protocol_methods<type_class::list<ElemClass>, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_LIST;

  using elem_type    = typename Type::value_type;
  using elem_tclass   = ElemClass;
  static_assert(!std::is_same<elem_tclass, type_class::unknown>(),
    "Unable to serialize unknown list element");

  using elem_proto_methods = protocol_methods<elem_tclass, elem_type>;

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    std::uint32_t list_size = -1;
    TType reported_type = protocol::T_STOP;

    out = Type();

    protocol.readListBegin(reported_type, list_size);

    Type list;
    if(detail::is_unknown_container_size(list_size)) {
      // list size unknown, SimpleJSON protocol won't know type, either
      // so let's just hope that it spits out something that makes sense
      // (if it did set reported_type to something known)
      if(reported_type != protocol::T_STOP) {
        assert(reported_type == elem_proto_methods::ttype_value);
      }

      for(int i = 0; protocol.peekList(); i++) {
        // TODO: this should grow better (e.g., 1.5x each time)
        // For now, we just bump the container size by 1 each time
        // because that's what the currently generated Thrift code does.
        out.resize(i+1);
        xfer += elem_proto_methods::read(protocol, out[i]);
      }
    } else {
      assert(reported_type == elem_proto_methods::ttype_value);
      out.resize(list_size);
      for(int i = 0; i < list_size; i++) {
        xfer += elem_proto_methods::read(protocol, out[i]);
      }
    }

    xfer += protocol.readListEnd();
    return xfer;
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type& out) {
    assert(false);
    return 0;
  }
};

// Sets
template <typename ElemClass, typename Type>
struct protocol_methods<type_class::set<ElemClass>, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_SET;

  // TODO: fair amount of shared code bewteen this and specialization for
  // type_class::list
  using elem_type   = typename Type::value_type;
  using elem_tclass = ElemClass;
  static_assert(!std::is_same<elem_tclass, type_class::unknown>(),
    "Unable to serialize unknown type");
  using elem_proto_methods = protocol_methods<elem_tclass, elem_type>;

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Type& out) {
    std::size_t xfer = 0;
    // TODO: readSetBegin takes (TType, std::uint32_t)
    // instead of a (TType, std::size_t)
    std::uint32_t set_size = -1;
    TType reported_type = protocol::T_STOP;

    const auto consume_elem = [&protocol, &xfer, &out]() {
      elem_type tmp;
      xfer += elem_proto_methods::template read<Protocol>(protocol, tmp);
      out.insert(std::move(tmp));
    };

    out = Type();
    xfer += protocol.readSetBegin(reported_type, set_size);
    if(detail::is_unknown_container_size(set_size)) {
      while(protocol.peekSet()) {
        consume_elem();
      }
    } else {
      assert(reported_type == elem_proto_methods::ttype_value);
      for(int i = 0; i < set_size; i++) {
        consume_elem();
      }
    }

    xfer += protocol.readSetEnd();
    return xfer;
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Type& in) {
    assert(false);
    return 0;
  }
};

// Maps
template <typename KeyClass, typename MappedClass, typename Type>
struct protocol_methods<type_class::map<KeyClass, MappedClass>, Type> {
  constexpr static protocol::TType ttype_value = protocol::T_MAP;

  using key_type    = typename Type::key_type;
  using key_tclass  = KeyClass;

  using mapped_type   = typename Type::mapped_type;
  using mapped_tclass = MappedClass;

  static_assert(!std::is_same<key_tclass, type_class::unknown>(),
    "Unable to serialize unknown key type in map");
  static_assert(!std::is_same<mapped_tclass, type_class::unknown>(),
    "Unable to serialize unknown mapped type in map");

    using key_methods    = protocol_methods<key_tclass, key_type>;
    using mapped_methods = protocol_methods<mapped_tclass, mapped_type>;

    template <typename Protocol>
    static std::size_t read(Protocol& protocol, Type& out) {
      std::size_t xfer = 0;
      // TODO: see above re: readMapBegin taking a uint32_t size param
      std::uint32_t map_size = -1;
      TType rpt_key_type = protocol::T_STOP, rpt_mapped_type = protocol::T_STOP;
      out = Type();

      const auto consume_elem = [&protocol, &xfer, &out]() {
        key_type key_tmp;
        xfer += key_methods::template read<>(protocol, key_tmp);
        xfer += mapped_methods::
          template read<>(protocol, out[std::move(key_tmp)]);
      };

      xfer += protocol.readMapBegin(rpt_key_type, rpt_mapped_type, map_size);
      if(detail::is_unknown_container_size(map_size)) {
        while(protocol.peekMap()) {
          consume_elem();
        }
      } else {
        assert(key_methods::ttype_value == rpt_key_type);
        assert(mapped_methods::ttype_value == rpt_mapped_type);
        for(int i = 0; i < map_size; i++) {
          consume_elem();
        }
      }

      xfer += protocol.readMapEnd();
      return xfer;
    }

    template <typename Protocol>
    static std::size_t write(Protocol& protocol, Type& out) {
      assert(false);
      return 0;
    }
};

namespace detail {
  // helper predicate for determining if a struct's MemberInfo is required
  // to be read out of the protocol
  template <typename MemberInfo>
  struct is_required_field :
    std::integral_constant<
      bool,
      MemberInfo::optional::value == optionality::required
    > {};

  // marks isset either on the required field array,
  // or the appropriate member within the Struct being read
  template <
    optionality opt,
    typename,
    typename isset_array,
    typename Struct,
    typename MemberInfo
  >
  typename std::enable_if<opt != optionality::required>::type
  mark_isset(isset_array& /*isset*/, Struct& obj) {
    MemberInfo::mark_set(obj);
  }

  template <
    optionality opt,
    typename required_fields,
    typename isset_array,
    typename Struct,
    typename MemberInfo
  >
  typename std::enable_if<opt == optionality::required>::type
  mark_isset(isset_array& isset, Struct& /*obj*/) {
    const std::size_t fid_idx =
      required_fields::template index_of<MemberInfo>::value;
    static_assert(fid_idx != required_fields::size,
      "internal error: didn't find reqired field");
    assert(fid_idx < required_fields::size);
    isset[fid_idx] = true;
  }

  template <typename T>
  using extract_descriptor_fid = typename T::metadata::id;
} /* namespace detail */


// specialization for variants (Thrift unions)
template <typename Union>
struct protocol_methods<type_class::variant, Union> {
  constexpr static protocol::TType ttype_value = protocol::T_STRUCT; // overlaps

  // using traits = reflect_union<Union>;
  using traits = fatal::variant_traits<Union>;
  using enum_traits = fatal::enum_traits<typename traits::id>;
  using id_name_strs = typename enum_traits::name_to_value::keys;
  using sorted_fids = typename traits::
    descriptors::
    template transform<detail::extract_descriptor_fid>::
    template sort<>;
  using fid_to_descriptor_map = typename fatal
    ::template type_map_from<detail::extract_descriptor_fid>
    ::list<typename traits::descriptors>;

  // Visitor, mapping member fname -> fid & ftype
  struct member_match_op {
    template <typename TString>
    void operator ()(
      fatal::type_tag<TString>,
      field_id_t& fid,
      protocol::TType& ftype)
    {
      using enum_value = typename enum_traits
        ::name_to_value
        ::template get<TString>;
      using descriptor = typename traits::by_id::map::template get<enum_value>;

      using field_type   = typename descriptor::type;
      using field_tclass = typename descriptor::metadata::type_class;

      static_assert(!std::is_same<field_tclass, type_class::unknown>(),
        "Instantiation failure, unknown field type");
      static_assert(
        std::is_same<typename descriptor::metadata::name, TString>(),
        "Instantiation failure, descriptor name mismatch");

      DLOG(INFO) << "(union) matched string: " << TString::string()
        << ", fid: " << fid
        << ", ftype: " << ftype;

      fid   = descriptor::metadata::id::value;
      ftype = protocol_methods<field_tclass, field_type>::ttype_value;
    }
  };

  // Visitor, match on member field id -> set that field
  template <typename Protocol>
  struct member_id_match_op {
    std::size_t& xfer;
    Protocol& protocol;
    Union& obj;

    member_id_match_op(std::size_t& xfer_, Protocol& protocol_, Union& obj_) :
      xfer(xfer_),
      protocol(protocol_),
      obj(obj_)
      {}

    // Fid is a std::integral_constant<field_id_t, fid>
    template <typename Fid, std::size_t Index>
    void operator ()(
      fatal::indexed_type_tag<Fid, Index>,
      const field_id_t needle,
      const TType ftype)
    {
      using descriptor = typename fid_to_descriptor_map::template get<Fid>;
      constexpr field_id_t fid = Fid::value;
      assert(needle == fid);

      DLOG(INFO) << "matched needle: " << fid;

      using field_methods = protocol_methods<
          typename descriptor::metadata::type_class,
          typename descriptor::type>;

      if(ftype == field_methods::ttype_value) {
        typename descriptor::type tmp;
        typename descriptor::setter s;
        xfer += field_methods::template read<Protocol>(protocol, tmp);
        s(obj, std::move(tmp));
      } else {
        xfer += protocol.skip(ftype);
      }
    }
  };

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Union& out) {
    field_id_t fid = -1;
    std::size_t xfer = 0;
    protocol::TType ftype = protocol::T_STOP;
    std::string fname;

    xfer += protocol.readStructBegin(fname);

    DLOG(INFO) << "began reading union: " << fname;
    xfer += protocol.readFieldBegin(fname, ftype, fid);
    if(ftype == protocol::T_STOP) {
      out.__clear();
    } else {
      // fid might not be known, such as in the case of the SimpleJSON protocol
      if(fid == std::numeric_limits<int16_t>::min()) {
        // if so, look up fid via fname
        assert(fname != "");
        auto found_ = id_name_strs
          ::template apply<fatal::string_lookup>
          ::template match<>::exact(
          fname.begin(), fname.end(), member_match_op(), fid, ftype
        );
        assert(found_ == 1);
      }

      if(!sorted_fids::
        template binary_search<>::
        exact(fid, member_id_match_op<Protocol>(xfer, protocol, out), ftype))
      {
        DLOG(INFO) << "didn't find field, fid: " << fid;
        xfer += protocol.skip(ftype);
      }

      xfer += protocol.readFieldEnd();
      xfer += protocol.readFieldBegin(fname, ftype, fid);
      if(UNLIKELY(ftype != protocol::T_STOP)) {
        // TODO: TProtocolException::throwUnionMissingStop() once D3474801 lands
        xfer += protocol.readFieldEnd();
      }
      xfer += protocol.readStructEnd();
    }
    return xfer;
  }

  template <typename Protocol>
  static std::size_t write(Protocol&, Union&) {
    assert(false);
    return 0;
  }
};

// specialization for structs
template <typename Struct>
struct protocol_methods<type_class::structure, Struct> {
  constexpr static protocol::TType ttype_value = protocol::T_STRUCT;

  using traits = apache::thrift::reflect_struct<Struct>;

  // fatal::type_list
  using member_names = typename traits::members::keys;
  using sorted_fids  = typename traits::members::mapped::
    template transform<fatal::get_member_type::id>::
    template sort<>;

  // fatal::prefix_tree
  using member_matcher =
    typename member_names::
    template apply<fatal::string_lookup>;

  // fatal::type_list
  using required_fields = typename
    traits::members::mapped::template filter<detail::is_required_field>;
  using optional_fields = typename
    traits::members::mapped::template reject<detail::is_required_field>;

  // fatal::type_map {std::integral_constant<field_id_t, Id> => MemberInfo}
  using id_to_member_map =
    typename fatal::type_map_from<fatal::get_member_type::id>::list<
      typename traits::members::mapped>;

  using isset_array = std::array<bool, required_fields::size>;

  // mapping member fname -> fid
  struct member_match_op {
    template <typename TString>
    void operator ()(
      fatal::type_tag<TString>,
      field_id_t& fid,
      protocol::TType& ftype) {
      using member = typename traits::members::template get<TString>;
      fid = member::id::value;
      ftype = protocol_methods<
        typename member::type_class,
        typename member::type
      >::ttype_value;

      DLOG(INFO) << "matched string: " << TString::string()
        << ", fid: " << fid
        << ", ftype: " << ftype;
    }
  };

  // match on member field id -> set that field
  template <typename Protocol>
  struct member_id_match_op {
    std::size_t& xfer;
    Protocol& protocol;
    Struct& obj;
    isset_array& required_isset;

    member_id_match_op(std::size_t& xfer_, Protocol& protocol_,
      Struct& obj_, isset_array& required_isset_) :
      xfer(xfer_),
      protocol(protocol_),
      obj(obj_),
      required_isset(required_isset_)
      {}

    // Fid is a std::integral_constant<field_id_t, fid>
    template <typename Fid, std::size_t Index>
    void operator ()(
      fatal::indexed_type_tag<Fid, Index>,
      const field_id_t needle,
      const TType ftype)
    {
      constexpr field_id_t
            fid    = Fid::value;
      using member = typename id_to_member_map::template get<Fid>;
      using getter = typename traits::getters::
        template get<typename member::name>;
      assert(needle == fid);

      DLOG(INFO) << "matched needle: " << fid;

      using protocol_method = protocol_methods<
        typename member::type_class,
        typename member::type
      >;

      if(ftype == protocol_method::ttype_value) {
        detail::mark_isset<
            member::optional::value,
            required_fields,
            isset_array,
            Struct,
            member>(required_isset, obj);
        xfer += protocol_method
          ::template read<Protocol>(protocol, getter::ref(obj));
      } else {
        xfer += protocol.skip(ftype);
      }
    }
  };

  template <typename Protocol>
  static std::size_t read(Protocol& protocol, Struct& out) {
    std::size_t xfer = 0;
    std::string fname;
    apache::thrift::protocol::TType ftype = protocol::T_STOP;
    std::int16_t fid = -1;
    isset_array required_isset = {};

    xfer += protocol.readStructBegin(fname);
    DLOG(INFO) << "start reading struct: " << fname;

    while(true) {
      xfer += protocol.readFieldBegin(fname, ftype, fid);
      DLOG(INFO) << "type: " << ftype
                 << ", fname: " << fname
                 << ", fid: " << fid;

      if(ftype == apache::thrift::protocol::T_STOP) {
        break;
      }

      // fid might not be known, such as in the case of SimpleJSON protocol
      if(fid == std::numeric_limits<int16_t>::min()) {
        // if so, look up fid via fname
        assert(fname != "");
        auto found_ = member_matcher::template match<>::exact(
          fname.begin(), fname.end(), member_match_op(), fid, ftype
        );
        assert(found_ == 1);
      }

      member_id_match_op<Protocol> member_fid_visitor(
        xfer, protocol, out, required_isset);

      if(!sorted_fids::
        template binary_search<>::
        exact(fid, member_fid_visitor, ftype))
      {
        DLOG(INFO) << "didn't find field, fid: " << fid
                   << ", fname: " << fname;
        xfer += protocol.skip(ftype);
      }

      xfer += protocol.readFieldEnd();
    }

    for(bool matched : required_isset) {
      if(!matched) {
        throw apache::thrift::TProtocolException(
          TProtocolException::MISSING_REQUIRED_FIELD,
          "Required field was not found in serialized data!");
      }
    }

    xfer += protocol.readStructEnd();
    return xfer;
  }

  template <typename Protocol>
  static std::size_t write(Protocol& protocol, Struct& in) {
    assert(false);
    return 0;
  }
};


/**
 * Entrypoints for using new serialization methods
 *
 * // C++
 * MyStruct a;
 * MyUnion b;
 * CompactProtocolReader reader;
 * CompactProtocolReader writer;
 *
 * serializer_read(a, reader);
 * serializer_write(b, writer);
 *
 * @author: Dylan Knutson <dymk@fb.com>
 */

template <typename Type, typename Protocol>
std::size_t serializer_read(Type& out, Protocol& protocol) {
  return protocol_methods<reflect_type_class<Type>, Type>::
    template read<>(protocol, out);
}

template <typename Type, typename Protocol>
std::size_t serializer_write(Type const& in, Protocol& protocol) {
  return protocol_methods<reflect_type_class<Type>, Type>::
    template write<>(protocol, in);
}

} } /* namespace apache::thrift */
