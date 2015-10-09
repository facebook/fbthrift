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
#ifndef THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_
#define THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_ 1

#include <fatal/type/enum.h>
#include <fatal/type/variant_traits.h>

namespace apache { namespace thrift { namespace detail {

template <> struct dynamic_converter_impl<thrift_category::enumeration> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    out = fatal::enum_to_string(what);
  }
};

template <> struct dynamic_converter_impl<thrift_category::list> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    out = std::initializer_list<folly::dynamic>{};
    for (auto const &i: what) {
      out.push_back(to_dynamic(i));
    }
  }
};

template <> struct dynamic_converter_impl<thrift_category::map> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    out = folly::dynamic::object;
    for (auto const &i: what) {
      to_dynamic(out[to_dynamic(i.first)], i.second);
    }
  }
};

template <> struct dynamic_converter_impl<thrift_category::set> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    dynamic_converter_impl<thrift_category::list>::to(out, what);
  }
};

// visitor for a type map
struct to_dynamic_variant_visitor {
  template <
    typename Id, typename Descriptor, std::size_t Index,
    typename Needle, typename T
  >
  void operator ()(
    fatal::indexed_type_pair_tag<Id, Descriptor, Index>,
    Needle,
    folly::dynamic &out,
    T const &what
  ) const {
    to_dynamic(out["value"], typename Descriptor::getter()(what));
  }
};

template <> struct dynamic_converter_impl<thrift_category::variant> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    using traits = fatal::variant_traits<T>;

    out = folly::dynamic::object;

    bool const found = traits::by_id::map::template binary_search<>::exact(
      what.getType(),
      to_dynamic_variant_visitor(),
      out,
      what
    );

    if (found) {
      out["type"] = fatal::enum_to_string(what.getType());
    }
  }
};

// visitor for a type list
struct to_dynamic_struct_visitor {
  template <typename MemberInfo, std::size_t Index, typename T>
  void operator ()(
    fatal::indexed_type_tag<MemberInfo, Index>,
    folly::dynamic &out,
    T const &what
  ) const {
    dynamic_converter_impl<MemberInfo::category::value>::to(
      out[folly::StringPiece(MemberInfo::name::data(), MemberInfo::name::size)],
      MemberInfo::getter::ref(what)
    );
  }
};

template <> struct dynamic_converter_impl<thrift_category::structure> {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    using info = reflect_struct<T>;
    out = folly::dynamic::object;
    info::members::mapped::foreach(to_dynamic_struct_visitor(), out, what);
  }
};

// fallback
template <thrift_category Category>
struct dynamic_converter_impl {
  template <typename T>
  static void to(folly::dynamic &out, T const &what) {
    out = what;
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_
