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
#ifndef THRIFT_FATAL_REFLECTION_INL_POST_H_
#define THRIFT_FATAL_REFLECTION_INL_POST_H_ 1

#if !defined THRIFT_FATAL_REFLECTION_H_
# error "This file must be included from reflection.h"
#endif

namespace apache { namespace thrift {
namespace detail { namespace reflection_impl {

template <typename Owner, typename Getter, bool has_isset>
struct is_set {
  constexpr static bool check(Owner const &owner) {
    return Getter::ref(owner.__isset);
  }
};

template <typename Owner, typename Getter>
struct is_set<
  Owner,
  Getter,
  false
> {
  constexpr static bool check(Owner const &) { return true; }
};

template <typename Owner, typename Getter, bool has_isset>
struct mark_set {
  constexpr static void mark(Owner& owner) {
    Getter::ref(owner.__isset) = true;
  }
};

template <typename Owner, typename Getter>
struct mark_set<
  Owner,
  Getter,
  false
> {
  constexpr static void mark(Owner& owner) {
    // nop
    return;
  }
};

template <typename, typename, bool has_isset>
struct unmark_set;

template <typename Owner, typename Getter>
struct unmark_set<
  Owner,
  Getter,
  true
> {
  constexpr static void mark(Owner& owner) {
    Getter::ref(owner.__isset) = false;
  }
};

} // reflection_impl

template <typename Module, typename Annotations, legacy_type_id_t LegacyTypeId>
struct type_common_metadata_impl {
  using module = Module;
  using annotations = Annotations;
  using legacy_id = std::integral_constant<legacy_type_id_t, LegacyTypeId>;
};

template <
  typename T,
  bool = fatal::is_complete<thrift_list_traits<T>>::value,
  bool = fatal::is_complete<thrift_map_traits<T>>::value,
  bool = fatal::is_complete<thrift_set_traits<T>>::value
>
struct reflect_container_type_class_impl {
  using type = type_class::unknown;
};

template <typename T>
struct reflect_container_type_class_impl<T, true, false, false> {
  using type = type_class::list<
    reflect_type_class<typename thrift_list_traits<T>::value_type>
  >;
};

template <typename T>
struct reflect_container_type_class_impl<T, false, true, false> {
  using type = type_class::map<
    reflect_type_class<typename thrift_map_traits<T>::key_type>,
    reflect_type_class<typename thrift_map_traits<T>::mapped_type>
  >;
};

template <typename T>
struct reflect_container_type_class_impl<T, false, false, true> {
  using type = type_class::set<
    reflect_type_class<typename thrift_set_traits<T>::value_type>
  >;
};

template <typename T>
struct reflect_type_class_impl {
  using type = typename std::conditional<
    is_reflectable_enum<T>::value,
    type_class::enumeration,
    typename std::conditional<
      is_reflectable_union<T>::value,
      type_class::variant,
      typename std::conditional<
        is_reflectable_struct<T>::value,
        type_class::structure,
        typename std::conditional<
          std::is_floating_point<T>::value,
          type_class::floating_point,
          typename std::conditional<
            std::is_integral<T>::value,
            type_class::integral,
            typename std::conditional<
              std::is_same<void, T>::value,
              type_class::nothing,
              typename std::conditional<
                fatal::is_complete<thrift_string_traits<T>>::value,
                type_class::string,
                typename reflect_container_type_class_impl<T>::type
              >::type
            >::type
          >::type
        >::type
      >::type
    >::type
  >::type;
};

template <typename, typename>
struct reflect_module_tag_selector {
  using type = void;
};

template <typename T>
struct reflect_module_tag_impl {
  template <typename = void>
  struct get {
    using type = typename reflect_module_tag_selector<
      reflect_type_class<T>,
      T
    >::type;

    static_assert(
      !std::is_same<void, type>::value,
      "given type has no reflection metadata or is not a struct, enum or union"
    );
  };

  template <typename Default>
  class try_get {
    using impl = typename reflect_module_tag_selector<
      reflect_type_class<T>,
      T
    >::type;

  public:
    using type = typename std::conditional<
      std::is_same<void, impl>::value, Default, impl
    >::type;
  };
};

template <typename T>
struct reflect_module_tag_selector<type_class::enumeration, T> {
  using type = typename fatal::enum_traits<T>::metadata::module;
};

template <typename T>
struct reflect_module_tag_selector<type_class::variant, T> {
  using type = typename fatal::variant_traits<T>::metadata::module;
};

template <typename T>
struct reflect_module_tag_selector<type_class::structure, T> {
  using type = typename reflect_struct<T>::module;
};

} // detail

template <>
struct reflected_annotations<void> {
  struct keys {};
  struct values {};
  using map = fatal::type_map<>;
};

}} // apache::thrift

#endif // THRIFT_FATAL_REFLECTION_INL_POST_H_
