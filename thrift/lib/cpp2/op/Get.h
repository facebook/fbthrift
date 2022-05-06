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

#include <thrift/lib/cpp/FieldId.h>
#include <thrift/lib/cpp2/op/detail/Get.h>

namespace apache {
namespace thrift {
namespace op {

template <typename Tag, typename IdentTag>
struct GetByIdent {
  template <typename T>
  FOLLY_ERASE constexpr decltype(auto) operator()(T&& t) const
      noexcept(noexcept(detail::get<IdentTag>(Tag{}, static_cast<T&&>(t)))) {
    return detail::get<IdentTag>(Tag{}, static_cast<T&&>(t));
  }
};

// Gets a field by identifier tag.
template <typename Tag, typename IdentTag>
FOLLY_INLINE_VARIABLE constexpr GetByIdent<Tag, IdentTag> getByIdent{};

template <typename Tag, FieldId Id>
struct GetById {
  template <typename T>
  FOLLY_ERASE constexpr decltype(auto) operator()(T&& t) const
      noexcept(noexcept(detail::get<Id>(Tag{}, static_cast<T&&>(t)))) {
    return detail::get<Id>(Tag{}, static_cast<T&&>(t));
  }
};

// Gets a field by FieldId.
template <typename Tag, FieldId Id>
FOLLY_INLINE_VARIABLE constexpr GetById<Tag, Id> getById{};

} // namespace op
} // namespace thrift
} // namespace apache
