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

#ifndef THRIFT_FATAL_DEBUG_POST_INL_H_
#define THRIFT_FATAL_DEBUG_POST_INL_H_ 1

#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <fatal/type/variant_traits.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp2/reflection/container_traits.h>

namespace apache {
namespace thrift {
namespace detail {

struct debug_equals_missing {
  template <typename Iter, typename Callback>
  void operator()(
      Callback&& callback,
      Iter iterLhs,
      folly::StringPiece path,
      folly::StringPiece message) {
    using value_type = std::remove_reference_t<decltype(*iterLhs)>;
    callback(&*iterLhs, static_cast<value_type*>(nullptr), path, message);
  }

  template <typename Callback>
  void operator()(
      Callback&& callback,
      std::vector<bool>::const_iterator iterLhs,
      folly::StringPiece path,
      folly::StringPiece message) {
    bool const value = *iterLhs;
    callback(&value, static_cast<bool*>(nullptr), path, message);
  }
};

struct debug_equals_extra {
  template <typename Iter, typename Callback>
  void operator()(
      Callback&& callback,
      Iter iterRhs,
      folly::StringPiece path,
      folly::StringPiece message) {
    using value_type = std::remove_reference_t<decltype(*iterRhs)>;
    callback(static_cast<value_type*>(nullptr), &*iterRhs, path, message);
  }

  template <typename Callback>
  void operator()(
      Callback&& callback,
      std::vector<bool>::const_iterator iterRhs,
      folly::StringPiece path,
      folly::StringPiece message) {
    bool const value = *iterRhs;
    callback(static_cast<bool const*>(nullptr), &value, path, message);
  }
};

template <typename T>
T const* debug_equals_get_pointer(T const& what) {
  return &what;
}

template <typename T>
T const* debug_equals_get_pointer(std::shared_ptr<T> const& what) {
  return what.get();
}

template <typename T>
T const* debug_equals_get_pointer(std::unique_ptr<T> const& what) {
  return what.get();
}

template <typename>
struct debug_equals_impl;

template <typename T, typename Callback>
bool debug_equals(
    std::string& path,
    T const& lhs,
    T const& rhs,
    Callback&& callback) {
  using impl =
      detail::debug_equals_impl<reflect_type_class<std::remove_reference_t<T>>>;

  static_assert(
      fatal::is_complete<impl>::value, "debug_equals: unsupported type");

  return impl::equals(path, lhs, rhs, callback);
}

struct scoped_path {
 public:
  ~scoped_path() {
    assert(path_.size() >= size_);
    path_.resize(size_);
  }

  static scoped_path member(std::string& path, folly::StringPiece member) {
    return scoped_path(path, '.', member);
  }

  static scoped_path index(std::string& path, int64_t index) {
    return scoped_path(path, '[', index, ']');
  }

  static scoped_path key(std::string& path, std::string const& key) {
    return scoped_path(path, '[', key, ']');
  }

 private:
  template <typename... Args>
  explicit scoped_path(std::string& path, Args&&... args)
      : size_(path.size()), path_(path) {
    folly::toAppend(std::forward<Args>(args)..., &path);
  }

