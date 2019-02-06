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

#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/admission_strategy/GlobalAdmissionStrategy.h>
#include <thrift/lib/cpp2/server/admission_strategy/PerClientIdAdmissionStrategy.h>
#include <thrift/lib/cpp2/server/admission_strategy/PriorityAdmissionStrategy.h>
#include <thrift/lib/cpp2/server/admission_strategy/WhitelistAdmissionStrategy.h>

#include <chrono>

#include <gtest/gtest.h>

#include <folly/Random.h>

#include <thrift/lib/cpp2/test/util/FakeClock.h>

using namespace apache::thrift;

using apache::thrift::transport::THeader;

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

class DummyController : public AdmissionController {
 public:
  bool admit() override {
    return true;
  }
  void dequeue() override {}
  void returnedResponse() override {}
};

class DummyConnContext : public Cpp2ConnContext {
 public:
  DummyConnContext() {
    setRequestHeader(&reqHeader_);
  }

  void setReadHeader(const std::string& key, const std::string& value) {
    auto headers = reqHeader_.releaseHeaders();
    headers.insert({key, value});
    reqHeader_.setReadHeaders(std::move(headers));
  }

 private:
  THeader reqHeader_;
};

class DummyContextTest : public testing::Test {};

TEST_F(DummyContextTest, globalAdmission) {
  DummyConnContext ctx;
  ctx.setReadHeader("toto", "titi");
  ctx.setReadHeader("tutu", "tata");

  // Simulate below the way we extract the clientId from ConnContext
  std::string clientId = "NONE";
  auto header = ctx.getHeader();
  if (header != nullptr) {
    auto headers = header->getHeaders();
    auto it = headers.find("toto");
    if (it != headers.end()) {
      clientId = it->second;
    }
  }

  ASSERT_EQ(clientId, "titi");
}

class AdmissionControllerSelectorTest : public testing::Test {
 public:
  const std::string kClientId{"client_id"};
};

TEST_F(AdmissionControllerSelectorTest, globalAdmission) {
  GlobalAdmissionStrategy selector(std::make_shared<DummyController>());

  DummyRequest request;
  DummyConnContext connContextA1;
  connContextA1.setReadHeader(kClientId, "A");
  auto admissionControllerA1 =
      selector.select("myThriftMethod", request, connContextA1);

  DummyConnContext connContextA2;
  connContextA2.setReadHeader(kClientId, "A");
  auto admissionControllerA2 =
      selector.select("myThriftMethod", request, connContextA2);

  ASSERT_EQ(admissionControllerA1, admissionControllerA2);

  DummyConnContext connContextB1;
  connContextB1.setReadHeader(kClientId, "B");
  auto admissionControllerB1 =
      selector.select("myThriftMethod", request, connContextB1);

  ASSERT_EQ(admissionControllerA1, admissionControllerB1);

  DummyConnContext connContextNoClientId;
  auto admissionControllerNoClientId =
      selector.select("myThriftMethod", request, connContextNoClientId);

  ASSERT_EQ(admissionControllerB1, admissionControllerNoClientId);
}

TEST_F(AdmissionControllerSelectorTest, perClientIdAdmission) {
  PerClientIdAdmissionStrategy selector(
      []() { return std::make_shared<DummyController>(); }, kClientId);

  DummyRequest request;
  DummyConnContext connContextA1;
  connContextA1.setReadHeader(kClientId, "A");
  auto admissionControllerA1 =
      selector.select("myThriftMethod", request, connContextA1);

  DummyConnContext connContextA2;
  connContextA2.setReadHeader(kClientId, "A");
  auto admissionControllerA2 =
      selector.select("myThriftMethod", request, connContextA2);

  ASSERT_EQ(admissionControllerA1, admissionControllerA2);

  DummyConnContext connContextB1;
  connContextB1.setReadHeader(kClientId, "B");
  auto admissionControllerB1 =
      selector.select("myThriftMethod", request, connContextB1);

  ASSERT_NE(admissionControllerA1, admissionControllerB1);

  DummyConnContext connContextB2;
  connContextB2.setReadHeader(kClientId, "B");
  auto admissionControllerB2 =
      selector.select("myThriftMethod", request, connContextB2);

  ASSERT_EQ(admissionControllerB1, admissionControllerB2);
}

