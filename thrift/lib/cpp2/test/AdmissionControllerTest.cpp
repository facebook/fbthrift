/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <thrift/lib/cpp2/server/QIAdmissionController.h>

#include <chrono>

#include <gtest/gtest.h>

#include <folly/Random.h>

#include <thrift/lib/cpp2/test/util/FakeClock.h>

using namespace apache::thrift;
using namespace std::chrono;
namespace apache {
namespace thrift {

FakeClock::time_point FakeClock::now_us_;

class DummyRequest : public ResponseChannelRequest {
  bool isActive() override {
    return true;
  }
  void cancel() override {}
  bool isOneway() override {
    return false;
  }
  void sendReply(std::unique_ptr<folly::IOBuf>&&, MessageChannel::SendCallback*)
      override {}

  void sendErrorWrapped(
      folly::exception_wrapper,
      std::string,
      MessageChannel::SendCallback*) override {}
};

class AdmissionControllerTest : public testing::Test {};

TEST_F(AdmissionControllerTest, admitFirstRequest) {
  QIAdmissionController<FakeClock> controller(seconds(1), seconds(5));

  // Fisrt request should always be accepted
  ASSERT_TRUE(controller.admit());
}

TEST_F(AdmissionControllerTest, firstReject) {
  constexpr int window = 5;
  constexpr int sla = 1;
  constexpr int minQueueLength = 10;
  QIAdmissionController<FakeClock> controller(
      seconds(sla), seconds(window), minQueueLength);

  // The min queue length is 10, so we mjust accept the first 10 messages
  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(controller.admit());
  }

  // all the subsequent requests will be rejected
  for (int i = 0; i < 100; i++) {
    ASSERT_FALSE(controller.admit());
  }
}

TEST_F(AdmissionControllerTest, steadyLowRPSTraffic) {
  constexpr int window = 5;
  constexpr int sla = 1;
  constexpr int minQueueLength = 10;
  QIAdmissionController<FakeClock> controller(
      seconds(sla), seconds(window), minQueueLength);

  // one request at the time, no rejection
  ASSERT_TRUE(controller.admit());
  FakeClock::advance(seconds(1));
  controller.dequeue();
  controller.returnedResponse(std::chrono::nanoseconds(1));

  // 2 concurrent requests, under the rate of responses/sec
  ASSERT_TRUE(controller.admit());
  FakeClock::advance(milliseconds(500));
  ASSERT_TRUE(controller.admit());
  FakeClock::advance(milliseconds(500));
  controller.dequeue();
  controller.returnedResponse(
      std::chrono::nanoseconds(1)); // response for the first req
  FakeClock::advance(milliseconds(500));
  controller.dequeue();
  controller.returnedResponse(
      std::chrono::nanoseconds(1)); // response for the second req

  // 5 concurrent requests, under the rate of responses/sec
  for (int i = 0; i < 5; i++) {
    ASSERT_TRUE(controller.admit());
    FakeClock::advance(milliseconds(500));
  }
}

TEST_F(AdmissionControllerTest, spikeAfterSteady) {
  constexpr int window = 5;
  constexpr int sla = 1;
  constexpr int minQueueLength = 10;
  QIAdmissionController<FakeClock> controller(
      seconds(sla), seconds(window), minQueueLength);

  // 100 req/resp to let the outgoing rate converge to 10RPS
  for (int i = 0; i < 200; i++) {
    ASSERT_TRUE(controller.admit());
    FakeClock::advance(milliseconds(100));
    controller.dequeue();
    controller.returnedResponse(std::chrono::nanoseconds(1));
  }

  // the previous steady state leave the queue empty
  auto admitted = 0;
  for (int i = 0; i < 100; i++) {
    if (controller.admit()) {
      admitted++;
    }
  }
  ASSERT_NEAR(admitted, minQueueLength, 2);
}

TEST_F(AdmissionControllerTest, steadyMaxRPS) {
  constexpr int window = 5;
  constexpr int sla = 1;
  constexpr int minQueueLength = 1;
  QIAdmissionController<FakeClock> controller(
      seconds(sla), seconds(window), minQueueLength);

  // 100 req/resp to let the outgoing rate converge to 10RPS
  for (int i = 0; i < 100; i++) {
    ASSERT_TRUE(controller.admit());
    FakeClock::advance(milliseconds(100));
    controller.dequeue();
    controller.returnedResponse(std::chrono::nanoseconds(1));
  }

  // queue limit should be equal to ~10 here
  // spike in traffic which fill up the queue (some will be rejected)
  for (int i = 0; i < 15; i++) {
    controller.admit();
  }

  // steady traffic at max capacity, at that point the algo will
  // reduce the queue limit
  auto rejected = 0;
  for (int i = 0; i < 100; i++) {
    if (!controller.admit()) {
      rejected++;
    }
    FakeClock::advance(milliseconds(100));
    controller.dequeue();
    controller.returnedResponse(std::chrono::nanoseconds(1));
  }

  ASSERT_NEAR(rejected, 10, 4);
}

} // namespace thrift
} // namespace apache
