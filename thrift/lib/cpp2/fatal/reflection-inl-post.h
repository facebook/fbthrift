/*
 * Copyright 2015 Facebook, Inc.
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

namespace apache { namespace thrift { namespace detail {

template <typename Module, typename Annotations, legacy_type_id_t LegacyTypeId>
struct type_common_metadata_impl {
  using module = Module;
  using annotations = Annotations;
  using legacy_id = std::integral_constant<legacy_type_id_t, LegacyTypeId>;
};

template <thrift_category Category>
using as_thrift_category = std::integral_constant<thrift_category, Category>;

template <typename T>
struct reflect_category_impl {
  using type = typename std::conditional<
    is_reflectable_enum<T>::value,
    as_thrift_category<thrift_category::enumeration>,
    typename std::conditional<
      is_reflectable_union<T>::value,
      as_thrift_category<thrift_category::variant>,
      typename std::conditional<
        is_reflectable_struct<T>::value,
        as_thrift_category<thrift_category::structure>,
        typename std::conditional<
          std::is_floating_point<T>::value,
          as_thrift_category<thrift_category::floating_point>,
          typename std::conditional<
            std::is_integral<T>::value,
            as_thrift_category<thrift_category::integral>,
            typename std::conditional<
              std::is_same<void, T>::value,
              as_thrift_category<thrift_category::nothing>,
              typename get_thrift_category<T>::type
            >::type
          >::type
        >::type
      >::type
    >::type
  >::type;
};

template <thrift_category, typename>
struct reflect_module_tag_selector {
  using type = void;
};

template <typename T>
struct reflect_module_tag_impl {
  template <typename = void>
  struct get {
    using type = typename reflect_module_tag_selector<
      reflect_category<T>::value,
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
      reflect_category<T>::value,
      T
    >::type;

  public:
    using type = typename std::conditional<
      std::is_same<void, impl>::value, Default, impl
    >::type;
  };
};

template <typename T>
struct reflect_module_tag_selector<thrift_category::enumeration, T> {
  using type = typename fatal::enum_traits<T>::metadata::module;
};

template <typename T>
struct reflect_module_tag_selector<thrift_category::variant, T> {
  using type = typename fatal::variant_traits<T>::metadata::module;
};

template <typename T>
struct reflect_module_tag_selector<thrift_category::structure, T> {
  using type = typename reflect_struct<T>::module;
};

////////////////////////////
// IMPLEMENTATION DETAILS //
////////////////////////////

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_REFLECTION_INL_POST_H_
