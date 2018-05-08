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

#include <thrift/lib/cpp2/transport/rsocket/server/test/RSResponderTestFixture.h>

#include <folly/io/async/EventBase.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/transport/core/testutil/CoreTestFixture.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponse.h>
#include <yarpl/flowable/Flowable.h>

namespace apache {
namespace thrift {

using namespace testing;
using namespace testutil::testservice;

TEST_F(RSResponderTestFixture, RequestResponse_Simple) {
  EXPECT_CALL(service_, sumTwoNumbers_(5, 10));

  eventBase_.runInEventBaseThread([this]() {
    folly::IOBufQueue request;
    auto metadata = std::make_unique<RequestRpcMetadata>();
    CoreTestFixture::serializeSumTwoNumbers(
        5, 10, false, &request, metadata.get());
    auto metaBuf = RSocketClientChannel::serializeMetadata(*metadata);

    responder_->handleRequestResponse(
        rsocket::Payload(request.move(), std::move(metaBuf)),
        0,
        yarpl::single::SingleObserver<rsocket::Payload>::create(
            [](auto payload) {
              auto result =
                  CoreTestFixture::deserializeSumTwoNumbers(payload.data.get());
              EXPECT_EQ(result, 15);
            },
            [](folly::exception_wrapper) {
              FAIL() << "No error is expected";
            }));
  });

  eventBase_.loop();
  threadManager_->join();
}

TEST_F(RSResponderTestFixture, RequestResponse_MissingRPCMethod) {
  eventBase_.runInEventBaseThread([this]() {
    folly::IOBufQueue request;
    auto metadata = std::make_unique<RequestRpcMetadata>();
    CoreTestFixture::serializeSumTwoNumbers(
        5, 10, true, &request, metadata.get());
    auto metaBuf = RSocketClientChannel::serializeMetadata(*metadata);

    responder_->handleRequestResponse(
        rsocket::Payload(request.move(), std::move(metaBuf)),
        0,
        yarpl::single::SingleObserver<rsocket::Payload>::create(
            [](auto payload) {
              EXPECT_THAT(
                  payload.data->cloneCoalescedAsValue()
                      .moveToFbString()
                      .toStdString(),
                  HasSubstr("not found"));
            },
            [](folly::exception_wrapper) {
              FAIL() << "No error is expected";
            }));
  });

  eventBase_.loop();
  threadManager_->join();
}
} // namespace thrift
} // namespace apache
