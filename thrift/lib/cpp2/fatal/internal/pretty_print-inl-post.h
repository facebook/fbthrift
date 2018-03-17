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
#ifndef THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_
#define THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/variant_traits.h>

namespace apache {
namespace thrift {
namespace detail {

/**
 * Pretty print specialization for enumerations.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::enumeration> {
  template <typename OutputStream, typename T>
  static void print(OutputStream& out, T const& what) {
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
  static void print(OutputStream& out, T const& what) {
    out << "<list>[";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const& i : what) {
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
  static void print(OutputStream& out, T const& what) {
    out << "<map>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const& i : what) {
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
  static void print(OutputStream& out, T const& what) {
    out << "<set>{";
    if (!what.empty()) {
      out.newline();
      auto const size = what.size();
      std::size_t index = 0;
      for (auto const& i : what) {
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
 * Pretty print specialization for variants (Thrift unions).
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <>
struct pretty_print_impl<type_class::variant> {
  template <typename OutputStream, typename T>
  static void print(OutputStream& out, T const& what) {
    using namespace fatal;
    using descriptors = typename variant_traits<T>::descriptors;
    out << "<variant>{";
    scalar_search<descriptors, get_type::id>(what.getType(), [&](auto indexed) {
      using descriptor = decltype(fatal::tag_type(indexed));
      auto scope = out.start_scope();
      scope.newline();
      scope << fatal::enum_to_string(what.getType()) << ": ";
      pretty_print_impl<typename descriptor::metadata::type_class>::print(
          scope, typename descriptor::getter()(what));
      scope.newline();
    });
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
  static void print(OutputStream& out, T const& what) {
    out << "<struct>{";
    out.newline();
    using info = reflect_struct<T>;
    fatal::foreach<typename info::members>([&](auto indexed) {
      constexpr auto size = fatal::size<typename info::members>::value;
      using member = decltype(fatal::tag_type(indexed));
      auto const index = fatal::tag_index(indexed);
      auto scope = out.start_scope();
      scope << fatal::z_data<typename member::name>() << ": ";
      pretty_print_impl<typename member::type_class>::print(
          scope, member::getter::ref(what));
      if (index + 1 < size) {
        scope << ',';
      }
      scope.newline();
    });
    out << '}';
  }

  template <typename OutputStream, typename T>
  static void print(OutputStream& out, std::unique_ptr<T> const& what) {
    if (!what) {
      out << "null";
      return;
    }
    print(out, *what);
  }

  template <typename OutputStream, typename T>
  static void print(OutputStream& out, std::shared_ptr<T> const& what) {
    if (!what) {
      out << "null";
      return;
    }
    print(out, *what);
  }

  template <typename OutputStream, typename T>
  static void print(OutputStream& out, std::shared_ptr<T const> const& what) {
    if (!what) {
      out << "null";
      return;
    }
    print(out, *what);
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
  static void print(OutputStream& out, T const& what) {
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
  static void print(OutputStream& out, T const& what) {
    out << what;
  }

  template <typename OutputStream>
  static void print(OutputStream& out, bool const what) {
    out << (what ? "true" : "false");
  }
};

} // namespace detail
} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_PRETTY_PRINT_INL_POST_H_
