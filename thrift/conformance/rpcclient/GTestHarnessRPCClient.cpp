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

#include <thrift/conformance/rpcclient/GTestHarnessRPCClient.h>

#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <folly/Subprocess.h>
#include <folly/futures/Future.h>
#include <thrift/conformance/Utils.h>
#include <thrift/conformance/if/gen-cpp2/ConformanceService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

namespace apache::thrift::conformance {

class ConformanceVerificationServer
    : public apache::thrift::ServiceHandler<ConformanceService> {
 public:
  explicit ConformanceVerificationServer(const TestCase& testCase)
      : testCase_(testCase) {}

  void getTestCase(TestCase& testCase) override {
    testCase = testCase_;
    getTestReceivedPromise_.setValue();
  }

  void sendTestResult(std::unique_ptr<ClientTestResult> result) override {
    clientResultPromise_.setValue(*result);
  }

  void requestResponse(Response& res, std::unique_ptr<Request>) override {
    res = *can_throw(testCase_.requestResponse_ref()->response());
  }

  folly::SemiFuture<folly::Unit> getTestReceived() {
    return getTestReceivedPromise_.getSemiFuture();
  }

  folly::SemiFuture<ClientTestResult> clientResult() {
    return clientResultPromise_.getSemiFuture();
  }

 private:
  const TestCase& testCase_;
  folly::Promise<folly::Unit> getTestReceivedPromise_;
  folly::Promise<ClientTestResult> clientResultPromise_;
};

testing::AssertionResult performRequestResponseAssertions(
    const TestCase& testCase, ClientTestResult result) {
  if (testCase.requestResponse_ref()->response() !=
      result.requestResponse_ref()->response()) {
    return testing::AssertionFailure();
  }
  return testing::AssertionSuccess();
}

testing::AssertionResult performAssertions(
    const TestCase& testCase, ClientTestResult result) {
  switch (testCase.test()->getType()) {
    case TestCaseUnion::Type::requestResponse:
      return performRequestResponseAssertions(testCase, result);
      break;
    default:
      return testing::AssertionFailure()
          << "Unsupported test case type: " << testCase.test()->getType();
  }
}

class RPCClientConformanceTest : public testing::Test {
 public:
  RPCClientConformanceTest(
      std::string_view clientCmd, const TestCase* testCase, bool conforming)
      : testCase_(*testCase),
        conforming_(conforming),
        handler_(std::make_shared<ConformanceVerificationServer>(testCase_)),
        server_(handler_) {
    clientProcess_ = folly::Subprocess(std::vector<std::string>{
        std::string(clientCmd),
        "--port",
        folly::to<std::string>(server_.getPort())});
  }

 protected:
  void TestBody() override {
    // Wait for client to fetch test case
    bool getTestReceived =
        handler_->getTestReceived().wait(std::chrono::seconds(1));
    EXPECT_EQ(conforming_, getTestReceived);

    // End test if client was unable to fetch test case
    if (!getTestReceived) {
      return;
    }

    // Wait for result from client
    folly::Try<ClientTestResult> result =
        handler_->clientResult().within(std::chrono::seconds(1)).getTry();

    // End test if result was not received
    if (result.hasException()) {
      EXPECT_FALSE(conforming_);
      return;
    }

    EXPECT_EQ(conforming_, performAssertions(testCase_, *result));
  }

  void TearDown() override {
    clientProcess_.sendSignal(SIGINT);
    clientProcess_.waitOrTerminateOrKill(
        std::chrono::seconds(1), std::chrono::seconds(1));
  }

 private:
  const TestCase& testCase_;
  bool conforming_;
  std::shared_ptr<ConformanceVerificationServer> handler_;
  apache::thrift::ScopedServerInterfaceThread server_;
  folly::Subprocess clientProcess_;
};

void RegisterTests(
    std::string_view category,
    const TestSuite* suite,
    const std::set<std::string>& nonconforming,
    std::string_view clientCmd,
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
          [&testCase, clientCmd, conforming]() {
            return new RPCClientConformanceTest(
                clientCmd, &testCase, conforming);
          });
    }
  }
}

} // namespace apache::thrift::conformance
