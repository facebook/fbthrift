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

namespace apache {
namespace thrift {

namespace legacy_reflection_detail {

using namespace folly::string_literals;

using id_t = legacy_reflection_id_t;
using datatype_t = reflection::DataType;
using schema_t = legacy_reflection_schema_t;
using type_t = reflection::Type;

// utils

template <
    typename S,
    typename N = typename S::name,
    typename R = folly::BasicFixedString<char, fatal::size<N>::value>>
constexpr R fs() {
  return fatal::to_instance<R, N>();
}

inline datatype_t* registering_datatype_prep(
    schema_t& schema,
    folly::StringPiece rname,
    id_t rid) {
  auto& dt = schema.dataTypes_ref()[rid];
  if (!dt.name_ref()->empty()) {
    return nullptr; // this datatype has already been registered
  }
  *dt.name_ref() = rname.str();
  schema.names_ref()[*dt.name_ref()] = rid;
  return &dt;
}

template <typename F>
void registering_datatype(
    schema_t& schema,
    folly::StringPiece rname,
    id_t rid,
    F&& f) {
  if (auto* dt = registering_datatype_prep(schema, rname, rid)) {
    f(*dt);
  }
}

template <typename T>
struct deref {
  using type = T;
};
template <typename T, typename D>
struct deref<std::unique_ptr<T, D>> : deref<T> {};
template <typename T>
struct deref<std::shared_ptr<T>> : deref<T> {};
template <typename T>
struct deref<std::shared_ptr<T const>> : deref<T> {};
template <typename T>
using deref_t = folly::_t<deref<T>>;
template <typename T>
using deref_inner_t = deref_t<folly::remove_cvref_t<T>>;

// helper
//
// The impl::go functions may recurse to other impl::go functions, but only
// indirectly through legacy_reflection<T>::register_into, which calls this
// helper for all the assertions. This permits explicit template instantiations
// of legacy_reflection to reduce the overall template recursion depth.

template <typename T, typename TC, typename = void>
struct helper;

//  get_helper
//
//  Helps with breaking recursion by permitting extern template instances of
//  legacy_reflection.
template <typename T, typename TC>
using get_helper = folly::conditional_t<
    std::is_same_v<TC, type_class::structure>,
    legacy_reflection<T>,
    folly::conditional_t<
        std::is_same_v<TC, type_class::variant>,
        legacy_reflection<T>,
        helper<T, TC>>>;

// impl
//
// The workhorse. Specialized per type-class.

template <typename T, typename TypeClass>
struct impl;

template <>
struct impl<void, type_class::nothing> {
  static constexpr auto rname = "void"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_VOID);
  }
  static void go(schema_t&) {}
};

template <>
struct impl<bool, type_class::integral> {
  static constexpr auto rname = "bool"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_BOOL);
  }
  static void go(schema_t&) {}
};

template <std::size_t I>
struct impl_integral;

template <>
struct impl_integral<1> {
  static constexpr auto rname = "byte"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_BYTE);
  }
  static void go(schema_t&) {}
};

template <>
struct impl_integral<2> {
  static constexpr auto rname = "i16"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_I16);
  }
  static void go(schema_t&) {}
};

template <>
struct impl_integral<4> {
  static constexpr auto rname = "i32"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_I32);
  }
  static void go(schema_t&) {}
};

template <>
struct impl_integral<8> {
  static constexpr auto rname = "i64"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_I64);
  }
  static void go(schema_t&) {}
};

template <typename I>
struct impl<I, type_class::integral> : impl_integral<sizeof(I)> {};

template <>
struct impl<double, type_class::floating_point> {
  static constexpr auto rname = "double"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_DOUBLE);
  }
  static void go(schema_t&) {}
};
template <>
struct impl<float, type_class::floating_point> {
  static constexpr auto rname = "float"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_FLOAT);
  }
  static void go(schema_t&) {}
};

template <typename T>
struct impl<T, type_class::binary> {
  static constexpr auto rname = "string"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_STRING);
  }
  static void go(schema_t&) {}
};

template <typename T>
struct impl<T, type_class::string> {
  static constexpr auto rname = "string"_fs;
  static id_t rid() {
    return id_t(type_t::TYPE_STRING);
  }
  static void go(schema_t&) {}
};

