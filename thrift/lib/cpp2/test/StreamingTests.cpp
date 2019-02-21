/*
 * Copyright 2004-present Facebook, Inc.
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

#include <folly/portability/GTest.h>

#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/async/StreamPublisher.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DiffTypesStreamingService.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

using namespace ::testing;
using namespace apache::thrift;

class DiffTypesStreamingService
    : public streaming_tests::DiffTypesStreamingServiceSvIf {
 public:
  explicit DiffTypesStreamingService(folly::EventBase& evb) : evb_(evb) {}

  apache::thrift::Stream<int32_t> downloadObject(int64_t) override {
    return toStream(yarpl::flowable::Flowable<int32_t>::just(42), &evb_);
  }

  apache::thrift::SemiStream<int32_t> clientDownloadObject(int64_t) {
    return toStream(yarpl::flowable::Flowable<int32_t>::just(42), &evb_);
  }

 protected:
  // Will never be needed to executed
  folly::EventBase& evb_;
};

TEST(StreamingTest, DifferentStreamClientCompiles) {
  folly::EventBase evb_;

  std::unique_ptr<streaming_tests::DiffTypesStreamingServiceAsyncClient>
      client = nullptr;

  DiffTypesStreamingService service(evb_);
  apache::thrift::SemiStream<int32_t> result;
  if (client) { // just to also test compilation of the client side.
    result = client->sync_downloadObject(123L);
  } else {
    result = service.clientDownloadObject(123L);
  }
  auto subscription = std::move(result).via(&evb_).subscribe([](int32_t) {});
  subscription.cancel();
  std::move(subscription).detach();
}

TEST(StreamingTest, StreamPublisherCancellation) {
  class SlowExecutor : public folly::SequencedExecutor,
                       public folly::DefaultKeepAliveExecutor {
   public:
    ~SlowExecutor() override {
      joinKeepAlive();
    }

    void add(folly::Func func) override {
      impl_.add([f = std::move(func)]() mutable {
        /* sleep override */ std::this_thread::sleep_for(
            std::chrono::milliseconds{5});
        f();
      });
    }

   private:
    folly::ScopedEventBaseThread impl_;
  };
  SlowExecutor executor;

  auto streamAndPublisher = apache::thrift::StreamPublisher<int>::create(
      folly::getKeepAliveToken(executor), [] {});

  int count = 0;

  auto subscription =
      std::move(streamAndPublisher.first)
          .subscribe(
              [&count](int value) mutable { EXPECT_EQ(count++, value); },
              apache::thrift::Stream<int>::kNoFlowControl);

  /* sleep override */ std::this_thread::sleep_for(
      std::chrono::milliseconds{100});

  std::atomic<bool> stop{false};
  std::thread publisherThread([&] {
    for (int i = 0; !stop; ++i) {
      streamAndPublisher.second.next(i);
      /* sleep override */ std::this_thread::sleep_for(
          std::chrono::milliseconds{1});
    }
  });

  /* sleep override */ std::this_thread::sleep_for(
      std::chrono::milliseconds{10});
  subscription.cancel();
  /* sleep override */ std::this_thread::sleep_for(
      std::chrono::milliseconds{100});
  stop = true;

  std::move(subscription).join();
  EXPECT_GT(count, 0);

  publisherThread.join();
}

TEST(StreamingTest, StreamPublisherNoSubscription) {
  class SlowExecutor : public folly::SequencedExecutor,
                       public folly::DefaultKeepAliveExecutor {
   public:
    ~SlowExecutor() override {
      joinKeepAlive();
    }

    void add(folly::Func func) override {
      impl_.add([f = std::move(func)]() mutable {
        /* sleep override */ std::this_thread::sleep_for(
            std::chrono::milliseconds{5});
        f();
      });
    }

   private:
    folly::ScopedEventBaseThread impl_;
  };
  SlowExecutor executor;

  auto streamAndPublisher = apache::thrift::StreamPublisher<int>::create(
      folly::getKeepAliveToken(executor), [] {});
  std::exchange(streamAndPublisher.first, apache::thrift::Stream<int>());
  std::move(streamAndPublisher.second).complete();
}
