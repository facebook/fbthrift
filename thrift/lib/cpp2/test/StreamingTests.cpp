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
