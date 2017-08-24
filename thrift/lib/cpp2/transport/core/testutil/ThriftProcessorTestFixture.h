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

#pragma once

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/transport/core/testutil/FakeChannel.h>
#include <memory>
#include <thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.tcc>

namespace apache {
namespace thrift {

class MyServiceImpl : public cpp2::TestServiceSvIf {
  int32_t sumTwoNumbers(int32_t x, int32_t y) override {
    return x + y;
  }
};

class ThriftProcessorTestFixture : public testing::Test {
 public:
  // Sets up for a test.
  ThriftProcessorTestFixture() {
    fakeChannel_ = std::make_shared<FakeChannel>(&eventBase_);
  }

  // Tears down after the test.
  ~ThriftProcessorTestFixture() override = default;

  // Send the two integers to be serialized for 'sumTwoNumbers'
  folly::IOBufQueue serializeSumTwoNumbers(int32_t x, int32_t y) const;

  // Receive the deserialized integer that results from 'sumTwoNumbers'
  int32_t deserializeSumTwoNumbers(folly::IOBuf* buf) const;

 protected:
  folly::EventBase eventBase_;
  std::shared_ptr<FakeChannel> fakeChannel_;
};

} // namespace thrift
} // namespace apache
