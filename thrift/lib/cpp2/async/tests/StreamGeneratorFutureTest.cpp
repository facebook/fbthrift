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

#include <folly/Conv.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/BlockingWait.h>
#endif
#include <folly/CancellationToken.h>
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

TEST(StreamGeneratorFutureTest, FutureLongStreamSmallFanout) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 100;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      },
      10);

  // future fullfilled with intervals
  for (int i = 0; i < length; i++) {
    pth.add([&, i]() {
      pth.getEventBase()->schedule(
          [&, i]() mutable { vp[i].setValue(i); },
          std::chrono::milliseconds(i));
    });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          50)
      .futureJoin()
      .get();
  EXPECT_EQ(expected_i, length);
}

TEST(StreamGeneratorFutureTest, FutureThrow) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 10;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  std::string errorMsg = "error message";
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }
        throw std::runtime_error(errorMsg);
      });

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  bool onError = false;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [&](folly::exception_wrapper ew) {
            onError = true;
            EXPECT_EQ(errorMsg, ew.get_exception()->what());
          },
          [] {},
          3)
      .futureJoin()
      .get();

  EXPECT_TRUE(onError);
}

TEST(StreamGeneratorFutureTest, FutureNoFlowControl) {
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

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          Stream<int>::kNoFlowControl)
      .futureJoin()
      .get();
}

// rely on client's cancel signal to stop the stream
TEST(StreamGeneratorFutureTest, FutureNoFlowControlNoEnd) {
  folly::ScopedEventBaseThread th;

  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [i = 0]() mutable -> folly::SemiFuture<folly::Optional<int>> {
        return makeSemiFuture(folly::Optional<int>(i++));
      },
      10);

  int expected_i = 0;
  int target = 101;
  folly::Baton<> b;
  auto subscription = std::move(stream).subscribe(
      [&](int i) {
        if (expected_i == target) {
          b.post();
        }
        EXPECT_EQ(expected_i++, i);
      },
      [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
      [&] {},
      Stream<int>::kNoFlowControl);
  b.wait();
  subscription.cancel();
  std::move(subscription).join();
  EXPECT_GE(expected_i, target);
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
  bool getError = false;
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
      getError = true;
      EXPECT_EQ(errorMsg, e.what());
    }
  }());
  EXPECT_TRUE(getError);
}

TEST(StreamGeneratorFutureTest, CoroGeneratorWithCancel) {
  int length = 10;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);
  int i = 0;

  {
    folly::ScopedEventBaseThread th;
    folly::ScopedEventBaseThread pth;
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
}

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorIntType) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;

  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::coro::AsyncGenerator<int> {
        for (int i = 0; i < length; i++) {
          co_yield i;
        }
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

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorStringType) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;

  Stream<std::string> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::coro::AsyncGenerator<std::string&&> {
        for (int i = 0; i < length; i++) {
          co_yield folly::to<std::string>(i);
        }
      });

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](std::string i) { EXPECT_EQ(expected_i++, folly::to<int>(i)); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
}

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorReverseFullfill) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<int>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::coro::AsyncGenerator<int> {
        while (i < length) {
          co_yield co_await vp[i++].getSemiFuture();
        }
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

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorLambdaGaptureVariable) {
  folly::ScopedEventBaseThread th;
  std::string t = "test";
  Stream<std::string> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&, t = std::move(t) ]() mutable
      -> folly::coro::AsyncGenerator<std::string&&> {
        co_yield std::move(t);
        co_return;
      });

  int expected_i = 0;
  std::string result;
  std::move(stream)
      .subscribe(
          [&](std::string t) { result = t; },
          [](folly::exception_wrapper ex) {
            CHECK(false) << "on Error " << ex.what();
          },
          [&] {},
          2)
      .futureJoin()
      .get();

  EXPECT_EQ("test", result);
}

