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

#include <thrift/conformance/GTestHarness.h>

#include <stdexcept>

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace apache::thrift::conformance {

testing::AssertionResult RunRoundTripTest(
    ConformanceServiceAsyncClient& client, const RoundTripTestCase& roundTrip) {
  RoundTripResponse res;
  try {
    client.sync_roundTrip(res, *roundTrip.request());
  } catch (const apache::thrift::TApplicationException&) {
    return testing::AssertionFailure();
  }

  const Any& expectedAny = roundTrip.expectedResponse()
      ? *roundTrip.expectedResponse().value_unchecked().value()
      : *roundTrip.request()->value();

  auto parseAny = [](const Any& a) {
    switch (auto protocol = a.protocol().value_or(StandardProtocol::Compact)) {
      case StandardProtocol::Compact:
        return parseObject<apache::thrift::CompactProtocolReader>(*a.data());
      case StandardProtocol::Binary:
        return parseObject<apache::thrift::BinaryProtocolReader>(*a.data());
      default:
        throw std::invalid_argument(
            "Unsupported protocol: " + util::enumNameSafe(protocol));
    }
  };

  Object actual = parseAny(*res.value());
  Object expected = parseAny(expectedAny);
  if (!op::identical<type::struct_t<Object>>(actual, expected)) {
    // TODO(afuller): Report out the delta
    return testing::AssertionFailure();
  }
  return testing::AssertionSuccess();
}

RequestResponseBasicClientTestResult RunRequestResponseBasic(
    RPCConformanceServiceAsyncClient& client,
    const RequestResponseBasicClientInstruction& instruction) {
  RequestResponseBasicClientTestResult result;
  client.sync_requestResponseBasic(
      result.response().emplace(), *instruction.request());
  return result;
}

ClientTestResult RunClientSteps(
    RPCConformanceServiceAsyncClient& client,
    const ClientInstruction& clientInstruction) {
  ClientTestResult result;
  switch (clientInstruction.getType()) {
    case ClientInstruction::Type::requestResponseBasic:
      result.set_requestResponseBasic(RunRequestResponseBasic(
          client, *clientInstruction.requestResponseBasic_ref()));
      break;
    default:
      break;
  }
  return result;
}

testing::AssertionResult RunRpcTest(
    RPCConformanceServiceAsyncClient& client, const RpcTestCase& rpc) {
  client.sync_sendTestCase(rpc);
  auto actualClientResult = RunClientSteps(client, *rpc.clientInstruction());
  if (actualClientResult != *rpc.clientTestResult()) {
    return testing::AssertionFailure();
  }

  // Get result from server
  ServerTestResult actualServerResult;
  client.sync_getTestResult(actualServerResult);
  if (actualServerResult != *rpc.serverTestResult()) {
    return testing::AssertionFailure();
  }
  return testing::AssertionSuccess();
}

} // namespace apache::thrift::conformance