TEST_F(AdmissionControllerSelectorTest, priorityBasedAdmission) {
  std::unordered_map<std::string, uint8_t> priorities = {
      {"A", 1}, {"B", 5}, {"*", 1}};
  PriorityAdmissionStrategy selector(
      priorities,
      []() { return std::make_shared<DummyController>(); },
      kClientId);

  std::map<std::string, std::set<std::shared_ptr<AdmissionController>>>
      mapping = {{"A", std::set<std::shared_ptr<AdmissionController>>()},
                 {"B", std::set<std::shared_ptr<AdmissionController>>()},
                 {"*", std::set<std::shared_ptr<AdmissionController>>()}};

  for (auto& it : mapping) {
    auto& clientId = it.first;
    auto& admControllerSet = it.second;
    for (int i = 0; i < 5; i++) {
      DummyRequest request;
      DummyConnContext connContext;
      connContext.setReadHeader(kClientId, clientId);
      auto controller = selector.select("myThriftMethod", request, connContext);
      admControllerSet.insert(controller);
    }
  }

  // the # of admission controllers should be equal to the priority assigned
  // to a specific client_id
  for (auto& it : priorities) {
    ASSERT_EQ(mapping[it.first].size(), it.second);
  }
}

TEST_F(AdmissionControllerSelectorTest, deniesZeroPriority) {
  std::unordered_map<std::string, uint8_t> priorities = {{"A", 2}, {"B", 0}};
  PriorityAdmissionStrategy selector(
      priorities,
      []() { return std::make_shared<DummyController>(); },
      kClientId);

  std::map<std::string, std::set<std::shared_ptr<AdmissionController>>>
      mapping = {{"A", std::set<std::shared_ptr<AdmissionController>>()},
                 {"B", std::set<std::shared_ptr<AdmissionController>>()}};

  for (auto& it : mapping) {
    auto& clientId = it.first;
    auto& admControllerSet = it.second;
    for (int i = 0; i < 5; i++) {
      DummyRequest request;
      DummyConnContext connContext;
      connContext.setReadHeader(kClientId, clientId);
      auto controller = selector.select("myThriftMethod", request, connContext);
      admControllerSet.insert(controller);
    }
  }
  DummyRequest requestC;
  DummyConnContext connContextC;
  auto controllerForEmpty =
      selector.select("myThriftMethod", requestC, connContextC);
  connContextC.setReadHeader("client_id", "C");
  auto controllerForC =
      selector.select("myThriftMethod", requestC, connContextC);
  ASSERT_FALSE(controllerForC->admit());
  ASSERT_EQ(controllerForEmpty, controllerForC);

  // the # of admission controllers should be equal to the priority assigned
  // to a specific client_id (or 1 for the deny controller)
  ASSERT_EQ(mapping["A"].size(), 2);
  ASSERT_EQ(mapping["B"].size(), 1);
  auto admControllerForB = *mapping["B"].begin();
  DummyRequest requestB;
  DummyConnContext connContextB;
  ASSERT_FALSE(admControllerForB->admit());
}

TEST_F(AdmissionControllerSelectorTest, whiteListAdmission) {
  std::unordered_set<std::string> whitelist{"getStatus"};
  WhitelistAdmissionStrategy<GlobalAdmissionStrategy> selector(
      whitelist, std::make_shared<DummyController>());

  DummyRequest request;
  DummyConnContext connContext;
  auto admissionController =
      selector.select("myThriftMethod", request, connContext);
  ASSERT_NE(dynamic_cast<DummyController*>(admissionController.get()), nullptr);

  DummyConnContext connContext2;
  auto admissionController2 =
      selector.select("getStatus", request, connContext2);
  ASSERT_NE(
      dynamic_cast<AcceptAllAdmissionController*>(admissionController2.get()),
      nullptr);
}

} // namespace thrift
} // namespace apache
