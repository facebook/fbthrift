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

#include <thrift/lib/cpp2/Thrift.h>

namespace apache {
namespace thrift {
namespace op {

template <FieldId Id>
struct access_field_by_id_fn {
 private:
  using access = apache::thrift::detail::st::struct_private_access;

  using under_t = std::underlying_type_t<FieldId>;
  static constexpr under_t under = folly::to_underlying(Id);
  using under_c = std::integral_constant<under_t, under>;

 public:
  template <
      typename T,
      typename...,
      typename Tag = access::__fbthrift_get<folly::remove_cvref_t<T>, under_c>>
  FOLLY_ERASE constexpr auto operator()(T&& t) const
      noexcept(noexcept(access_field<Tag>(static_cast<T&&>(t))))
          -> decltype(access_field<Tag>(static_cast<T&&>(t))) {
    return access_field<Tag>(static_cast<T&&>(t));
  }
};

// Gets a field by FieldId.
template <FieldId Id>
FOLLY_INLINE_VARIABLE constexpr access_field_by_id_fn<Id> access_field_by_id{};
template <FieldId Id>
static constexpr auto& getById = access_field_by_id<Id>;

} // namespace op
} // namespace thrift
} // namespace apache
