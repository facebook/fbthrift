/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/StreamGenerator.h>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/BlockingWait.h>
#endif
#include <folly/io/async/ScopedEventBaseThread.h>

namespace apache {
namespace thrift {

TEST(StreamGeneratorFutureTest, Basic) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }

        return folly::none;
      });

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
}

TEST(StreamGeneratorFutureTest, FutureBasic) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      });

  // intentionally let promised fullfilled in reverse order, but the result
  // should come back to stream in order
  for (int i = length - 1; i >= 0; i--) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
}

#if FOLLY_HAS_COROUTINES
TEST(StreamGeneratorFutureTest, CoroGeneratorBasic) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }

        return folly::none;
      });

  int expected_i = 0;
  SemiStream<int> semi = SemiStream<int>(std::move(stream));
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto gen = toAsyncGenerator<int>(std::move(semi), 100);
    auto it = co_await gen.begin();
    int val;
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, val);
      co_await(++it);
    }
    EXPECT_EQ(length, val + 1);
  }());
}

TEST(StreamGeneratorFutureTest, CoroGeneratorMap) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }

        return folly::none;
      });

  int expected_i = 0;
  SemiStream<std::string> semi =
      SemiStream<int>(std::move(stream)).map([](int i) {
        return folly::to<std::string>(i);
      });

  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto gen = toAsyncGenerator<std::string>(std::move(semi), 100);
    auto it = co_await gen.begin();
    std::string val;
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, folly::to<int>(val));
      co_await(++it);
    }
    EXPECT_EQ(length, folly::to<int>(val) + 1);
  }());
}

TEST(StreamGeneratorFutureTest, CoroGeneratorSmallBuffer) {
  folly::ScopedEventBaseThread th;
  int length = 100;
  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }

        return folly::none;
      });

  int expected_i = 0;
  SemiStream<int> semi = SemiStream<int>(std::move(stream));
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto gen = toAsyncGenerator<int>(std::move(semi), 3);
    auto it = co_await gen.begin();
    int val = 0;
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, val);
      co_await(++it);
    }
    EXPECT_EQ(length, val + 1);
  }());
}

TEST(StreamGeneratorFutureTest, CoroGeneratorWithFuture) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 10;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      });

  // intentionally let promised fullfilled in reverse order, but the result
  // should come back to stream in order
  for (int i = length - 1; i >= 0; i--) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  SemiStream<int> semi = SemiStream<int>(std::move(stream));
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto gen = toAsyncGenerator<int>(std::move(semi), 5);
    auto it = co_await gen.begin();
    int val = 0;
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, val);
      co_await(++it);
    }
    EXPECT_EQ(length, val + 1);
  }());
}

TEST(StreamGeneratorFutureTest, CoroGeneratorWithError) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;
  std::string errorMsg = "error message";
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }
        throw std::runtime_error(errorMsg);
        return folly::none;
      });

  int expected_i = 0;
  SemiStream<int> semi = SemiStream<int>(std::move(stream));
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    try {
      auto gen = toAsyncGenerator<int>(std::move(semi), 100);
      auto it = co_await gen.begin();
      int val = 0;
      while (it != gen.end()) {
        val = *it;
        EXPECT_EQ(expected_i++, val);
        co_await(++it);
      }
    } catch (const std::exception& e) {
      EXPECT_EQ(errorMsg, e.what());
    }
  }());
}

TEST(StreamGeneratorFutureTest, CoroGeneratorWithCancel) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 10;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      });

  // intentionally let promised fullfilled in reverse order, but the result
  // should come back to stream in order
  for (int i = length - 1; i >= 0; i--) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  SemiStream<int> semi = SemiStream<int>(std::move(stream));
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto gen = toAsyncGenerator<int>(std::move(semi), 5);
    auto it = co_await gen.begin();
    int val = 0;
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, val);
      co_await(++it);
      break;
    }
    EXPECT_GT(length, val + 1);
    // exit coroutine early will send cancel to the stream
  }());
}
#endif

} // namespace thrift
} // namespace apache
