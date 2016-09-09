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
#ifndef THRIFT_FATAL_DEBUG_POST_INL_H_
#define THRIFT_FATAL_DEBUG_POST_INL_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <folly/Conv.h>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <fatal/type/variant_traits.h>

#include <stdexcept>
#include <type_traits>
#include <utility>

#include <cassert>

namespace apache { namespace thrift { namespace detail {

template <typename> struct debug_equals_impl;

template <typename T, typename Callback>
bool debug_equals(
  std::string &path,
  T const &lhs,
  T const &rhs,
  Callback &&callback
) {
  using impl = detail::debug_equals_impl<
    reflect_type_class<typename std::decay<T>::type>
  >;

  static_assert(
    fatal::is_complete<impl>::value,
    "debug_equals: unsupported type"
  );

  return impl::equals(path, lhs, rhs, callback);
}

struct scoped_path {
  template <typename... Args>
  explicit scoped_path(std::string &path, Args &&...args):
    size_(path.size()),
    path_(path)
  {
    if (!path.empty()) {
      path.push_back('.');
    }

    folly::toAppend(std::forward<Args>(args)..., &path);
  }

  ~scoped_path() {
    assert(path_.size() >=  size_);
    path_.resize(size_);
  }

private:
  std::size_t const size_;
  std::string &path_;
};

template <>
struct debug_equals_impl<type_class::enumeration> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(
        fatal::enum_to_string(lhs, "<unknown>"),
        fatal::enum_to_string(rhs, "<unknown>"),
        path,
        "value mismatch"
      );
      return false;
    }
    return true;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::list<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_list_traits<T>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
      return false;
    }

    auto l = traits::begin(lhs);
    auto const le = traits::end(lhs);
    auto r = traits::begin(rhs);
    auto const re = traits::end(rhs);
    for (std::size_t index = 0; l != le && r != re; ++l, ++r, ++index) {
      scoped_path guard(path, index);

      if (!debug_equals_impl<ValueTypeClass>::equals(path, *l, *r, callback)) {
        return false;
      }
    }
    assert(l == le);
    assert(r == re);
    return true;
  }
};

template <typename Type, typename>
struct debug_equals_impl_pretty {
  static std::string go(Type const &v) {
    return pretty_string(v);
  }
};
template <typename Type>
struct debug_equals_impl_pretty<Type, type_class::string> {
  static std::string go(Type const &v) {
    return folly::to<std::string>(v);
  }
};
template <>
struct debug_equals_impl_pretty<std::string, type_class::string> {
  static std::string const &go(std::string const &v) {
    return v;
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct debug_equals_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_map_traits<T>;
    using key_type = typename traits::key_type;
    using mapped_impl = debug_equals_impl<MappedTypeClass>;
    using pretty_key = debug_equals_impl_pretty<key_type, KeyTypeClass>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
    }
    auto const le = traits::end(lhs);
    auto const re = traits::end(rhs);

    bool equals = true;

    for (auto l = traits::begin(lhs); l != le; ++l) {
      auto const &key = traits::key(l);
      scoped_path guard(path, pretty_key::go(key));
      if (traits::find(rhs, key) == re) {
        equals = false;
        callback(lhs, rhs, path, "missing");
      }
    }

    for (auto r = traits::begin(rhs); r != re; ++r) {
      auto const &key = traits::key(r);
      scoped_path guard(path, pretty_key::go(key));
      if (traits::find(lhs, key) == le) {
        equals = false;
        callback(lhs, rhs, path, "extra");
      }
    }

    for (auto l = traits::begin(lhs); l != le; ++l) {
      auto const &key = traits::key(l);
      auto r = traits::find(rhs, key);
      if (r != re) {
        scoped_path guard(path, pretty_key::go(key));
        auto const& lv = traits::mapped(l);
        auto const& rv = traits::mapped(r);
        equals &= mapped_impl::equals(path, lv, rv, callback);
      }
    }

    return equals;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::set<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_set_traits<T>;
    using value_type = typename traits::value_type;
    using pretty_value = debug_equals_impl_pretty<value_type, ValueTypeClass>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
    }

    auto const le = traits::end(lhs);
    auto const re = traits::end(rhs);

    bool equals = true;

    for (auto l = traits::begin(lhs); l != le; ++l) {
      scoped_path guard(path, pretty_value::go(*l));
      if (traits::find(rhs, *l) == re) {
        equals = false;
        callback(lhs, rhs, path, "missing");
      }
    }

    for (auto r = traits::begin(rhs); r != re; ++r) {
      scoped_path guard(path, pretty_value::go(*r));
      if (traits::find(lhs, *r) == le) {
        equals = false;
        callback(lhs, rhs, path, "extra");
      }
    }

    return equals;
  }
};

template <typename VariantTraits>
struct debug_equals_variant_visitor {
  template <
    typename Descriptor, std::size_t Index,
    typename T, typename Callback
  >
  void operator ()(
    fatal::indexed<Descriptor, Index>,
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback,
    bool &result
  ) const {
    assert(Descriptor::id::value == VariantTraits::get_id(lhs));
    assert(Descriptor::id::value == VariantTraits::get_id(rhs));

    using name = typename Descriptor::metadata::name;
    scoped_path guard(path, fatal::z_data<name>());

    using type_class = typename Descriptor::metadata::type_class;
    typename Descriptor::getter getter;
    result = debug_equals_impl<type_class>::equals(
      path, getter(lhs), getter(rhs), callback
    );
  }
};

template <>
struct debug_equals_impl<type_class::variant> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using namespace fatal;
    using traits = variant_traits<T>;

    if (traits::get_id(lhs) != traits::get_id(rhs)) {
      callback(
        enum_to_string(traits::get_id(lhs), "<unknown>"),
        enum_to_string(traits::get_id(rhs), "<unknown>"),
        path,
        "variant stored type mismatch"
      );
      return false;
    }

    bool result = true;
    sorted_search<
      sort<typename traits::descriptors, less, get_type::id>,
      get_type::id::apply
    >(
      lhs.getType(),
      debug_equals_variant_visitor<traits>(),
      path,
      lhs,
      rhs,
      callback,
      result
    );

    return result;
  }
};

struct debug_equals_struct_visitor {
  template <typename Member, std::size_t Index, typename T, typename Callback>
  void operator ()(
    fatal::indexed<Member, Index>,
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback,
    bool &result
  ) const {
    if (!result) {
      return;
    }

    scoped_path guard(path, fatal::z_data<typename Member::name>());

    using getter = typename Member::getter;
    result = debug_equals_impl<typename Member::type_class>::equals(
      path, getter::ref(lhs), getter::ref(rhs), callback
    );
  }
};

template <>
struct debug_equals_impl<type_class::structure> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    bool result = true;

    fatal::foreach<typename reflect_struct<T>::members>(
      debug_equals_struct_visitor(),
      path, lhs, rhs, callback, result
    );

    return result;
  }
};

template <>
struct debug_equals_impl<type_class::string> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "string mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::floating_point> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "floating point value mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::integral> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "integral value mismatch");
      return false;
    }
    return true;
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_DEBUG_POST_INL_H_
