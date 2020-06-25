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

#include <folly/Portability.h>

#if FOLLY_HAS_COROUTINES
#include <boost/variant.hpp>

#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>

namespace apache {
namespace thrift {
namespace detail {

using ClientMessage = boost::variant<folly::Try<StreamPayload>, int64_t>;
struct SinkComplete {};
using ServerMessage = boost::variant<folly::Try<StreamPayload>, SinkComplete>;

class CoroConsumer {
 public:
  void consume() {
    baton_.post();
  }

  [[noreturn]] void canceled() {
    folly::assume_unreachable();
  }

  folly::coro::Task<void> wait() {
    co_await baton_;
  }

 private:
  folly::coro::Baton baton_;
};

} // namespace detail
} // namespace thrift
} // namespace apache

#endif
