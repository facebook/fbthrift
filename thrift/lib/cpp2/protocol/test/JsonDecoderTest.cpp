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

// Not assignable from std::string; filled through clear() and append(), like
// test::Buffer in thrift/lib/py3/test/BinaryTypes.h.
class AppendOnlyBuffer {
 public:
  void clear() { data_.clear(); }
  void append(const char* data, std::size_t size) { data_.append(data, size); }
  const std::string& str() const { return data_; }

 private:
  std::string data_;
};

TEST(Json5DecoderCustomTypeTest, CustomBinaryType) {
  using Tag = type::cpp_type<AppendOnlyBuffer, type::binary_t>;
  EXPECT_EQ(
      Json5ProtocolUtils::fromJson5<Tag>(R"({"utf-8": "hello"})").str(),
      "hello");
  EXPECT_EQ(Json5ProtocolUtils::fromJson5<Tag>(R"("aGVsbG8=")").str(), "hello");
}

// Binary in base64 form may still carry arbitrary bytes.
TEST(Json5DecoderUtf8Test, RejectsInvalidUtf8) {
  auto decode = [](std::string_view json) {
    return Json5ProtocolUtils::fromJson5<Example>(json);
  };
  using protocol::TProtocolException;
  EXPECT_THROW(
      decode("{\"stringValue\": \"\x80\x81\x82\"}"), TProtocolException);
  EXPECT_THROW(
      decode("{\"binaryValue\": {\"utf-8\": \"\xC3\"}}"), TProtocolException);
  EXPECT_THROW(
      decode("{\"stringAsKey\": {\"\xE0\x80\x80\": \"x\"}}"),
      TProtocolException);

  EXPECT_EQ(
      decode("{\"binaryValue\": {\"base64url\": \"gIGC\"}}").binaryValue(),
      "\x80\x81\x82");
}

} // namespace apache::thrift
