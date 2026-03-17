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

#include <thrift/lib/cpp2/protocol/Json5Protocol.h>

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_test_constants.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_test_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/json5_test_types_custom_protocol.h>

namespace apache::thrift {

using facebook::thrift::json5::Example;
using facebook::thrift::json5::TestCase;
using namespace facebook::thrift::json5::json5_test_constants;

class Json5DecoderTest : public testing::TestWithParam<TestCase> {};

TEST_P(Json5DecoderTest, Decode) {
  for (const auto& json : {*GetParam().json(), *GetParam().json5()}) {
    auto out = Json5ProtocolUtils::fromJson5<Example>(json);
    EXPECT_EQ(out, *GetParam().example()) << json;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Decode,
    Json5DecoderTest,
    testing::ValuesIn(testCases()),
    [](const auto& info) { return *info.param.name(); });

} // namespace apache::thrift
