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
#include <thrift/lib/cpp2/async/StreamCallbacks.h>

namespace apache {
namespace thrift {
namespace detail {

using ServerStreamFactory = folly::Function<
    void(FirstResponsePayload&&, StreamClientCallback*, folly::EventBase*)>;

template <typename T>
using ServerStreamFn = folly::Function<ServerStreamFactory(
    folly::Executor::KeepAlive<>,
    folly::Try<StreamPayload> (*)(folly::Try<T>&&))>;

} // namespace detail
} // namespace thrift
} // namespace apache
