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

namespace apache {
namespace thrift {

using namespace testing;
using namespace testutil::testservice;

TEST_F(RSResponderTestFixture, RequestChannel) {
  auto request = TestServiceMock::serializeNoParameterRPCCall("helloChannel");
  auto names = {
      std::string("name0"), std::string("name1"), std::string("name2")};

  auto input = yarpl::flowable::Flowables::justN(names)->map(
      [this](const std::string& name) {
        return rsocket::Payload(service_.encodeString(name));
      });

  auto response = responder_->handleRequestChannel(
      rsocket::Payload(request.move()), input, 0);

  auto it = names.begin();
  response->subscribe(
      [&it, this](rsocket::Payload payload) mutable {
        EXPECT_STREQ(
            service_.decodeString(std::move(payload.data)).c_str(),
            ("Hello, " + *it).c_str());
        ++it;
      },
      [](folly::exception_wrapper) { FAIL() << "No error is expected"; });

  threadManager_->join();
}

TEST_F(RSResponderTestFixture, RequestChannel_MethodNotFound) {
  auto request = TestServiceMock::serializeNoParameterRPCCall("missing_method");
  auto names = {
      std::string("name0"), std::string("name1"), std::string("name2")};

  auto input = yarpl::flowable::Flowables::justN(names)->map(
      [this](const std::string& name) {
        return rsocket::Payload(service_.encodeString(name));
      });

  auto response = responder_->handleRequestChannel(
      rsocket::Payload(request.move()), input, 0);

  response->subscribe(
      [](rsocket::Payload) { FAIL() << "onError should have been called"; },
      [](folly::exception_wrapper ew) {
        LOG(INFO) << "Exception: " << ew.what();
        EXPECT_THAT(ew.what().c_str(), HasSubstr("missing_method"));
      });

  threadManager_->join();
}

TEST_F(RSResponderTestFixture, RequestChannel_InputThrows_ExpectedError) {
  auto request = TestServiceMock::serializeNoParameterRPCCall("helloChannel");
  auto names = {
      std::string("name0"), std::string("name1"), std::string("name2")};

  bool second = false;
  auto input =
      yarpl::flowable::Flowables::justN(names)->map([&second, this](auto name) {
        if (second) {
          TestServiceException exception;
          exception.message = "mock_service_method_exception";
          throw exception;
        }
        second = true;
        return rsocket::Payload(service_.encodeString(name));
      });

  auto response = responder_->handleRequestChannel(
      rsocket::Payload(request.move()), input, 0);

  auto it = names.begin();
  response->subscribe(
      [&it, this](rsocket::Payload payload) mutable {
        EXPECT_STREQ(
            service_.decodeString(std::move(payload.data)).c_str(),
            ("Hello, " + *it).c_str());
        ++it;
      },
      [](folly::exception_wrapper ew) {
        LOG(INFO) << "Exception: " << ew.what();
        EXPECT_THAT(
            ew.get_exception()->what(), HasSubstr("TestServiceException"));
      });

  threadManager_->join();
}

TEST_F(RSResponderTestFixture, RequestChannel_ResultThrows_UnexpectedError) {
  auto request = TestServiceMock::serializeNoParameterRPCCall("helloChannel");
  auto names = {
      std::string("name0"), std::string("name1"), std::string("name2")};

  bool second = false;
  auto input =
      yarpl::flowable::Flowables::justN(names)->map([&second, this](auto name) {
        if (second) {
          throw std::runtime_error("mock_runtime_error");
        }
        second = true;
        return rsocket::Payload(service_.encodeString(name));
      });

  auto response = responder_->handleRequestChannel(
      rsocket::Payload(request.move()), input, 0);

  auto it = names.begin();
  response->subscribe(
      [&it, this](rsocket::Payload payload) mutable {
        EXPECT_STREQ(
            service_.decodeString(std::move(payload.data)).c_str(),
            ("Hello, " + *it).c_str());
        ++it;
      },
      [](folly::exception_wrapper ew) {
        LOG(INFO) << "Exception: " << ew.what();
        EXPECT_THAT(ew.what().c_str(), HasSubstr("mock_runtime_error"));
      });

  threadManager_->join();
}
} // namespace thrift
} // namespace apache