  std::size_t const size_;
  std::string& path_;
};

template <>
struct debug_equals_impl<type_class::enumeration> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    if (lhs != rhs) {
      auto l = fatal::enum_to_string(lhs, "<unknown>");
      auto r = fatal::enum_to_string(rhs, "<unknown>");
      callback(&l, &r, path, "value mismatch");
      return false;
    }
    return true;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::list<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs_, T const& rhs_, Callback&& callback) {
    using impl = debug_equals_impl<ValueTypeClass>;
    thrift_list_traits_adapter<T const> lhs{lhs_};
    thrift_list_traits_adapter<T const> rhs{rhs_};
    auto const minSize = std::min(lhs.size(), rhs.size());
    bool result = true;

    for (std::size_t index = 0; index < minSize; ++index) {
      auto const l = lhs.begin() + index;
      auto const r = rhs.begin() + index;
      auto guard = scoped_path::index(path, index);
      result = impl::equals(path, *l, *r, callback) && result;
    }

    for (std::size_t index = minSize; index < lhs.size(); ++index) {
      auto const lIter = lhs.begin() + index;
      auto guard = scoped_path::index(path, index);
      debug_equals_missing()(callback, lIter, path, "missing list entry");
      result = false;
    }

    for (std::size_t index = minSize; index < rhs.size(); ++index) {
      auto const rIter = rhs.begin() + index;
      auto guard = scoped_path::index(path, index);
      debug_equals_extra()(callback, rIter, path, "extra list entry");
      result = false;
    }
    return result;
  }
};

template <typename Type, typename>
struct debug_equals_impl_pretty {
  static std::string go(Type const& v) {
    return pretty_string(v);
  }
};

template <typename Type>
struct debug_equals_impl_pretty<Type, type_class::binary> {
  static std::string go(Type const& v) {
    std::string out;
    out.reserve(4 + v.size() * 2);

    auto const to_hex_digit = [](std::uint8_t const c) {
      return "0123456789ABCDEF"[c & 0xf];
    };

    out.append("\"0x");
    for (auto c : v) {
      out.push_back(to_hex_digit(c >> 4));
      out.push_back(to_hex_digit(c));
    }
    out.push_back('"');

    return out;
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct debug_equals_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs_, T const& rhs_, Callback&& callback) {
    using traits = thrift_map_traits<T>;
    using key_type = typename traits::key_type;
    using mapped_impl = debug_equals_impl<MappedTypeClass>;
    using pretty_key = debug_equals_impl_pretty<key_type, KeyTypeClass>;

    thrift_map_traits_adapter<T const> lhs{lhs_};
    thrift_map_traits_adapter<T const> rhs{rhs_};
    bool result = true;

    for (auto l : lhs) {
      auto const& key = traits::key(l);
      auto guard = scoped_path::key(path, pretty_key::go(key));
      if (rhs.find(key) == rhs.end()) {
        debug_equals_missing()(
            callback, &traits::mapped(l), path, "missing map entry");
        result = false;
      }
    }

    for (auto r : rhs) {
      auto const& key = traits::key(r);
      auto guard = scoped_path::key(path, pretty_key::go(key));
      if (lhs.find(key) == lhs.end()) {
        debug_equals_extra()(
            callback, &traits::mapped(r), path, "extra map entry");
        result = false;
      }
    }

    for (auto l : lhs) {
      auto const& key = traits::key(l);
      auto const ri = rhs.find(key);
      if (ri != rhs.end()) {
        auto guard = scoped_path::key(path, pretty_key::go(key));
        auto const& r = *ri;
        auto const& lv = traits::mapped(l);
        auto const& rv = traits::mapped(r);
        result = mapped_impl::equals(path, lv, rv, callback) && result;
      }
    }
    return result;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::set<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs_, T const& rhs_, Callback&& callback) {
    using traits = thrift_set_traits<T>;
    using value_type = typename traits::value_type;
    using pretty_value = debug_equals_impl_pretty<value_type, ValueTypeClass>;

    thrift_set_traits_adapter<T const> lhs{lhs_};
    thrift_set_traits_adapter<T const> rhs{rhs_};
    bool result = true;

    for (auto const& l : lhs) {
      auto guard = scoped_path::key(path, pretty_value::go(l));
      if (rhs.find(l) == rhs.end()) {
        debug_equals_missing()(callback, &l, path, "missing set entry");
        result = false;
      }
    }

    for (auto const& r : rhs) {
      auto guard = scoped_path::key(path, pretty_value::go(r));
      if (lhs.find(r) == lhs.end()) {
        debug_equals_extra()(callback, &r, path, "extra set entry");
        result = false;
      }
    }
    return result;
  }
};

struct debug_equals_with_pointers {
 protected:
  template <typename TypeClass, typename T, typename U, typename Callback>
  static bool recurse_into(
      std::string& path,
      T const& lMember,
      T const& rMember,
      Callback&& callback,
      U const&,
      U const&) {
    return debug_equals_impl<TypeClass>::equals(
        path, lMember, rMember, callback);
  }

  template <typename TypeClass, typename T, typename U, typename Callback>
  static bool recurse_into_ptr(
      std::string& path,
      T const* lMember,
      T const* rMember,
      Callback&& callback,
      U const& lObject,
      U const& rObject) {
    if (!lMember && !rMember) {
      return true;
    }
    if (!rMember) {
      debug_equals_missing()(callback, lMember, path, "missing");
      return false;
    }
    if (!lMember) {
      debug_equals_extra()(callback, rMember, path, "extra");
      return false;
    }
    return recurse_into<TypeClass>(
        path, *lMember, *rMember, callback, lObject, rObject);
  }

  template <typename TypeClass, typename Callback, typename U, typename T>
  static bool recurse_into(
      std::string& path,
      std::shared_ptr<T> const& lMember,
      std::shared_ptr<T> const& rMember,
      Callback&& callback,
      U const& lObject,
      U const& rObject) {
    return recurse_into_ptr<TypeClass>(
        path, lMember.get(), rMember.get(), callback, lObject, rObject);
  }

