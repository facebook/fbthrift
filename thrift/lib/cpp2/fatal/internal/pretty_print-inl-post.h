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
#ifndef THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_
#define THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/type.h>
#include <fatal/type/variant_traits.h>

namespace apache { namespace thrift { namespace detail {

/**
 * Pretty print specialization for enumerations.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::enumeration> {
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
template <typename ValueTypeClass>
struct pretty_print_impl<type_class::list<ValueTypeClass>> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<list>[";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.start_scope();
        scope << index << ": ";
        pretty_print_impl<ValueTypeClass>::print(scope, i);
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
template <typename KeyTypeClass, typename MappedTypeClass>
struct pretty_print_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<map>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.start_scope();
        pretty_print_impl<KeyTypeClass>::print(scope, i.first);
        scope << ": ";
        pretty_print_impl<MappedTypeClass>::print(scope, i.second);
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
 * Pretty print specialization for sets.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename ValueTypeClass>
struct pretty_print_impl<type_class::set<ValueTypeClass>> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<set>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const &i: what) {
        auto scope = out.start_scope();
        pretty_print_impl<ValueTypeClass>::print(scope, i);
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
    typename Descriptor, std::size_t Index,
    typename OutputStream, typename T
  >
  void operator ()(
    fatal::indexed<Descriptor, Index>,
    OutputStream &&out,
    T const &what
  ) const {
    out.newline();
    out << fatal::enum_to_string(what.getType()) << ": ";
    pretty_print_impl<typename Descriptor::metadata::type_class>::print(
      out, typename Descriptor::getter()(what)
    );
    out.newline();
  }
};

/**
 * Pretty print specialization for variants (Thrift unions).
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::variant> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    using namespace fatal;
    out << "<variant>{";
    sorted_search<
      sort<typename variant_traits<T>::descriptors, less, get_type::id>,
      get_type::id::apply
    >(
      what.getType(),
      pretty_print_variant_visitor(),
      out.start_scope(),
      what
    );
    out << '}';
  }
};

/*
 * Pretty print specialization for structures.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::structure> {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << "<struct>{";
    out.newline();
    using info = reflect_struct<T>;
    constexpr auto size = fatal::size<typename info::members>::value;
    fatal::foreach<typename info::members>([&](auto indexed) {
      using member = fatal::type_of<decltype(indexed)>;
      constexpr auto index = decltype(indexed)::value;
      auto scope = out.start_scope();
      scope << fatal::z_data<typename member::name>() << ": ";
      pretty_print_impl<typename member::type_class>::print(
        scope, member::getter::ref(what)
      );
      if (index + 1 < size) {
        scope << ',';
      }
      scope.newline();
    });
    out << '}';
  }
};

/**
 * Pretty print specialization for strings.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::string> {
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
template <typename>
struct pretty_print_impl {
  template <typename OutputStream, typename T>
  static void print(OutputStream &out, T const &what) {
    out << what;
  }

  template <typename OutputStream>
  static void print(OutputStream &out, bool const what) {
    out << (what ? "true" : "false");
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_
