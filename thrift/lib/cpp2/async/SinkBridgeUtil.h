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
  void consume() { baton_.post(); }

  void canceled() { baton_.post(); }

  folly::coro::Task<void> wait() { return waitImpl(baton_); }

 private:
  // TODO(T88629984): This is implemented as a static function because
  // clang-9 + member function coroutines + ASAN == ICE. Revert D27688850
  // once everything using thrift sink is past clang-9.
  static folly::coro::Task<void> waitImpl(folly::coro::Baton& baton) {
    co_await baton;
  }
  folly::coro::Baton baton_;
};

} // namespace detail
} // namespace thrift
} // namespace apache

#endif
