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

#include <utility>

namespace apache {
namespace thrift {
namespace adapt_detail {

template <typename Adapter, typename AdaptedType, typename NativeType>
void fromThrift(AdaptedType& adapted, NativeType&& native) {
  adapted = Adapter::fromThrift(std::forward<NativeType>(native));
}

// The type returned by the adapter for the given thrift type.
template <typename Adapter, typename ThriftType>
using adapted_t = decltype(Adapter::fromThrift(std::declval<ThriftType&&>()));

} // namespace adapt_detail
} // namespace thrift
} // namespace apache
