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

#include <thrift/conformance/data/CompatibilityGenerator.h>

#include <cstdio>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <boost/mp11.hpp>
#include <folly/init/Init.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/conformance/cpp2/Object.h>
#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/data/ValueGenerator.h>
#include <thrift/conformance/data/internal/TestGenerator.h>
#include <thrift/conformance/if/gen-cpp2/test_suite_types.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/type/Name.h>
#include <thrift/test/testset/Testset.h>

using apache::thrift::test::testset::detail::mod_set;
using apache::thrift::test::testset::detail::struct_ByFieldType;
namespace mp11 = boost::mp11;

namespace apache::thrift::conformance::data {

namespace {

std::unique_ptr<folly::IOBuf> serialize(
    const Object& a, const Protocol& protocol) {
  switch (auto p = protocol.standard()) {
    case StandardProtocol::Compact:
      return serializeObject<apache::thrift::CompactProtocolWriter>(a);
    case StandardProtocol::Binary:
      return serializeObject<apache::thrift::BinaryProtocolWriter>(a);
    default:
      throw std::invalid_argument(
          "Unsupported protocol: " + util::enumNameSafe(p));
  }
}

template <class TT>
TestCase addFieldTestCase(const Protocol& protocol) {
  const typename struct_ByFieldType<TT, mod_set<>>::type def;

  RoundTripTestCase roundTrip;
  roundTrip.request()->value() = AnyRegistry::generated().store(def, protocol);
  roundTrip.request()->value()->data() = *serialize({}, protocol);
  roundTrip.expectedResponse().emplace().value() =
      AnyRegistry::generated().store(def, protocol);

  TestCase testCase;
  testCase.name() = fmt::format("testset.{}/AddField", type::getName<TT>());
  testCase.test()->roundTrip_ref() = std::move(roundTrip);
  return testCase;
}

template <typename TT>
Test createCompatibilityTest(const Protocol& protocol) {
  Test test;
  test.name() = protocol.name();
  test.testCases()->push_back(addFieldTestCase<TT>(protocol));
  return test;
}
} // namespace

using PrimaryTypeTags = mp11::mp_list<
    type::bool_t,
    type::byte_t,
    type::i16_t,
    type::i32_t,
    type::float_t,
    type::double_t,
    type::string_t,
    type::binary_t>;

TestSuite createCompatibilitySuite() {
  TestSuite suite;
  suite.name() = "CompatibilityTest";
  for (const auto& protocol : detail::toProtocols(detail::kDefaultProtocols)) {
    mp11::mp_for_each<PrimaryTypeTags>([&](auto t) {
      suite.tests()->emplace_back(
          createCompatibilityTest<decltype(t)>(protocol));
    });
  }
  return suite;
}

} // namespace apache::thrift::conformance::data
