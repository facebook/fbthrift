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

#pragma once

namespace apache { namespace thrift { namespace merge_into_detail {

template <typename T>
struct merge {
  using impl = merge_impl<reflect_type_class<T>>;
  static constexpr auto is_complete = fatal::is_complete<impl>::value;
  static_assert(is_complete, "merge_into: incomplete type");
  static constexpr auto is_known = !std::is_same<
      reflect_type_class<T>, type_class::unknown>::value;
  static_assert(is_known, "merge_into: missing reflection metadata");
  static void go(const T& src, T& dst) { impl::template go<T>(src, dst); }
  static void go(T&& src, T& dst) { impl::template go<T>(std::move(src), dst); }
};

template <typename TypeClass>
struct merge_impl {
  template <typename T>
  static void go(const T& src, T& dst) {
    dst = src;
  }
  template <typename T>
  static void go(T&& src, T& dst) {
    dst = std::move(src);
  }
};

template <>
struct merge_impl<type_class::structure> {
  template <typename T>
  static void impl(T&& src, remove_const_reference<T>& dst) {
    using type = remove_const_reference<T>;
    using members = typename reflect_struct<type>::members;
    fatal::foreach<members>([&](auto indexed) {
      using member = fatal::type_of<decltype(indexed)>;
      using mtype = typename member::type;
      using mgetter = typename member::getter;
      using mref = fatal::add_cv_reference_from_t<mtype, T&&>;
      if (member::optional::value == optionality::optional &&
          member::is_set(src)) {
        return;
      }
      member::mark_set(dst);
      merge<mtype>::go(static_cast<mref>(mgetter::ref(src)), mgetter::ref(dst));
    });
  }
  template <typename T>
  static void go(const T& src, T& dst) {
    impl(src, dst);
  }
  template <typename T>
  static void go(T&& src, T& dst) {
    impl(std::move(src), dst);
  }
};

template <typename ValueTypeClass>
struct merge_impl<type_class::list<ValueTypeClass>> {
  template <typename T>
  struct wrapper {
    T& rep;
    using traits = thrift_list_traits<T>;
    using value_type = typename traits::value_type;
    void push_back(const value_type& v) { traits::push_back(rep, v); }
    void push_back(value_type&& v) { traits::push_back(rep, std::move(v)); }
  };
  template <typename T>
  static void go(const T& src, T& dst) {
    using traits = thrift_list_traits<T>;
    traits::reserve(dst, traits::size(dst) + traits::size(src));
    wrapper<T> dstw { dst };
    std::copy(traits::cbegin(src), traits::cend(src), std::back_inserter(dstw));
  }
  template <typename T>
  static void go(T&& src, T& dst) {
    using traits = thrift_list_traits<T>;
    traits::reserve(dst, traits::size(dst) + traits::size(src));
    wrapper<T> dstw { dst };
    std::move(traits::begin(src), traits::end(src), std::back_inserter(dstw));
  }
};

template <typename ValueTypeClass>
struct merge_impl<type_class::set<ValueTypeClass>> {
  template <typename T>
  struct wrapper {
    T& rep;
    using traits = thrift_set_traits<T>;
    using value_type = typename traits::value_type;
    using iterator = typename traits::iterator;
    using const_iterator = typename traits::const_iterator;
    iterator insert(const_iterator position, const value_type& val) {
      return traits::insert(rep, position, val);
    }
    iterator insert(const_iterator position, value_type&& val) {
      return traits::insert(rep, position, std::move(val));
    }
  };
  template <typename T>
  static void go(const T& src, T& dst) {
    using traits = thrift_set_traits<T>;
    wrapper<T> dstw { dst };
    std::copy(
        traits::cbegin(src),
        traits::cend(src),
        std::inserter(dstw, traits::end(dst)));
  }
  template <typename T>
  static void go(T&& src, T& dst) {
    using traits = thrift_set_traits<T>;
    wrapper<T> dstw { dst };
    std::move(
        traits::begin(src),
        traits::end(src),
        std::inserter(dstw, traits::end(dst)));
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct merge_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T>
  static void go(const T& src, T& dst) {
    using traits = thrift_map_traits<T>;
    using M = typename traits::mapped_type;
    auto r = folly::range(traits::cbegin(src), traits::cend(src));
    for (const auto& kv : r) {
      merge<M>::go(kv.second, traits::get_or_create(dst, kv.first));
    }
  }
  template <typename T>
  static void go(T&& src, T& dst) {
    using traits = thrift_map_traits<T>;
    using M = typename traits::mapped_type;
    auto r = folly::range(traits::begin(src), traits::end(src));
    for (auto& kv : r) {
      merge<M>::go(std::move(kv.second), traits::get_or_create(dst, kv.first));
    }
  }
};

}}} // apache::thrift::merge_into_detail

namespace apache { namespace thrift {

template <typename T>
void merge_into(T&& src, merge_into_detail::remove_const_reference<T>& dst) {
  using D = typename merge_into_detail::remove_const_reference<T>;
  using W = fatal::add_cv_reference_from_t<D, T&&>;
  merge_into_detail::merge<D>::go(static_cast<W>(src), dst);
}

}} // apache::thrift
