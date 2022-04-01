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

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
