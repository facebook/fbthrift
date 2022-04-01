/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <utility>

#include <thrift/lib/cpp2/op/detail/BasePatch.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <typename C1, typename C2>
void erase_all(C1& container, const C2& values) {
  for (auto itr = values.begin(); itr != values.end() && !container.empty();
       ++itr) {
    container.erase(*itr);
  }
}

// Patch must have the following fields:
//   optional list<T> assign;
//   bool clear;
//   list<T> append;
//   list<T> prepend;
template <typename Patch>
class ListPatch : public BaseClearablePatch<Patch, ListPatch<Patch>> {
  using Base = BaseClearablePatch<Patch, ListPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  template <typename C = T>
  static ListPatch createAppend(C&& values) {
    ListPatch result;
    *result.patch_.append() = std::forward<C>(values);
    return result;
  }

  template <typename C = T>
  static ListPatch createPrepend(C&& values) {
    ListPatch result;
    *result.patch_.prepend() = std::forward<C>(values);
    return result;
  }

  template <typename C = T>
  void append(C&& rhs) {
    auto& lhs = assignOr(*patch_.append());
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
  }
  template <typename... Args>
  void emplace_back(Args&&... args) {
    assignOr(*patch_.append()).emplace_back(std::forward<Args>(args)...);
  }
  template <typename U = typename T::value_type>
  void push_back(U&& val) {
    assignOr(*patch_.append()).push_back(std::forward<U>(val));
  }

  template <typename C = T>
  void prepend(C&& lhs) {
    auto& rhs = assignOr(*patch_.prepend());
    rhs.insert(rhs.begin(), lhs.begin(), lhs.end());
  }
  template <typename... Args>
  void emplace_front(Args&&... args) {
    // TODO(afuller): Switch prepend to a std::forward_list.
    auto& prepend = assignOr(*patch_.prepend());
    prepend.emplace(prepend.begin(), std::forward<Args>(args)...);
  }
  template <typename U = typename T::value_type>
  void push_front(U&& val) {
    // TODO(afuller): Switch prepend to a std::forward_list.
    auto& prepend = assignOr(*patch_.prepend());
    prepend.insert(prepend.begin(), std::forward<U>(val));
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      if (patch_.clear() == true) {
        val.clear();
      }
      val.insert(
          val.begin(), patch_.prepend()->begin(), patch_.prepend()->end());
      val.insert(val.end(), patch_.append()->begin(), patch_.append()->end());
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      // TODO(afuller): Optimize the r-value reference case.
      if (!next.get().prepend()->empty()) {
        decltype(auto) rhs = *std::forward<U>(next).get().prepend();
        patch_.prepend()->insert(
            patch_.prepend()->begin(), rhs.begin(), rhs.end());
      }
      if (!next.get().append()->empty()) {
        decltype(auto) rhs = *std::forward<U>(next).get().append();
        patch_.append()->reserve(patch_.append()->size() + rhs.size());
        auto inserter = std::back_inserter(*patch_.append());
        std::copy_n(rhs.begin(), rhs.size(), inserter);
      }
    }
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssignAndClear;
  using Base::patch_;
};

// Patch must have the following fields:
//   optional set<T> assign;
//   bool clear;
//   set<T> add;
//   set<T> remove;
template <typename Patch>
class SetPatch : public BaseClearablePatch<Patch, SetPatch<Patch>> {
  using Base = BaseClearablePatch<Patch, SetPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  template <typename C = T>
  static SetPatch createAdd(C&& keys) {
    SetPatch result;
    *result.patch_.add() = std::forward<C>(keys);
    return result;
  }

  template <typename C = T>
  static SetPatch createRemove(C&& keys) {
    SetPatch result;
    *result.patch_.remove() = std::forward<C>(keys);
    return result;
  }

  template <typename C = T>
  void add(C&& keys) {
    erase_all(*patch_.remove(), keys);
    assignOr(*patch_.add()).insert(keys.begin(), keys.end());
  }
  template <typename... Args>
  void emplace(Args&&... args) {
    if (patch_.assign().has_value()) {
      patch_.assign()->emplace(std::forward<Args>(args)...);
      return;
    }
    auto result = patch_.add()->emplace(std::forward<Args>(args)...);
    if (result.second) {
      patch_.remove()->erase(*result.first);
    }
  }
  template <typename U = typename T::value_type>
  void insert(U&& val) {
    if (patch_.assign().has_value()) {
      patch_.assign()->insert(std::forward<U>(val));
      return;
    }
    patch_.remove()->erase(val);
    patch_.add()->insert(std::forward<U>(val));
  }

  template <typename C = T>
  void remove(C&& keys) {
    if (patch_.assign().has_value()) {
      erase_all(*patch_.assign(), keys);
      return;
    }
    erase_all(*patch_.add(), keys);
    patch_.remove()->insert(keys.begin(), keys.end());
  }
  template <typename U = typename T::value_type>
  void erase(U&& val) {
    assignOr(*patch_.add()).erase(val);
    assignOr(*patch_.remove()).insert(std::forward<U>(val));
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      if (patch_.clear() == true) {
        val.clear();
      } else {
        erase_all(val, *patch_.remove());
      }
      val.insert(patch_.add()->begin(), patch_.add()->end());
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      remove(*std::forward<U>(next).get().remove());
      add(*std::forward<U>(next).get().add());
    }
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssignAndClear;
  using Base::patch_;
};

// Patch must have the following fields:
//   optional map<K, V> assign;
//   bool clear;
//   map<K, V> put;
template <typename Patch>
class MapPatch : public BaseClearablePatch<Patch, MapPatch<Patch>> {
  using Base = BaseClearablePatch<Patch, MapPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  template <typename C = T>
  static MapPatch createPut(C&& entries) {
    MapPatch result;
    *result.patch_.put() = std::forward<C>(entries);
    return result;
  }

  template <typename C = T>
  void put(C&& entries) {
    auto& field = assignOr(*patch_.put());
    for (auto&& entry : entries) {
      field.insert_or_assign(
          std::forward<decltype(entry)>(entry).first,
          std::forward<decltype(entry)>(entry).second);
    }
  }
  template <typename K, typename V>
  void insert_or_assign(K&& key, V&& value) {
    assignOr(*patch_.put())
        .insert_or_assign(std::forward<K>(key), std::forward<V>(value));
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      if (patch_.clear() == true) {
        val.clear();
      }
      for (const auto& entry : *patch_.put()) {
        val.insert_or_assign(entry.first, entry.second);
      }
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      put(*std::forward<U>(next).get().put());
    }
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssignAndClear;
  using Base::patch_;
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