TEST(StreamGeneratorFutureTest, ShouldNotHaveCoAwaitMoreThanOnce) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<int>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::coro::AsyncGenerator<int> {
        while (i < length) {
          co_yield co_await vp[i++].getSemiFuture();
        }
      });

  int expected_i = 0;
  auto subscription = std::move(stream).subscribe(
      [&](int i) { EXPECT_EQ(expected_i++, i); },
      [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
      [&] { EXPECT_EQ(expected_i, length); },
      5);
  // subscribe before fullfill future, expecting co_await on the future will
  // happen before setValue()
  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  std::move(subscription).futureJoin().get();
}

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorMap) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<std::string>> vp(length);

  int i = 0;
  Stream<int> stream =
      StreamGenerator::create(
          folly::getKeepAliveToken(th.getEventBase()),
          [&]() -> folly::coro::AsyncGenerator<std::string&&> {
            while (i < length) {
              co_yield co_await vp[i++].getSemiFuture();
            }
          })
          .map([](std::string s) { return folly::to<int>(s); });

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(folly::to<std::string>(i)); });
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

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorError) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<std::string>> vp(length);

  int i = 0;
  std::string errorMsg = "error message";
  Stream<int> stream =
      StreamGenerator::create(
          folly::getKeepAliveToken(th.getEventBase()),
          [&]() -> folly::coro::AsyncGenerator<std::string&&> {
            while (i < length) {
              co_yield co_await vp[i++].getSemiFuture();
            }
            throw std::runtime_error(errorMsg);
          })
          .map([](std::string s) { return folly::to<int>(s); });

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(folly::to<std::string>(i)); });
  }

  int expected_i = 0;
  bool getException = false;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [&](folly::exception_wrapper ex) {
            getException = true;
            EXPECT_EQ(errorMsg, ex.get_exception()->what());
          },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
  EXPECT_TRUE(getException);
}

TEST(StreamGeneratorFutureTest, CoroAsyncGeneratorPreemptiveCancel) {
  folly::ScopedEventBaseThread th;
  folly::coro::Baton b;
  bool canceled = false;
  Stream<std::string> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&](folly::CancellationToken token)
          -> folly::coro::AsyncGenerator<std::string&&> {
        // cancelCallback will be execute in the same event loop
        // as async generator
        folly::CancellationCallback cancelCallback(token, [&]() {
          canceled = true;
          b.post();
        });
        co_yield "first";
        co_await b;
        if (!canceled) {
          co_yield "never_reach";
        }
      });

  folly::Baton<> b1;
  auto subscription = std::move(stream).subscribe(
      [&](const std::string& value) {
        EXPECT_EQ("first", value);
        b1.post();
      },
      [&](folly::exception_wrapper) {},
      [&] {},
      2);
  b1.wait();
  subscription.cancel();
  std::move(subscription).join();
  EXPECT_TRUE(canceled);
}

TEST(StreamGeneratorFutureTest, CoroGeneratorWithCancellationToken) {
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

  folly::CancellationSource cancelSource;

  // Fulfill the first 5 requests and leave the rest unfulfilled.
  // Then after some delay trigger a request for cancellation.
  for (int j = 4; j >= 0; j--) {
    pth.add([&vp, j]() { vp[j].setValue(j); });
  }
  pth.add([&, cancelSource]() mutable {
    pth.getEventBase()->schedule(
        [cancelSource = std::move(cancelSource)] {
          cancelSource.requestCancellation();
        },
        std::chrono::milliseconds(50));
  });

  int expected_i = 0;
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto gen =
        toAsyncGenerator<int>(std::move(stream), 5, cancelSource.getToken());
    int val = 0;

    auto it = co_await gen.begin();
    while (it != gen.end()) {
      val = *it;
      EXPECT_EQ(expected_i++, val);
      co_await(++it);
    }

    EXPECT_TRUE(cancelSource.isCancellationRequested());

    EXPECT_EQ(5, val + 1);
  }());
}

#endif

} // namespace thrift
} // namespace apache
