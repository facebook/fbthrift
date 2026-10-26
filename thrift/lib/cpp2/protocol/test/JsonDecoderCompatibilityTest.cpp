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

// Tests JSON decoder backward compatibility for parsing various input formats.
// The JSON5 decoder should accept multiple representations for the same value
// to maintain compatibility with different serialization outputs.

#include <thrift/lib/cpp2/protocol/Json5Protocol.h>

#include <gtest/gtest.h>
#include <thrift/test/json5/gen-cpp2/json5_compatibility_test_constants.h>
#include <thrift/test/json5/gen-cpp2/json5_compatibility_test_types.h>
#include <thrift/test/json5/gen-cpp2/json5_test_types.h>
#include <thrift/test/json5/gen-cpp2/json5_test_types_custom_protocol.h>

namespace apache::thrift {

using facebook::thrift::json5::CompatibilityTestCase;
using facebook::thrift::json5::Example;
using facebook::thrift::json5::RoundTripTestCase;
using namespace facebook::thrift::json5::json5_compatibility_test_constants;

class JsonDecoderCompatibilityTest
    : public ::testing::TestWithParam<CompatibilityTestCase> {};

TEST_P(JsonDecoderCompatibilityTest, ParsesCorrectly) {
  for (const auto& json : *GetParam().inputs()) {
    auto result = Json5ProtocolUtils::fromJson5<Example>(json);
    EXPECT_EQ(result, *GetParam().output()) << json;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Compatibility,
    JsonDecoderCompatibilityTest,
    ::testing::ValuesIn(compatibilityTestCases()),
    [](const auto& info) { return *info.param.name(); });

class JsonDecoderRoundTripTest
    : public ::testing::TestWithParam<RoundTripTestCase> {};

TEST_P(JsonDecoderRoundTripTest, WritesDecodedValue) {
  for (const auto& json : *GetParam().inputs()) {
    auto result = Json5ProtocolUtils::fromJson5<Example>(json);
    EXPECT_EQ(Json5ProtocolUtils::toBasicJson(result), *GetParam().json())
        << json;
    EXPECT_EQ(Json5ProtocolUtils::toJson5(result), *GetParam().json5()) << json;
  }
}

INSTANTIATE_TEST_SUITE_P(
    RoundTrip,
    JsonDecoderRoundTripTest,
    ::testing::ValuesIn(roundTripTestCases()),
    [](const auto& info) { return *info.param.name(); });

} // namespace apache::thrift
