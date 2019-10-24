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

#include <thrift/lib/py3/stream.h>

#include <folly/portability/GTest.h>

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/async/StreamGenerator.h>

namespace thrift {
namespace py3 {
namespace test {

using namespace apache::thrift;

TEST(SemiStreamWrapperTest, testGetNext) {
  folly::coro::blockingWait([]() -> folly::coro::Task<void> {
    folly::ScopedEventBaseThread th;
    int length = 3;
    int i = 0;
    Stream<int> stream = StreamGenerator::create(
        folly::getKeepAliveToken(th.getEventBase()),
        [&]() -> folly::Optional<int> {
          if (i < length) {
            return i++;
          }

          return folly::none;
        });

    SemiStream<int> semi = SemiStream<int>(std::move(stream));
    SemiStreamWrapper<int> wrapper{semi, 2};
    auto v = co_await wrapper.getNext();
    EXPECT_EQ(v, 0);
    v = co_await wrapper.getNext();
    EXPECT_EQ(v, 1);
    v = co_await wrapper.getNext();
    EXPECT_EQ(v, 2);
    v = co_await wrapper.getNext();
    EXPECT_EQ(v, folly::none);
  }());
}
} // namespace test
} // namespace py3
} // namespace thrift
