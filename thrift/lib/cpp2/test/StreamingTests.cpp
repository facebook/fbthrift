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

#include <folly/portability/GTest.h>

#include <folly/Portability.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DiffTypesStreamingService.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/BlockingWait.h>
#endif

using namespace ::testing;
using namespace apache::thrift;

class DiffTypesStreamingService
    : public streaming_tests::DiffTypesStreamingServiceSvIf {
 public:
  explicit DiffTypesStreamingService(folly::EventBase& evb) : evb_(evb) {}

  apache::thrift::ServerStream<int32_t> downloadObject(int64_t) override {
    return []() -> folly::coro::AsyncGenerator<int32_t&&> {
      co_yield 42;
    }
    ();
  }

  apache::thrift::ClientBufferedStream<int32_t> clientDownloadObject(int64_t) {
    return downloadObject(0).toClientStream(&evb_);
  }

 protected:
  // Will never be needed to executed
  folly::EventBase& evb_;
};

TEST(StreamingTest, DifferentStreamClientCompiles) {
  folly::ScopedEventBaseThread evb_;

  std::unique_ptr<streaming_tests::DiffTypesStreamingServiceAsyncClient>
      client = nullptr;

  DiffTypesStreamingService service(*evb_.getEventBase());
  apache::thrift::ClientBufferedStream<int32_t> result;
  if (client) { // just to also test compilation of the client side.
    result = client->sync_downloadObject(123L);
  } else {
    result = service.clientDownloadObject(123L);
  }
  auto subscription = std::move(result).subscribeExTry(&evb_, [](auto&&) {});
  subscription.cancel();
  std::move(subscription).detach();
}

TEST(StreamingTest, StreamPublisherCancellation) {
  std::atomic<bool> stop{false};
  auto streamAndPublisher = apache::thrift::ServerStream<int>::createPublisher(
      [&]() { stop = true; });

  int count = 0;
  auto subscription =
      std::move(streamAndPublisher.first)
          .toClientStream()
          .subscribeExTry(folly::getEventBase(), [&count](auto value) mutable {
            if (value.hasValue()) {
              EXPECT_EQ(count++, *value);
            }
          });

  std::thread publisherThread([&] {
    for (int i = 0; !stop; ++i) {
      streamAndPublisher.second.next(i);
    }
  });

  subscription.cancel();

  std::move(subscription).join();
  publisherThread.join();
}

TEST(StreamingTest, StreamPublisherNoSubscription) {
  auto streamAndPublisher =
      apache::thrift::ServerStream<int>::createPublisher();
  std::exchange(
      streamAndPublisher.first,
      apache::thrift::ServerStream<int>::createEmpty());
  std::move(streamAndPublisher.second).complete();
}

#if FOLLY_HAS_COROUTINES
TEST(StreamingTest, DiffTypesStreamingServiceGeneratorCompiles) {
  class DiffTypesStreamingService
      : public streaming_tests::DiffTypesStreamingServiceSvIf {
   public:
    apache::thrift::ServerStream<int32_t> downloadObject(int64_t) override {
      return []() -> folly::coro::AsyncGenerator<int32_t&&> {
        co_yield 42;
      }
      ();
    }
  };
  DiffTypesStreamingService service;

  bool done = false;
  service.downloadObject(0).toClientStream().subscribeInline(
      [first = true, &done](folly::Try<int32_t>&& t) mutable {
        if (first) {
          EXPECT_EQ(*t, 42);
          first = false;
        } else {
          EXPECT_FALSE(t.hasValue() || t.hasException());
          done = true;
        }
      });
  EXPECT_TRUE(done);
}
#endif

TEST(StreamingTest, DiffTypesStreamingServicePublisherCompiles) {
  class DiffTypesStreamingService
      : public streaming_tests::DiffTypesStreamingServiceSvIf {
   public:
    apache::thrift::ServerStream<int32_t> downloadObject(int64_t) override {
      auto [stream, ptr] =
          apache::thrift::ServerStream<int32_t>::createPublisher();
      ptr.next(42);
      std::move(ptr).complete();
      return std::move(stream);
    }
  };
  DiffTypesStreamingService service;

  bool done = false;
  service.downloadObject(0).toClientStream().subscribeInline(
      [first = true, &done](folly::Try<int32_t>&& t) mutable {
        if (first) {
          EXPECT_EQ(*t, 42);
          first = false;
        } else {
          EXPECT_FALSE(t.hasValue() || t.hasException());
          done = true;
        }
      });
  EXPECT_TRUE(done);
}

TEST(StreamingTest, DiffTypesStreamingServiceWrappedStreamCompiles) {
  class DiffTypesStreamingService
      : public streaming_tests::DiffTypesStreamingServiceSvIf {
   public:
    apache::thrift::ServerStream<int32_t> downloadObject(int64_t) override {
      auto streamAndPublisher =
          apache::thrift::ServerStream<int>::createPublisher();
      streamAndPublisher.second.next(42);
      std::move(streamAndPublisher.second).complete();
      return std::move(streamAndPublisher.first);
    }
    folly::ScopedEventBaseThread executor;
  };
  DiffTypesStreamingService service;

  bool done = false;
  service.downloadObject(0).toClientStream().subscribeInline(
      [first = true, &done](folly::Try<int32_t>&& t) mutable {
        if (first) {
          EXPECT_EQ(*t, 42);
          first = false;
        } else {
          EXPECT_FALSE(t.hasValue() || t.hasException());
          done = true;
        }
      });
  EXPECT_TRUE(done);
}

#if FOLLY_HAS_COROUTINES
TEST(StreamingTest, WrapperToAsyncGenerator) {
  class DiffTypesStreamingService
      : public streaming_tests::DiffTypesStreamingServiceSvIf {
   public:
    apache::thrift::ServerStream<int32_t> downloadObject(int64_t) override {
      auto [stream, ptr] =
          apache::thrift::ServerStream<int32_t>::createPublisher();
      ptr.next(42);
      std::move(ptr).complete();
      return std::move(stream);
    }
  };
  DiffTypesStreamingService service;
  auto gen = service.downloadObject(0).toClientStream().toAsyncGenerator();
  folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
    auto next = co_await gen.next();
    EXPECT_EQ(*next, 42);
    EXPECT_FALSE(co_await gen.next());
  }());
}
#endif
