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

#include <thrift/lib/cpp2/transport/core/FunctionInfo.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/core/testutil/ThriftProcessorTestFixture.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache {
namespace thrift {

TEST_F(ThriftProcessorTestFixture, SendAndReceiveSumTwoNumbers) {
  int32_t x = 5;
  int32_t y = 10;
  int32_t expected_result = x + y;

  // Set up Async Processor
  MyServiceImpl service;
  std::unique_ptr<apache::thrift::AsyncProcessor> cpp2Processor =
      service.getProcessor();

  // Set up Thread Manager
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager(
      PriorityThreadManager::newPriorityThreadManager(
          32 /*threads*/, true /*stats*/, 1000 /*maxQueueLen*/));
  threadManager->start();

  // Set up processor.
  ThriftProcessor processor(std::move(cpp2Processor), threadManager.get());

  // Schedule the calls to the processor in the event base so that the
  // event loop is running for the entirety of the test.

  auto request = serializeSumTwoNumbers(x, y);

  eventBase_.runInEventBaseThread([&]() mutable {
    auto headers = std::make_unique<std::map<std::string, std::string>>();
    auto channel = std::shared_ptr<ThriftChannelIf>(fakeChannel_);
    auto finfo = std::make_unique<FunctionInfo>();
    finfo->kind = SINGLE_REQUEST_SINGLE_RESPONSE;
    finfo->seqId = 0;
    // "name" and "protocol" fields of "finfo" are not used right now.
    processor.onThriftRequest(
        std::move(finfo), std::move(headers), request.move(), channel);
  });

  // Start the event loop before calling into the channel and leave it
  // running for the entirety of the test.  The loop exits after
  // FakeChannel::sendThriftResponse() is called.
  eventBase_.loop();

  // The RPC has completed.
  threadManager->join();

  // Receive Response and compare result
  auto result = deserializeSumTwoNumbers(fakeChannel_->getPayloadBuf());
  EXPECT_EQ(result, expected_result);
}

} // namespace thrift
} // namespace apache
