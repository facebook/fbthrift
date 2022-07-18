/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glog/logging.h>
#include <folly/init/Init.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBaseManager.h>

#include <thrift/conformance/if/gen-cpp2/ConformanceService.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

DEFINE_int32(port, 7777, "Port for Conformance Verification Server");

using namespace apache::thrift;
using namespace apache::thrift::conformance;

std::unique_ptr<Client<ConformanceService>> createClient() {
  return std::make_unique<Client<ConformanceService>>(
      RocketClientChannel::newChannel(
          folly::AsyncTransport::UniquePtr(new folly::AsyncSocket(
              folly::EventBaseManager::get()->getEventBase(),
              folly::SocketAddress("::1", FLAGS_port)))));
}

RequestResponseClientTestResult runRequestResponseTest(
    const RequestResponseTestDescription& test) {
  RequestResponseClientTestResult result;
  auto client = createClient();
  client->sync_requestResponse(
      result.response().emplace(), *can_throw(test.request()));
  return result;
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  TestCase testCase;
  auto client = createClient();
  client->sync_getTestCase(testCase);

  ClientTestResult result;
  switch (testCase.test()->getType()) {
    case TestCaseUnion::Type::requestResponse:
      result.set_requestResponse(
          runRequestResponseTest(*testCase.requestResponse_ref()));
      break;
    default:
      throw std::runtime_error("Invalid TestCase type");
  }

  client->sync_sendTestResult(result);
  return 0;
}