template <typename T>
struct impl<T, type_class::enumeration> {
  using meta = reflect_enum<T>;
  using meta_traits = typename meta::traits;
  using module_meta = reflect_module<typename meta::module>;
  static constexpr auto rname =
      "enum "_fs + fs<module_meta>() + "." + fs<meta_traits>();
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_ENUM, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    using traits = TEnumTraits<T>;
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      apache::thrift::ensure_isset_unsafe(dt.enumValues_ref());
      for (size_t i = 0; i < traits::size; ++i) {
        (*dt.enumValues_ref())[traits::names[i].str()] = int(traits::values[i]);
      }
    });
  }
};

struct impl_structure_util {
  static void init(
      reflection::StructField& field,
      id_t type,
      size_t index,
      optionality opt,
      std::string& name,
      size_t n_annots) {
    *field.isRequired_ref() = opt != optionality::optional;
    *field.type_ref() = type;
    *field.name_ref() = name;
    *field.order_ref() = index;
    auto annotations = field.annotations_ref();
    if (n_annots > 0) {
      annotations = {};
    } else {
      annotations.reset();
    }
  }
};

template <typename T>
struct impl<T, type_class::structure> {
  using meta = reflect_struct<T>;
  using module_meta = reflect_module<typename meta::module>;
  struct visitor {
    template <typename MemberInfo, size_t Index>
    void operator()(
        fatal::indexed<MemberInfo, Index>,
        schema_t& schema,
        datatype_t& dt) {
      using getter = typename MemberInfo::getter;
      using type = deref_inner_t<decltype(getter::ref(std::declval<T&>()))>;
      using type_class = typename MemberInfo::type_class;
      using type_helper = get_helper<type, type_class>;
      using member_name = typename MemberInfo::name;
      using member_annotations = typename MemberInfo::annotations::map;
      type_helper::register_into(schema);
      auto& f = (*dt.fields_ref())[MemberInfo::id::value];
      std::string name = fatal::to_instance<std::string, member_name>();
      impl_structure_util::init(
          f,
          type_helper::id(),
          Index,
          MemberInfo::optional::value,
          name,
          fatal::size<member_annotations>());
      auto annotations = f.annotations_ref();
      fatal::foreach<member_annotations>([&](auto tag) {
        using annotation = decltype(fatal::tag_type(tag));
        annotations->emplace(
            fatal::to_instance<std::string, typename annotation::key>(),
            fatal::to_instance<std::string, typename annotation::value>());
      });
    }
  };
  static constexpr auto rname =
      "struct "_fs + fs<module_meta>() + "." + fs<meta>();
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_STRUCT, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      apache::thrift::ensure_isset_unsafe(dt.fields_ref());
      fatal::foreach<typename meta::members>(visitor(), schema, dt);
    });
  }
};

template <typename T>
struct impl<T, type_class::variant> {
  using meta = reflect_variant<T>;
  using meta_traits = typename meta::traits;
  using module_meta = reflect_module<typename meta::module>;
  struct visitor {
    template <typename MemberInfo, size_t Index>
    void operator()(
        fatal::indexed<MemberInfo, Index>,
        schema_t& schema,
        datatype_t& dt) {
      typename MemberInfo::getter getter;
      using type = deref_inner_t<decltype(getter(std::declval<T&>()))>;
      using type_class = typename MemberInfo::metadata::type_class;
      using type_helper = get_helper<type, type_class>;
      using member_name = typename MemberInfo::metadata::name;
      type_helper::register_into(schema);
      auto& f = (*dt.fields_ref())[MemberInfo::metadata::id::value];
      *f.isRequired_ref() = true;
      *f.type_ref() = type_helper::id();
      *f.name_ref() = fatal::to_instance<std::string, member_name>();
      *f.order_ref() = Index;
    }
  };
  static constexpr auto rname =
      "struct "_fs + fs<module_meta>() + "." + fs<meta_traits>();
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_STRUCT, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      apache::thrift::ensure_isset_unsafe(dt.fields_ref());
      fatal::foreach<typename meta::traits::descriptors>(visitor(), schema, dt);
    });
  }
};

