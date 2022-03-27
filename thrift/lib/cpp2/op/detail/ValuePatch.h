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

// A patch adapter that only supports 'assign',
// which is the minimum any patch should support.
//
// Patch must have the following fields:
//   optional T assign;
template <typename Patch>
class AssignPatch : public BaseValuePatch<Patch, AssignPatch<Patch>> {
  using Base = BaseValuePatch<Patch, AssignPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  void apply(T& val) const { applyAssign(val); }
  template <typename U>
  void merge(U&& next) {
    mergeAssign(std::forward<U>(next));
  }

 private:
  using Base::applyAssign;
  using Base::mergeAssign;
};

// Patch must have the following fields:
//   optional T assign;
//   bool invert;
template <typename Patch>
class BoolPatch : public BaseValuePatch<Patch, BoolPatch<Patch>> {
  using Base = BaseValuePatch<Patch, BoolPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::get;
  using Base::operator=;

  static BoolPatch createInvert() { return !BoolPatch{}; }

  void invert() {
    auto& val = assignOr(*patch_.invert());
    val = !val;
  }

  void apply(T& val) const {
    if (!applyAssign(val) && *patch_.invert()) {
      val = !val;
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      *patch_.invert() ^= *next.get().invert();
    }
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;
  using Base::patch_;

  friend BoolPatch operator!(BoolPatch val) { return (val.invert(), val); }
};

// Patch must have the following fields:
//   optional T assign;
//   T add;
template <typename Patch>
class NumberPatch : public BaseValuePatch<Patch, NumberPatch<Patch>> {
  using Base = BaseValuePatch<Patch, NumberPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  template <typename U>
  static NumberPatch createAdd(U&& val) {
    NumberPatch patch;
    patch.add(std::forward<U>(val));
    return patch;
  }
  template <typename U>
  static NumberPatch createSubtract(U&& val) {
    NumberPatch patch;
    patch.subtract(std::forward<U>(val));
    return patch;
  }

  template <typename U>
  void add(U&& val) {
    assignOr(*patch_.add()) += std::forward<U>(val);
  }

  template <typename U>
  void subtract(U&& val) {
    assignOr(*patch_.add()) -= std::forward<U>(val);
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      val += *patch_.add();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      *patch_.add() += *next.get().add();
    }
  }

  template <typename U>
  NumberPatch& operator+=(U&& val) {
    add(std::forward<U>(val));
    return *this;
  }

  template <typename U>
  NumberPatch& operator-=(U&& val) {
    subtract(std::forward<T>(val));
    return *this;
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;
  using Base::patch_;

  template <typename U>
  friend NumberPatch operator+(NumberPatch lhs, U&& rhs) {
    lhs.add(std::forward<U>(rhs));
    return lhs;
  }

  template <typename U>
  friend NumberPatch operator+(U&& lhs, NumberPatch rhs) {
    rhs.add(std::forward<U>(lhs));
    return rhs;
  }

  template <typename U>
  friend NumberPatch operator-(NumberPatch lhs, U&& rhs) {
    lhs.subtract(std::forward<U>(rhs));
    return lhs;
  }
};

// Patch must have the following fields:
//   optional T assign;
//   bool clear;
//   T append;
//   T prepend;
template <typename Patch>
class StringPatch : public BaseClearablePatch<Patch, StringPatch<Patch>> {
  using Base = BaseClearablePatch<Patch, StringPatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;

  template <typename... Args>
  static StringPatch createAppend(Args&&... args) {
    StringPatch patch;
    patch.append(std::forward<Args>(args)...);
    return patch;
  }

  template <typename U>
  static StringPatch createPrepend(U&& val) {
    StringPatch patch;
    patch.prepend(std::forward<U>(val));
    return patch;
  }

  template <typename... Args>
  void append(Args&&... args) {
    assignOr(*patch_.append()).append(std::forward<Args>(args)...);
  }

  template <typename U>
  void prepend(U&& val) {
    T& cur = assignOr(*patch_.prepend());
    cur = std::forward<U>(val) + std::move(cur);
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      if (patch_.clear() == true) {
        val.clear();
      }
      val = *patch_.prepend() + std::move(val) + *patch_.append();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      *patch_.prepend() =
          *std::forward<U>(next).get().prepend() + std::move(*patch_.prepend());
      patch_.append()->append(*std::forward<U>(next).get().append());
    }
  }

  template <typename U>
  StringPatch& operator+=(U&& val) {
    assignOr(*patch_.append()) += std::forward<U>(val);
    return *this;
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssignAndClear;
  using Base::patch_;

  template <typename U>
  friend StringPatch operator+(StringPatch lhs, U&& rhs) {
    return lhs += std::forward<U>(rhs);
  }
  template <typename U>
  friend StringPatch operator+(U&& lhs, StringPatch rhs) {
    rhs.prepend(std::forward<U>(lhs));
    return rhs;
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