  template <typename TypeClass, typename Callback, typename U, typename T>
  static bool recurse_into(
      std::string& path,
      std::unique_ptr<T> const& lMember,
      std::unique_ptr<T> const& rMember,
      Callback&& callback,
      U const& lObject,
      U const& rObject) {
    return recurse_into_ptr<TypeClass>(
        path, lMember.get(), rMember.get(), callback, lObject, rObject);
  }
};

template <>
struct debug_equals_impl<type_class::variant> : debug_equals_with_pointers {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    using traits = fatal::variant_traits<T>;
    using descriptors = typename traits::descriptors;

    if (traits::get_id(lhs) != traits::get_id(rhs)) {
      visit_changed<debug_equals_missing>(path, lhs, callback);
      visit_changed<debug_equals_extra>(path, rhs, callback);
      return false;
    }

    bool result = true;

    using key = fatal::get_type::id;
    fatal::scalar_search<descriptors, key>(lhs.getType(), [&](auto indexed) {
      using descriptor = decltype(fatal::tag_type(indexed));

      assert(descriptor::id::value == traits::get_id(lhs));
      assert(descriptor::id::value == traits::get_id(rhs));

      using name = typename descriptor::metadata::name;
      auto guard = scoped_path::member(path, fatal::z_data<name>());

      using type_class = typename descriptor::metadata::type_class;
      typename descriptor::getter getter;
      result = recurse_into<type_class>(
          path, getter(lhs), getter(rhs), callback, lhs, rhs);
    });

    return result;
  }

 private:
  template <typename Change, typename T, typename Callback>
  static void
  visit_changed_field(std::string& path, T const& field, Callback&& callback) {
    Change()(callback, &field, path, "union type changed");
  }

  template <typename Change, typename T, typename Callback>
  static void visit_changed_field(
      std::string& path,
      std::unique_ptr<T> const& field,
      Callback&& callback) {
    Change()(callback, field.get(), path, "union type changed");
  }

  template <typename Change, typename T, typename Callback>
  static void visit_changed_field(
      std::string& path,
      std::shared_ptr<T> const& field,
      Callback&& callback) {
    Change()(callback, field.get(), path, "union type changed");
  }

  template <typename Change, typename T, typename Callback>
  static void
  visit_changed(std::string& path, T const& variant, Callback&& callback) {
    using traits = fatal::variant_traits<T>;
    using descriptors = typename traits::descriptors;
    fatal::scalar_search<descriptors, fatal::get_type::id>(
        variant.getType(), [&](auto indexed) {
          using descriptor = decltype(fatal::tag_type(indexed));

          assert(descriptor::id::value == traits::get_id(variant));

          using name = typename descriptor::metadata::name;
          auto guard = scoped_path::member(path, fatal::z_data<name>());

          typename descriptor::getter getter;
          visit_changed_field<Change>(path, getter(variant), callback);
        });
  }
};

template <>
struct debug_equals_impl<type_class::structure> : debug_equals_with_pointers {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    bool result = true;

    fatal::foreach<typename reflect_struct<T>::members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using getter = typename member::getter;

      if (member::optional::value == optionality::optional &&
          !member::is_set(lhs) && !member::is_set(rhs)) {
        return;
      }

      auto guard =
          scoped_path::member(path, fatal::z_data<typename member::name>());

      if (member::optional::value == optionality::optional) {
        if (!member::is_set(rhs)) {
          auto const& lPtr = debug_equals_get_pointer(getter::ref(lhs));
          debug_equals_missing()(callback, lPtr, path, "missing");
          result = false;
          return;
        }

        if (!member::is_set(lhs)) {
          auto const& rPtr = debug_equals_get_pointer(getter::ref(rhs));
          debug_equals_extra()(callback, rPtr, path, "extra");
          result = false;
          return;
        }
      }

      result =
          recurse_into<typename member::type_class>(
              path, getter::ref(lhs), getter::ref(rhs), callback, lhs, rhs) &&
          result;
    });

    return result;
  }
};

template <>
struct debug_equals_impl<type_class::string> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    if (lhs != rhs) {
      callback(&lhs, &rhs, path, "string mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::binary> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    if (lhs != rhs) {
      callback(&lhs, &rhs, path, "binary mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::floating_point> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    if (lhs != rhs) {
      callback(&lhs, &rhs, path, "floating point value mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::integral> {
  template <typename T, typename Callback>
  static bool
  equals(std::string& path, T const& lhs, T const& rhs, Callback&& callback) {
    if (lhs != rhs) {
      callback(&lhs, &rhs, path, "integral value mismatch");
      return false;
    }
    return true;
  }
};

} // namespace detail
} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_DEBUG_POST_INL_H_
