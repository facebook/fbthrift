/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/io/async/EventBase.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponseThriftChannel.h>

namespace apache {
namespace thrift {

using namespace testing;

TEST(RequestResponseThriftChannel, SuccessResponse) {
  folly::EventBase evb;
  yarpl::Reference<yarpl::single::SingleObserver<rsocket::Payload>>
      subscriberRef;
  folly::Baton<> subscriberBaton;
  auto single = yarpl::single::Single<rsocket::Payload>::create(
      [&subscriberRef, &subscriberBaton](auto subscriber) mutable {
        subscriberRef = subscriber;
        subscriberBaton.post();
      });

  single->subscribe([](rsocket::Payload payload) mutable {
    CHECK_STREQ(
        "data",
        payload.data->cloneCoalescedAsValue()
            .moveToFbString()
            .toStdString()
            .c_str());
  });

  subscriberBaton.wait();

  auto requestResponse =
      std::make_shared<RequestResponseThriftChannel>(&evb, subscriberRef);

  evb.runInEventBaseThread([requestResponse = std::move(requestResponse)]() {
    auto metadata = std::make_unique<ResponseRpcMetadata>();
    auto data = folly::IOBuf::copyBuffer(std::string("data"));
    requestResponse->sendThriftResponse(std::move(metadata), std::move(data));
  });

  evb.loop();
}

} // namespace thrift
} // namespace apache
