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

#include <thrift/conformance/if/gen-cpp2/RPCConformanceService.h>
#include <thrift/conformance/if/gen-cpp2/rpc_types.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

DEFINE_int32(port, 7777, "Port for Conformance Verification Server");

using namespace apache::thrift;
using namespace apache::thrift::conformance;

std::unique_ptr<Client<RPCConformanceService>> createClient() {
  return std::make_unique<Client<RPCConformanceService>>(
      RocketClientChannel::newChannel(
          folly::AsyncTransport::UniquePtr(new folly::AsyncSocket(
              folly::EventBaseManager::get()->getEventBase(),
              folly::SocketAddress("::1", FLAGS_port)))));
}

RequestResponseBasicClientTestResult runRequestResponseBasicTest(
    const RequestResponseBasicClientInstruction& instruction) {
  RequestResponseBasicClientTestResult result;
  auto client = createClient();
  client->sync_requestResponseBasic(
      result.response().emplace(), *instruction.request());
  return result;
}

RequestResponseDeclaredExceptionClientTestResult
requestResponseDeclaredExceptionTest(
    const RequestResponseDeclaredExceptionClientInstruction& instruction) {
  RequestResponseDeclaredExceptionClientTestResult result;
  auto client = createClient();
  try {
    client->sync_requestResponseDeclaredException(*instruction.request());
  } catch (const UserException& e) {
    result.userException() = e;
  }
  return result;
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  RpcTestCase testCase;
  auto client = createClient();
  client->sync_getTestCase(testCase);
  auto& clientInstruction = *testCase.clientInstruction_ref();

  ClientTestResult result;
  switch (clientInstruction.getType()) {
    case ClientInstruction::Type::requestResponseBasic:
      result.set_requestResponseBasic(runRequestResponseBasicTest(
          *clientInstruction.requestResponseBasic_ref()));
      break;
    case ClientInstruction::Type::requestResponseDeclaredException:
      result.set_requestResponseDeclaredException(
          requestResponseDeclaredExceptionTest(
              *clientInstruction.requestResponseDeclaredException_ref()));
      break;
    default:
      throw std::runtime_error("Invalid TestCase type");
  }

  client->sync_sendTestResult(result);
  return 0;
}
