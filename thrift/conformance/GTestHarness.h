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

#pragma once

#include <functional>

#include <folly/Range.h>
#include <folly/String.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/ConformanceServiceAsyncClient.h>
#include <thrift/conformance/if/gen-cpp2/test_suite_types.h>

// Registers the given conformance test suites with gtest using the
// ConformanceServiceAsyncClient providers in clientFns.
#define THRIFT_CONFORMANCE_TEST(suites, clientFns, nonconforming)           \
  static ::apache::thrift::conformance::detail::ConformanceTestRegistration \
      __suite_reg_##__LINE__(                                               \
          suites, clientFns, nonconforming, __FILE__, __LINE__)

namespace apache::thrift::conformance {

// A map from name to ConformanceServiceAsyncClient provider.
using client_fn_map =
    std::map<std::string_view, std::function<ConformanceServiceAsyncClient&()>>;

// Names default to parent directory, or can be customized by appending
// '#<name>' to the command. If the command itself has a '#' character in it,
// appending an additional "#" will cause it to parse correctly.
std::pair<std::string_view, std::string_view> parseNameAndCmd(
    std::string_view entry);

// Parses commands (and optionally custom names) seporated by ','
std::map<std::string_view, std::string_view> parseCmds(
    std::string_view cmdsStr);

// Parses a set of non-conforming test names, seporated by '/n'
//
// Use # for comments.
std::set<std::string> parseNonconforming(std::string_view data);

// Runs a conformance test case against the given client.
testing::AssertionResult RunTestCase(
    ConformanceServiceAsyncClient& client, const TestCase& testCase);

// Registers a test suite with gtest.
void RegisterTests(
    std::string_view name,
    const TestSuite* suite,
    const std::set<std::string>& nonconforming,
    std::function<ConformanceServiceAsyncClient&()> clientFn,
    const char* file = "",
    int line = 0);

namespace detail {

class ConformanceTestRegistration {
 public:
  ConformanceTestRegistration(
      std::vector<TestSuite> suites,
      client_fn_map clientFns,
      const std::set<std::string>& nonconforming,
      const char* file = "",
      int line = 0)
      : suites_(std::move(suites)) {
    for (const auto& entry : clientFns) {
      for (const auto& suite : suites_) {
        RegisterTests(
            entry.first, &suite, nonconforming, entry.second, file, line);
      }
    }
  }

 private:
  const std::vector<TestSuite> suites_;
};

} // namespace detail
} // namespace apache::thrift::conformance
