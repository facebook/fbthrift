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

#include <thrift/lib/cpp2/Object.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace detail {

struct ApplyPatch {
  // Applies 'patch' to 'value' in-place.
  void operator()(const Object& patch, protocol::Value& value) const;
  void operator()(const Object& patch, bool& value) const;
};

} // namespace detail

// TODO: Document.
FOLLY_INLINE_VARIABLE constexpr detail::ApplyPatch applyPatch{};

} // namespace protocol
} // namespace thrift
} // namespace apache
