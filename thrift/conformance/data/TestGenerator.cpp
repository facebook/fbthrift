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

#include <thrift/conformance/data/TestGenerator.h>

#include <fmt/core.h>

#include <thrift/conformance/cpp2/Object.h>
#include <thrift/conformance/data/ValueGenerator.h>
#include <thrift/lib/cpp2/type/Name.h>
#include <thrift/test/testset/Testset.h>

namespace apache::thrift::conformance::data {

namespace {

template <typename TT>
Test createRoundTripTest(
    const AnyRegistry& registry, const Protocol& protocol) {
  using namespace apache::thrift::test::testset::detail;

  Test test;
  test.name_ref() = protocol.name();
  for (const auto& value : ValueGenerator<TT>::getInterestingValues()) {
    // Test case #1: Use ValueStruct
    RoundTripTestCase roundTrip;
    roundTrip.request_ref()->value_ref() =
        registry.store(asValueStruct<TT>(value.value), protocol);

    auto& testCase1 = test.testCases_ref()->emplace_back();
    testCase1.name_ref() =
        fmt::format("{}/{}", type::getName<TT>(), value.name);
    testCase1.test_ref()->set_roundTrip(roundTrip);

    // Test case #2: Use Testset
    typename struct_ByFieldType<TT, mod_set<>>::type data;
    data.field_1_ref() = value.value;
    roundTrip.request_ref()->value_ref() = registry.store(data, protocol);
    auto& testCase2 = test.testCases_ref()->emplace_back();
    testCase2.name_ref() =
        fmt::format("testset.{}/{}", type::getName<TT>(), value.name);
    testCase2.test_ref()->set_roundTrip(roundTrip);
  }

  return test;
}

} // namespace

void addRoundTripToSuite(
    const AnyRegistry& registry, const Protocol& protocol, TestSuite& suite) {
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::bool_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::byte_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::i16_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::i32_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::float_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::double_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::string_t>(registry, protocol));
  suite.tests_ref()->emplace_back(
      createRoundTripTest<type::binary_t>(registry, protocol));
}

TestSuite createRoundTripSuite(
    const std::set<Protocol>& protocols, const AnyRegistry& registry) {
  TestSuite suite;
  suite.name_ref() = "RoundTripTest";
  for (const auto& protocol : protocols) {
    addRoundTripToSuite(registry, protocol, suite);
  }
  return suite;
}

} // namespace apache::thrift::conformance::data
