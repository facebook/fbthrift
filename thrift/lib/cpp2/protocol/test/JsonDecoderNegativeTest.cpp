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

// Tests JSON decoder negative cases for rejecting invalid input formats.
// The JSON5 decoder should reject malformed or type-mismatched values
// to ensure strict validation of incoming data.

#include <thrift/lib/cpp2/protocol/Json5Protocol.h>

#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include <folly/lang/Pretty.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_negative_test_constants.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_negative_test_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_test_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_test_types_custom_protocol.h>

namespace apache::thrift {

using facebook::thrift::json5::Example;
using facebook::thrift::json5::NegativeTestCase;
using namespace facebook::thrift::json5::json5_negative_test_constants;

class JsonDecoderNegativeTest
    : public ::testing::TestWithParam<NegativeTestCase> {};

TEST_P(JsonDecoderNegativeTest, RejectsInvalidInput) {
  try {
    (void)Json5ProtocolUtils::fromJson5<Example>(*GetParam().json());
    ADD_FAILURE() << "expected the reader to reject the input";
  } catch (const protocol::TProtocolException& e) {
    EXPECT_EQ(e.getType(), protocol::TProtocolException::INVALID_DATA)
        << e.what();
  }
}

// Like Binary/Compact, malformed input is reported as TProtocolException
// INVALID_DATA, whichever layer detects it.
TEST(JsonDecoderErrorTypeTest, ReportsInvalidDataAsTProtocolException) {
  for (std::string_view json : {
           R"({"i64Value": 1)", // syntax error in the lexer
           R"({"i64Value": 99999999999999999999})", // integer literal overflow
           R"({"i32Value": 3000000000})", // out of range for the field type
           R"({"enumValue": "NOT_AN_ENUM"})", // rejected by the protocol reader
           R"({"binaryValue": {"base64": "!!!!"}})", // invalid base64
           R"({"i64Value": 1}])", // trailing content after the value
       }) {
    try {
      (void)Json5ProtocolUtils::fromJson5<Example>(json);
      ADD_FAILURE() << "expected rejection: " << json;
    } catch (const protocol::TProtocolException& e) {
      EXPECT_EQ(e.getType(), protocol::TProtocolException::INVALID_DATA)
          << json;
    }
  }
}

TEST(JsonDecoderErrorTypeTest, TypeMismatchNamesTheFoundType) {
  try {
    (void)Json5ProtocolUtils::fromJson5<Example>(R"({"boolValue": 1})");
    ADD_FAILURE() << "expected rejection";
  } catch (const protocol::TProtocolException& e) {
    auto expected = std::string("cannot parse `") +
        folly::pretty_name<std::int64_t>() + "` as `bool`";
    EXPECT_NE(std::string_view(e.what()).find(expected), std::string_view::npos)
        << e.what();
  }
}

INSTANTIATE_TEST_SUITE_P(
    EnumValidation,
    JsonDecoderNegativeTest,
    ::testing::ValuesIn(enumValidationNegativeCases()),
    [](const auto& info) { return std::string(*info.param.name()); });

INSTANTIATE_TEST_SUITE_P(
    TypeValidation,
    JsonDecoderNegativeTest,
    ::testing::ValuesIn(typeValidationNegativeCases()),
    [](const auto& info) { return std::string(*info.param.name()); });

INSTANTIATE_TEST_SUITE_P(
    TypeMismatch,
    JsonDecoderNegativeTest,
    ::testing::ValuesIn(typeMismatchNegativeCases()),
    [](const auto& info) { return std::string(*info.param.name()); });

INSTANTIATE_TEST_SUITE_P(
    FormatValidation,
    JsonDecoderNegativeTest,
    ::testing::ValuesIn(formatValidationNegativeCases()),
    [](const auto& info) { return std::string(*info.param.name()); });

INSTANTIATE_TEST_SUITE_P(
    OverflowValidation,
    JsonDecoderNegativeTest,
    ::testing::ValuesIn(overflowValidationNegativeCases()),
    [](const auto& info) { return std::string(*info.param.name()); });

} // namespace apache::thrift
