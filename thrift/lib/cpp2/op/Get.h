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

#include <folly/Traits.h>
#include <thrift/lib/cpp/FieldId.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <FieldId Id>
struct GetById {
 private:
  using access = thrift::detail::st::struct_private_access;
  template <typename T>
  using ident_tag = access::ident_tag<folly::remove_cvref_t<T>, Id>;

 public:
  template <typename T>
  FOLLY_ERASE constexpr auto operator()(T&& t) const
      noexcept(noexcept(access_field<ident_tag<T>>(static_cast<T&&>(t))))
          -> decltype(access_field<ident_tag<T>>(static_cast<T&&>(t))) {
    return access_field<ident_tag<T>>(static_cast<T&&>(t));
  }
};

} // namespace detail

// Gets a field by FieldId.
template <FieldId Id>
FOLLY_INLINE_VARIABLE constexpr auto getById = detail::GetById<Id>{};

} // namespace op
} // namespace thrift
} // namespace apache
