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
  yarpl::Reference<yarpl::single::SingleObserver<std::unique_ptr<folly::IOBuf>>>
      subscriberRef;
  folly::Baton<> subscriberBaton;
  auto single =
      yarpl::single::Single<std::unique_ptr<folly::IOBuf>>::create(
          [&subscriberRef, &subscriberBaton](auto subscriber) mutable {
            subscriberRef = subscriber;
            subscriberBaton.post();
          })
          ->map([](auto buff) { return rsocket::Payload(std::move(buff)); });

  single->subscribe([](rsocket::Payload payload) mutable {
    CHECK_STREQ(
        payload.data->cloneCoalescedAsValue()
            .moveToFbString()
            .toStdString()
            .c_str(),
        "data");
  });

  subscriberBaton.wait();

  auto requestResponse =
      std::make_shared<RequestResponseThriftChannel>(&evb, subscriberRef);

  evb.runInEventBaseThread([requestResponse = std::move(requestResponse)]() {
    auto headers = std::make_unique<std::map<std::string, std::string>>();
    auto data = folly::IOBuf::copyBuffer(std::string("data"));
    requestResponse->sendThriftResponse(0, std::move(headers), std::move(data));
  });

  evb.loop();
}

TEST(RequestResponseThriftChannel, FailureResponse) {
  folly::EventBase evb;
  yarpl::Reference<yarpl::single::SingleObserver<std::unique_ptr<folly::IOBuf>>>
      subscriberRef;
  folly::Baton<> subscriberBaton;
  auto single =
      yarpl::single::Single<std::unique_ptr<folly::IOBuf>>::create(
          [&subscriberRef, &subscriberBaton](auto subscriber) mutable {
            subscriberRef = subscriber;
            subscriberBaton.post();
          })
          ->map([](auto buff) { return rsocket::Payload(std::move(buff)); });

  single->subscribe(
      [](rsocket::Payload) { FAIL() << "Error is expected"; },
      [](folly::exception_wrapper ex) {
        CHECK_STREQ(ex.get_exception()->what(), "mock_exception");
      });

  subscriberBaton.wait();

  auto requestResponse =
      std::make_shared<RequestResponseThriftChannel>(&evb, subscriberRef);

  evb.runInEventBaseThread([requestResponse = std::move(requestResponse)]() {
    auto headers = std::make_unique<std::map<std::string, std::string>>();
    auto data = folly::IOBuf::copyBuffer(std::string("data"));
    requestResponse->sendErrorWrapped(
        folly::exception_wrapper(), "mock_exception", nullptr);
  });

  evb.loop();
}
}
}
