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

#pragma once

#include <folly/Optional.h>
#include <folly/Portability.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {
namespace detail {
struct CopyToFollyOptional {
  template <class T>
  [[deprecated(
      "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
      Optional<std::remove_const_t<T>>
      operator()(optional_field_ref<T&> t) const {
    if (t) {
      return *t;
    }
    return {};
  }
};

struct MoveToFollyOptional {
  template <class T>
  [[deprecated(
      "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
      Optional<std::remove_const_t<T>>
      operator()(optional_field_ref<T&&> t) const {
    if (t) {
      return std::move(*t);
    }
    return {};
  }

  template <class T>
  [[deprecated(
      "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
      Optional<T>
      operator()(optional_field_ref<T&> t) const {
    if (t) {
      return std::move(*t);
    }
    return {};
  }
};

struct FromFollyOptional {
  template <class T>
  [[deprecated(
      "Use std::optional with optional_field_ref::from_optional(...) instead")]] auto
  operator()(optional_field_ref<T&> lhs, const folly::Optional<T>& rhs) const {
    if (rhs) {
      lhs = *rhs;
    } else {
      lhs.reset();
    }
    return lhs;
  }

  template <class T>
  [[deprecated(
      "Use std::optional with optional_field_ref::from_optional(...) instead")]] auto
  operator()(optional_field_ref<T&> lhs, folly::Optional<T>&& rhs) const {
    if (rhs) {
      lhs = std::move(*rhs);
    } else {
      lhs.reset();
    }

    return lhs;
  }
};

struct EqualToFollyOptional {
  template <class T>
  bool operator()(
      optional_field_ref<T> a,
      const folly::Optional<folly::remove_cvref_t<T>>& b) const {
    return a && b ? *a == *b : a.has_value() == b.has_value();
  }
};
} // namespace detail

constexpr detail::CopyToFollyOptional copyToFollyOptional;
constexpr detail::MoveToFollyOptional moveToFollyOptional;
constexpr detail::FromFollyOptional fromFollyOptional;
constexpr detail::EqualToFollyOptional equalToFollyOptional;

} // namespace thrift
} // namespace apache
