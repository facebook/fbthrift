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

#include <fatal/type/search.h>
#include <fatal/type/slice.h>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/Traits.h>

namespace apache {
namespace thrift {

namespace detail {
namespace st {
struct struct_private_access;
} // namespace st
} // namespace detail

namespace type {
namespace detail {

struct field_to_id {
  template <class>
  struct apply;

  template <FieldId Id, class Tag>
  struct apply<field_t<Id, Tag>> {
    static constexpr auto value = folly::to_underlying(Id);
  };
};

template <class StructTag, FieldId Id>
class field_tag {
 private:
  using fields = ::apache::thrift::detail::st::struct_private_access::fields<
      typename detail::traits<StructTag>::native_type>;

  using sorted_fields = fatal::sort_by<fields, field_to_id>;

  static constexpr auto index() {
    auto ret = fatal::size<fields>::value; // return size(fields) if not found
    fatal::sorted_search<sorted_fields, field_to_id>(
        Id, [&](auto index) { ret = index.value; });
    return ret;
  }

 public:
  using type = fatal::try_at<sorted_fields, index(), void>;
};

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
