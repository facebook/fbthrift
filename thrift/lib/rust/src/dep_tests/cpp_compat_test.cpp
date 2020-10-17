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

#include <common/gtest/gtest_extensions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "thrift/lib/rust/src/dep_tests/gen-cpp2/test_thrift_types.h"

namespace facebook {
namespace azw {

TEST(JsonTest, structKey) {
  azw::Small s;
  azw::SubStruct stru;
  std::map<azw::Small, int32_t> m;
  m.insert({s, 1});
  stru.key_map_ref() = m;

  SubStruct outStruct;

  auto t = apache::thrift::SimpleJSONSerializer::serialize<std::string>(stru);
  ASSERT_EQ(
      t,
      "{\"req_def\":\"IAMREQ\",\"key_map\":{{\"num\":0,\"two\":0}:1},\"bin\":\"\"}");

  apache::thrift::SimpleJSONSerializer::deserialize(t, outStruct);
  ASSERT_EQ(stru, outStruct);
}

TEST(JsonTest, weirdText) {
  azw::SubStruct stru;
  stru.opt_def_ref() = "stuff\twith\nescape\\characters'...\"lots{of}fun</xml>";
  stru.bin_ref() = "1234";

  SubStruct outStruct;

  auto t = apache::thrift::SimpleJSONSerializer::serialize<std::string>(stru);
  ASSERT_EQ(
      t,
      "{\"opt_def\":\"stuff\\twith\\nescape\\\\characters'...\\\"lots{of}fun</xml>\","
      "\"req_def\":\"IAMREQ\",\"bin\":\"MTIzNA\"}");

  apache::thrift::SimpleJSONSerializer::deserialize(t, outStruct);
  ASSERT_EQ(stru, outStruct);

  stru.opt_def_ref() = "UNICODE\U0001F60AUH OH";
  t = apache::thrift::SimpleJSONSerializer::serialize<std::string>(stru);
  ASSERT_EQ(
      t,
      "{\"opt_def\":\"UNICODE\xf0\x9f\x98\x8aUH OH\","
      "\"req_def\":\"IAMREQ\",\"bin\":\"MTIzNA\"}");

  apache::thrift::SimpleJSONSerializer::deserialize(t, outStruct);
  ASSERT_EQ(stru, outStruct);
}

TEST(JsonTest, skipComplex) {
  azw::SubStruct stru;
  stru.opt_def_ref() = "thing";
  stru.bin_ref() = "1234";

  SubStruct outStruct;

  std::string input(
      "{\"opt_def\":\"thing\","
      "\"req_def\":\"IAMREQ\",\"bin\":\"MTIzNA\","
      "\"extra\":[1, {\"thing\":\"thing2\"}],"
      "\"extra_map\":{\"thing\":null,\"thing2\":2}}");

  apache::thrift::SimpleJSONSerializer::deserialize(input, outStruct);
  ASSERT_EQ(stru, outStruct);
}

TEST(JsonTest, nullStuff) {
  azw::SubStruct stru;
  stru.bin_ref() = "1234";

  SubStruct outStruct;

  std::string input(
      "{\"opt_def\":null,"
      "\"req_def\":\"IAMREQ\",\"bin\":\"MTIzNA\"}");

  apache::thrift::SimpleJSONSerializer::deserialize(input, outStruct);
  ASSERT_EQ(stru, outStruct);
}

} // namespace azw
} // namespace facebook
