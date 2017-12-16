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

#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

namespace apache {
namespace thrift {

class RSResponderTestFixture : public testing::Test {
 public:
  RSResponderTestFixture() {
    threadManager_ = PriorityThreadManager::newPriorityThreadManager(
        32 /*threads*/, true /*stats*/, 1000 /*maxQueueLen*/);
    threadManager_->start();

    auto cpp2Processor = service_.getProcessor();
    processor_ = std::make_unique<ThriftProcessor>(
        std::move(cpp2Processor), serverConfigs_);
    processor_->setThreadManager(threadManager_.get());

    responder_ =
        std::make_unique<RSResponder>(processor_.get(), &eventBase_, nullptr);
  }

  // Tears down after the test.
  ~RSResponderTestFixture() override = default;

 protected:
  apache::thrift::server::ServerConfigsMock serverConfigs_;
  testing::StrictMock<testutil::testservice::TestServiceMock> service_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  std::unique_ptr<ThriftProcessor> processor_;
  std::unique_ptr<RSResponder> responder_;
  folly::EventBase eventBase_;
};
} // namespace thrift
} // namespace apache
