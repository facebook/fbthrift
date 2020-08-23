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

#include <folly/Traits.h>
#include <thrift/lib/cpp2/visitation/metadata.h>

namespace apache {
namespace thrift {
namespace detail {
template <class T>
struct ForEachField {
  static_assert(sizeof(T) < 0, "Must include visitation header");
};
} // namespace detail
/**
 * for_each_field iterates over fields in thrift struct. Example:
 *
 *   for_each_field(thriftObject, [](const ThriftField& meta, auto field_ref) {
 *     LOG(INFO) << *meta.name_ref() << " --> " << *field_ref;
 *   });
 *
 * ThriftField schema is defined here: https://git.io/JJQpY
 * If there are mixin fields, inner fields won't be iterated.
 * If `no_metadata` thrift option is enabled, ThriftField will be empty.
 *
 * @param t thrift object
 * @param f a callable that will be called for each thrift field
 */
template <typename T, typename F>
void for_each_field(T&& t, F&& f) {
  detail::ForEachField<folly::remove_cvref_t<T>>()(f, static_cast<T&&>(t));
}
} // namespace thrift
} // namespace apache
