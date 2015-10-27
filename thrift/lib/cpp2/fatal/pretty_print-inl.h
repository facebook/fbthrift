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
#ifndef THRIFT_FATAL_PRETTY_PRINT_INL_H_
#define THRIFT_FATAL_PRETTY_PRINT_INL_H_ 1

#include <fatal/type/enum.h>
#include <fatal/type/variant_traits.h>

namespace apache { namespace thrift { namespace detail {

template <thrift_category> struct pretty_print_impl;

/**
 * Pretty print entry point that reuses existing stream.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename OutputStream, typename T>
OutputStream &pretty_print(OutputStream &&out, T &&what) {
  using impl = detail::pretty_print_impl<
    reflect_category<typename std::decay<T>::type>::value
  >;

  impl::print(out, std::forward<T>(what));

  return out;
}

/**
 * Pretty print specialization for enumerations.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::enumeration> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << fatal::enum_to_string(what);
  }
};

/**
 * Pretty print specialization for lists.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::list> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<list>[";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.push();
        scope << index << ": ";
        detail::pretty_print(scope, i);
        if (++index < size) {
          scope << ',';
        }
        scope.newline();
      }
    }
    out << ']';
  }
};

/**
 * Pretty print specialization for maps.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::map> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<map>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.push();
        detail::pretty_print(scope, i.first);
        scope << ": ";
        detail::pretty_print(scope, i.second);
        if (++index < size) {
          scope << ',';
        }
        scope.newline();
      }
    }
    out << ']';
  }
};

/**
 * Pretty print specialization for sets.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::set> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<set>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.push();
        detail::pretty_print(scope, i);
        if (++index < size) {
          scope << ',';
        }
        scope.newline();
      }
    }
    out << '}';
  }
};

/**
 * Pretty print visitor for variant (Thrift union) members.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
struct pretty_print_variant_visitor {
  template <
    typename Id, typename Descriptor, std::size_t Index,
    typename OutputStream, typename Needle, typename T
  >
  void operator ()(
    fatal::indexed_type_pair_tag<Id, Descriptor, Index>,
    Needle,
    OutputStream &&out,
    T const &what
  ) const {
    out.newline();
    out << fatal::enum_to_string(what.getType()) << ": ";
    detail::pretty_print(out, typename Descriptor::getter()(what));
    out.newline();
  }
};

/**
 * Pretty print specialization for variants (Thrift unions).
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::variant> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<variant>{";
    fatal::variant_traits<T>::by_id::map::template binary_search<>::exact(
      what.getType(),
      pretty_print_variant_visitor(),
      out.push(),
      what
    );
    out << '}';
  }
};

/**
 * Pretty print visitor for structure members.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <std::size_t Size>
struct pretty_print_struct_visitor {
  template <
    typename MemberInfo, std::size_t Index,
    typename OutputStream, typename T
  >
  void operator ()(
    fatal::indexed_type_tag<MemberInfo, Index>,
    OutputStream &&out,
    T const &what
  ) const {
    auto scope = out.push();
    scope << MemberInfo::name::z_data() << ": ";
    detail::pretty_print(scope, MemberInfo::getter::ref(what));
    if (Index + 1 < Size) {
      scope << ',';
    }
    scope.newline();
  }
};

/**
 * Pretty print specialization for structures.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::structure> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<struct>{";
    out.newline();
    using info = reflect_struct<T>;
    info::members::mapped::foreach(
      pretty_print_struct_visitor<info::members::size>(),
      out,
      what
    );
    out << '}';
  }
};

/**
 * Pretty print specialization for strings.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <> struct pretty_print_impl<thrift_category::string> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << '"' << what << '"';
  }
};

/**
 * Pretty print fallback specialization.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <thrift_category Category>
struct pretty_print_impl {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << what;
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_PRETTY_PRINT_INL_H_
