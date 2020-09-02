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
#include <type_traits>

#include <folly/Indestructible.h>
#include <folly/Portability.h>
#include <folly/Range.h>
#include <folly/Traits.h>

#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache {
namespace thrift {
namespace detail {

namespace st {

struct translate_field_name_table {
  size_t size;
  folly::StringPiece const* names;
  int16_t const* ids;
  protocol::TType const* types;
};

void translate_field_name(
    folly::StringPiece fname,
    int16_t& fid,
    protocol::TType& ftype,
    translate_field_name_table const& table) noexcept;

namespace {

//  gen_check_get_json
//  gen_check_get_nimble
//
//  Metafunctions for getting the member types named, respectively,
//    * __fbthrift_cpp2_gen_json
//    * __fbthrift_cpp2_gen_nimble
struct gen_check_get_json {
  template <typename Type>
  using apply =
      decltype(struct_private_access::__fbthrift_cpp2_gen_json<Type>());
};
struct gen_check_get_nimble {
  template <typename Type>
  using apply =
      decltype(struct_private_access::__fbthrift_cpp2_gen_nimble<Type>());
};

//  gen_check_get
//
//  Metafunction for applying Get over Type and for handling the case where
//  Get fails to apply.
//
//  Get is one of the getters above:
//    * gen_check_get_json
//    * gen_check_get_nimble
//
//  When Get::apply<Type>:
//    * fails to apply (because cpp.type is in use), treat as true
//    * returns signed (extern template instances are generated), treat as true
//    * returns unsigned (otherwise), treat as false
//
//  The tag types signed and unsigned are used in the generated code to minimize
//  the overhead of parsing the class body, shifting all overhead to the code
//  which inspects these tags.
template <typename Void, typename Get, typename Type>
constexpr bool gen_check_get_ = true;
template <typename Get, typename Type>
constexpr bool gen_check_get_<
    folly::void_t<typename Get::template apply<Type>>,
    Get,
    Type> = Get::template apply<Type>::value;
template <typename Get, typename Type>
constexpr bool gen_check_get = gen_check_get_<void, Get, Type>;

//  gen_check_rec
//
//  Metafunction for recursing through container types to apply the metafunction
//  gen_check_get over struct/union types.
//
//  Get is one of the getters above:
//    * gen_check_get_json
//    * gen_check_get_nimble
template <typename TypeClass>
struct gen_check_rec {
  template <typename Get, typename Type>
  static constexpr bool apply = true;
};
template <typename ValueTypeClass>
struct gen_check_rec_list_set {
  using ValueTraits = gen_check_rec<ValueTypeClass>;
  template <typename Get, typename Type>
  static constexpr bool apply =
      ValueTraits::template apply<Get, typename Type::value_type>;
};
template <typename ValueTypeClass>
struct gen_check_rec<type_class::list<ValueTypeClass>>
    : gen_check_rec_list_set<ValueTypeClass> {};
template <typename ValueTypeClass>
struct gen_check_rec<type_class::set<ValueTypeClass>>
    : gen_check_rec_list_set<ValueTypeClass> {};
template <typename KeyTypeClass, typename MappedTypeClass>
struct gen_check_rec<type_class::map<KeyTypeClass, MappedTypeClass>> {
  using KeyTraits = gen_check_rec<KeyTypeClass>;
  using MappedTraits = gen_check_rec<MappedTypeClass>;
  template <typename Get, typename Type>
  static constexpr bool apply =
      KeyTraits::template apply<Get, typename Type::key_type>&&
          MappedTraits::template apply<Get, typename Type::mapped_type>;
};
struct gen_check_rec_structure_variant {
  template <typename Get, typename Type>
  static constexpr bool apply = gen_check_get<Get, Type>;
};
template <>
struct gen_check_rec<type_class::structure> : gen_check_rec_structure_variant {
};
template <>
struct gen_check_rec<type_class::variant> : gen_check_rec_structure_variant {};

//  gen_check
//
//  Returns whether, if the property Get holds for the outer structure Type,
//  that it also holds for each structure-typed field FieldType of the outer
//  type, peering through containers.
//
//  Get is one of the getters above:
//    * gen_check_get_json
//    * gen_check_get_nimble
template <
    typename Get,
    typename Type,
    typename FieldTypeClass,
    typename FieldType>
constexpr bool gen_check = !gen_check_get<Get, Type> ||
    gen_check_rec<FieldTypeClass>::template apply<Get, FieldType>;

//  gen_check_json
//  gen_check_nimble
//
//  Aliases to gen_check partially instantiated with one of the getters above:
//    * gen_check_get_json
//    * gen_check_get_nimble
//
//  Used by a generated static_assert to enforce consistency over transitive
//  dependencies in the use of extern-template instantiations over json/nimble.
template <typename Type, typename FieldTypeClass, typename FieldType>
constexpr bool gen_check_json =
    gen_check<gen_check_get_json, Type, FieldTypeClass, FieldType>;
template <typename Type, typename FieldTypeClass, typename FieldType>
constexpr bool gen_check_nimble =
    gen_check<gen_check_get_nimble, Type, FieldTypeClass, FieldType>;

} // namespace

} // namespace st

} // namespace detail
} // namespace thrift
} // namespace apache
