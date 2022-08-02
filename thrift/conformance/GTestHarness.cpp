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

testing::AssertionResult RunRequestResponseTest(
    ConformanceServiceAsyncClient& client,
    const RequestResponseTestCase& requestResponse) {
  if (!requestResponse.request() || !requestResponse.response()) {
    throw std::invalid_argument("Unimplemented");
  }

  // Test request-response
  try {
    Response res;
    client.sync_requestResponse(res, *requestResponse.request());
    if (res != requestResponse.response()) {
      return testing::AssertionFailure();
    }
  } catch (const apache::thrift::TApplicationException&) {
    return testing::AssertionFailure();
  }

  // Get result from server
  ServerTestResult result;
  client.sync_getTestResult(result);

  if (result.requestResponse_ref()->request() != requestResponse.request()) {
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
    case TestCaseUnion::Type::requestResponse:
      return RunRequestResponseTest(client, *testCase.requestResponse_ref());
    default:
      return testing::AssertionFailure()
          << "Unsupported test case type: " << testCase.test()->getType();
  }
}

class ConformanceTest : public testing::Test {
 public:
  ConformanceTest(
      ConformanceServiceAsyncClient* client,
      const TestCase* testCase,
      bool conforming)
      : client_(client), testCase_(*testCase), conforming_(conforming) {}

 protected:
  void TestBody() override {
    EXPECT_EQ(RunTestCase(*client_, testCase_), conforming_);
  }

 private:
  ConformanceServiceAsyncClient* const client_;
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
          [&testCase, clientFn, conforming]() {
            return new ConformanceTest(&clientFn(), &testCase, conforming);
          });
    }
  }
}

} // namespace apache::thrift::conformance