template <typename T, typename ValueTypeClass>
struct impl<T, type_class::list<ValueTypeClass>> {
  using traits = thrift_list_traits<T>;
  using value_type = typename traits::value_type;
  using value_helper = get_helper<value_type, ValueTypeClass>;
  static constexpr auto rname = "list<"_fs + value_helper::name() + ">";
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_LIST, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      dt.valueType_ref() = value_helper::id();
      legacy_reflection<value_type>::register_into(schema);
    });
  }
};

template <typename T, typename ValueTypeClass>
struct impl<T, type_class::set<ValueTypeClass>> {
  using traits = thrift_set_traits<T>;
  using value_type = typename traits::value_type;
  using value_helper = get_helper<value_type, ValueTypeClass>;
  static constexpr auto rname = "set<"_fs + value_helper::name() + ">";
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_SET, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      dt.valueType_ref() = value_helper::id();
      legacy_reflection<value_type>::register_into(schema);
    });
  }
};

template <typename T, typename KeyTypeClass, typename MappedTypeClass>
struct impl<T, type_class::map<KeyTypeClass, MappedTypeClass>> {
  using traits = thrift_map_traits<T>;
  using key_type = typename traits::key_type;
  using key_helper = get_helper<key_type, KeyTypeClass>;
  using mapped_type = typename traits::mapped_type;
  using mapped_helper = get_helper<mapped_type, MappedTypeClass>;
  static constexpr auto rname =
      "map<"_fs + key_helper::name() + ", " + mapped_helper::name() + ">";
  static id_t rid() {
    static const auto storage = get_type_id(type_t::TYPE_MAP, rname);
    return storage;
  }
  static void go(schema_t& schema) {
    registering_datatype(schema, rname, rid(), [&](datatype_t& dt) {
      dt.mapKeyType_ref() = key_helper::id();
      dt.valueType_ref() = mapped_helper::id();
      legacy_reflection<key_type>::register_into(schema);
      legacy_reflection<mapped_type>::register_into(schema);
    });
  }
};

template <typename T, typename TC>
using is_unknown = folly::bool_constant<
    std::is_same<TC, type_class::unknown>::value ||
    (std::is_same<reflect_type_class<T>, type_class::unknown>::value &&
     (std::is_same<TC, type_class::enumeration>::value ||
      std::is_same<TC, type_class::structure>::value ||
      std::is_same<TC, type_class::variant>::value))>;

template <typename T, typename TC>
using is_known = folly::bool_constant<!is_unknown<T, TC>::value>;

template <typename T, typename TC>
using is_complete = fatal::is_complete<impl<T, TC>>;

// helper

template <typename T, typename TC>
struct helper<
    T,
    TC,
    typename std::enable_if<
        is_known<T, TC>::value && is_complete<T, TC>::value>::type> {
  using type_impl = impl<T, TC>;
  static void register_into(schema_t& schema) {
    type_impl::go(schema);
  }
  static constexpr auto name() {
    return type_impl::rname;
  }
  static id_t id() {
    return type_impl::rid();
  }
};

template <typename T, typename TC>
struct helper<
    T,
    TC,
    typename std::enable_if<!(
        is_known<T, TC>::value && is_complete<T, TC>::value)>::type> {
  static_assert(
      is_known<T, TC>::value,
      "legacy_reflection: missing reflection metadata");
  static_assert(
      !is_known<T, TC>::value || is_complete<T, TC>::value,
      "legacy_reflection: incomplete handler");

  static void register_into(schema_t&) {}
  static constexpr auto name() {
    return ""_fs;
  }
  static id_t id() {
    return {};
  }
};

} // namespace legacy_reflection_detail

// legacy_reflection

template <typename T>
void legacy_reflection<T>::register_into(legacy_reflection_schema_t& schema) {
  using TC = reflect_type_class<T>;
  legacy_reflection_detail::helper<T, TC>::register_into(schema);
}

template <typename T>
legacy_reflection_schema_t legacy_reflection<T>::schema() {
  legacy_reflection_schema_t schema;
  register_into(schema);
  return schema;
}

template <typename T>
constexpr auto legacy_reflection<T>::name() {
  using TC = reflect_type_class<T>;
  return legacy_reflection_detail::helper<T, TC>::name();
}

template <typename T>
legacy_reflection_id_t legacy_reflection<T>::id() {
  using TC = reflect_type_class<T>;
  return legacy_reflection_detail::helper<T, TC>::id();
}

} // namespace thrift
} // namespace apache
