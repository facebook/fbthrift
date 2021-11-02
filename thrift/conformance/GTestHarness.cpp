/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/conformance/cpp2/Object.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/type/ThriftOp.h>

namespace apache::thrift::conformance {
namespace {

// From a newer version of gtest.
//
// TODO(afuller): Delete once gtest is updated.
template <typename Factory>
testing::TestInfo* RegisterTest(
    const char* test_suite_name,
    const char* test_name,
    const char* type_param,
    const char* value_param,
    const char* file,
    int line,
    Factory factory) {
  using TestT = typename std::remove_pointer<decltype(factory())>::type;

  class FactoryImpl : public testing::internal::TestFactoryBase {
   public:
    explicit FactoryImpl(Factory f) : factory_(std::move(f)) {}
    testing::Test* CreateTest() override { return factory_(); }

   private:
    Factory factory_;
  };

  return testing::internal::MakeAndRegisterTestInfo(
      test_suite_name,
      test_name,
      type_param,
      value_param,
      testing::internal::CodeLocation(file, line),
      testing::internal::GetTypeId<TestT>(),
      TestT::SetUpTestCase,
      TestT::TearDownTestCase,
      new FactoryImpl{std::move(factory)});
}

testing::AssertionResult RunRoundTripTest(
    ConformanceServiceAsyncClient& client, RoundTripTestCase roundTrip) {
  RoundTripResponse res;
  client.sync_roundTrip(res, *roundTrip.request_ref());

  const Any& expectedAny = roundTrip.expectedResponse_ref()
      ? *roundTrip.expectedResponse_ref().value_unchecked().value_ref()
      : *roundTrip.request_ref()->value_ref();

  // TODO(afuller): Make add asValueStruct support to AnyRegistry and use that
  // instead of hard coding type.
  auto actual = AnyRegistry::generated().load<Value>(*res.value_ref());
  auto expected = AnyRegistry::generated().load<Value>(expectedAny);
  if (!op::identical<type::struct_t<Value>>(actual, expected)) {
    // TODO(afuller): Report out the delta
    return testing::AssertionFailure();
  }
  return testing::AssertionSuccess();
}

} // namespace

std::pair<std::string_view, std::string_view> parseNameAndCmd(
    std::string_view entry) {
  // Look for a custom name.
  auto pos = entry.find_last_of("#/");
  if (pos != std::string_view::npos && entry[pos] == '#') {
    if (pos == entry.size() - 1) {
      // Just a trailing delim, remove it.
      entry = entry.substr(0, pos);
    } else {
      // Use the custom name.
      return {entry.substr(pos + 1), entry.substr(0, pos)};
    }
  }

  // No custom name, so use parent directory as name.
  size_t stop = entry.find_last_of("\\/") - 1;
  size_t start = entry.find_last_of("\\/", stop);
  return {entry.substr(start + 1, stop - start), entry};
}

std::map<std::string_view, std::string_view> parseCmds(
    std::string_view cmdsStr) {
  std::map<std::string_view, std::string_view> result;
  std::vector<folly::StringPiece> cmds;
  folly::split(',', cmdsStr, cmds);
  for (auto cmd : cmds) {
    auto entry = parseNameAndCmd(folly::trimWhitespace(cmd));
    auto res = result.emplace(entry);
    if (!res.second) {
      folly::throw_exception<std::invalid_argument>(fmt::format(
          "Multiple servers have the name {}: {} vs {}",
          entry.first,
          res.first->second,
          entry.second));
    }
  }
  return result;
}

std::set<std::string> parseNonconforming(std::string_view data) {
  std::vector<folly::StringPiece> lines;
  folly::split("\n", data, lines);
  std::set<std::string> result;
  for (auto& line : lines) {
    // Strip any comments.
    if (auto pos = line.find_first_of('#'); pos != folly::StringPiece::npos) {
      line = line.subpiece(0, pos);
    }
    // Add trimmed, non-empty lines.
    line = folly::trimWhitespace(line);
    if (!line.empty()) {
      result.emplace(line);
    }
  }
  return result;
}

testing::AssertionResult RunTestCase(
    ConformanceServiceAsyncClient& client, const TestCase& testCase) {
  switch (testCase.test_ref()->getType()) {
    case TestCaseUnion::roundTrip:
      return RunRoundTripTest(client, *testCase.roundTrip_ref());
    default:
      return testing::AssertionFailure()
          << "Unsupported test case type: " << testCase.test_ref()->getType();
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
  for (const auto& test : *suite->tests_ref()) {
    for (const auto& testCase : *test.testCases_ref()) {
      std::string suiteName = fmt::format(
          "{}/{}/{}", category, *suite->name_ref(), *testCase.name_ref());
      std::string fullName = fmt::format("{}.{}", suiteName, *test.name_ref());
      bool conforming = nonconforming.find(fullName) == nonconforming.end();
      RegisterTest(
          suiteName.c_str(),
          test.name_ref()->c_str(),
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
