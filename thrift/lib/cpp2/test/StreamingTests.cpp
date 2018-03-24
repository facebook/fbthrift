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

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DiffTypesStreamingService.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

using namespace ::testing;
using namespace apache::thrift;

class DiffTypesStreamingService
    : public streaming_tests::DiffTypesStreamingServiceSvIf {
 public:
  apache::thrift::Stream<int32_t> uploadObject(
      apache::thrift::SemiStream<std::string> chunks,
      int64_t) override {
    return std::move(chunks)
        .map([](auto chunk) { return static_cast<int32_t>(chunk.size()); })
        .via(&evb_);
  }

  apache::thrift::SemiStream<int32_t> clientUploadObject(
      apache::thrift::Stream<std::string> chunks,
      int64_t) {
    return std::move(chunks).map(
        [](auto chunk) { return static_cast<int32_t>(chunk.size()); });
  }

 protected:
  // Will never be needed to executed
  folly::EventBase evb_;
};

TEST(StreamingTest, DifferentStreamClientCompiles) {
  folly::EventBase evb_;

  std::unique_ptr<streaming_tests::DiffTypesStreamingServiceAsyncClient>
      client = nullptr;

  DiffTypesStreamingService service;
  auto flowable =
      yarpl::flowable::internal::flowableFromSubscriber<std::string>(
          [](auto subscriber) {
            subscriber->onSubscribe(yarpl::flowable::Subscription::create());
            subscriber->onNext(std::string("foobar"));
            subscriber->onComplete();
          });

  apache::thrift::SemiStream<int32_t> result;
  if (client) { // just to also test compilation of the client side.
    result =
        client->sync_uploadObject(toStream(std::move(flowable), &evb_), 123L);
  } else {
    result =
        service.clientUploadObject(toStream(std::move(flowable), &evb_), 123L);
  }
  toFlowable(std::move(result).via(&evb_))->subscribe([](int32_t) {});
}
