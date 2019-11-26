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

#include "thrift/lib/cpp2/async/tests/util/TestStreamService.h"

namespace testutil {
namespace testservice {

apache::thrift::ServerStream<int32_t> TestStreamService::range(
    int32_t from,
    int32_t to) {
  return folly::coro::
      co_invoke([=]() -> folly::coro::AsyncGenerator<int32_t&&> {
        for (int i = from; i <= to; i++) {
          co_yield std::move(i);
        }
      });
}

apache::thrift::ServerStream<int32_t> TestStreamService::rangeThrow(
    int32_t from,
    int32_t to) {
  return folly::coro::
      co_invoke([=]() -> folly::coro::AsyncGenerator<int32_t&&> {
        for (int i = from; i <= to; i++) {
          co_yield std::move(i);
        }
        throw std::runtime_error("I am a search bar");
      });
}

apache::thrift::ServerStream<int32_t> TestStreamService::rangeThrowUDE(
    int32_t from,
    int32_t to) {
  return folly::coro::
      co_invoke([=]() -> folly::coro::AsyncGenerator<int32_t&&> {
        for (int i = from; i <= to; i++) {
          co_yield std::move(i);
        }
        throw UserDefinedException();
      });
}

} // namespace testservice
} // namespace testutil
