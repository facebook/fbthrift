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

#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <folly/lang/Exception.h>
#include <thrift/conformance/Utils.h>
#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/conformance/cpp2/Object.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace apache::thrift::conformance {
namespace {

testing::AssertionResult RunRoundTripTest(
    ConformanceServiceAsyncClient& client, RoundTripTestCase roundTrip) {
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
    ConformanceServiceAsyncClient& client,
    const RequestResponseBasicClientInstruction& instruction) {
  RequestResponseBasicClientTestResult result;
  client.sync_requestResponseBasic(
      result.response().emplace(), *instruction.request());
  return result;
}

ClientTestResult RunClientSteps(
    ConformanceServiceAsyncClient& client,
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
    ConformanceServiceAsyncClient& client, const RpcTestCase& rpc) {
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

} // namespace

testing::AssertionResult RunTestCase(
    ConformanceServiceAsyncClient& client, const TestCase& testCase) {
  client.sync_sendTestCase(testCase);
  switch (testCase.test()->getType()) {
    case TestCaseUnion::Type::roundTrip:
      return RunRoundTripTest(client, *testCase.roundTrip_ref());
    case TestCaseUnion::Type::rpc:
      return RunRpcTest(client, *testCase.rpc_ref());
    default:
      return testing::AssertionFailure()
          << "Unsupported test case type: " << testCase.test()->getType();
  }
}

class ConformanceTest : public testing::Test {
 public:
  ConformanceTest(
      ConformanceServiceAsyncClient* client,
      const TestSuite* suite,
      const conformance::Test* test,
      const TestCase* testCase,
      bool conforming)
      : client_(client),
        suite_(*suite),
        test_(*test),
        testCase_(*testCase),
        conforming_(conforming) {}

 protected:
  void TestBody() override {
    testing::AssertionResult conforming = RunTestCase(*client_, testCase_);
    if (conforming_) {
      EXPECT_TRUE(conforming) << "For more detail see:"
                              << std::endl
                              // Most specific to least specific.
                              << genTagLinks(testCase_) << genTagLinks(test_)
                              << genTagLinks(suite_);
      ;
    } else {
      EXPECT_FALSE(conforming)
          << "If intentional, please remove the associated entry from:"
          << std::endl
          << "    thrift/conformance/data/nonconforming.txt" << std::endl;
    }
  }

 private:
  ConformanceServiceAsyncClient* const client_;

  const TestSuite& suite_;
  const conformance::Test& test_;
  const TestCase& testCase_;
  const bool conforming_;
};

void RegisterTests(
    std::string_view category,
    const TestSuite* suite,
    const std::set<std::string>& nonconforming,
    std::function<ConformanceServiceAsyncClient&()> clientFn,
    const char* file,
    int line) {
  for (const auto& test : *suite->tests()) {
    for (const auto& testCase : *test.testCases()) {
      std::string suiteName =
          fmt::format("{}/{}/{}", category, *suite->name(), *testCase.name());
      std::string fullName = fmt::format("{}.{}", suiteName, *test.name());
      bool conforming = nonconforming.find(fullName) == nonconforming.end();
      RegisterTest(
          suiteName.c_str(),
          test.name()->c_str(),
          nullptr,
          conforming ? nullptr : "nonconforming",
          file,
          line,
          [&test, &testCase, suite, clientFn, conforming]() {
            return new ConformanceTest(
                &clientFn(), suite, &test, &testCase, conforming);
          });
    }
  }
}

} // namespace apache::thrift::conformance
