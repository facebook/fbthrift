/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;

TestStruct makeTestStruct() {
  TestStruct s;
  s.s = "test";
  s.i = 48;
  return s;
}

TEST(SerializationTest, CompactSerializerRoundtripPasses) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  TestStruct out;
  CompactSerializer::deserialize(q.front(), out);

  EXPECT_EQ(out, s);
}

TEST(SerializationTest, BinarySerializerRoundtripPasses) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  BinarySerializer::serialize(s, &q);

  TestStruct out;
  BinarySerializer::deserialize(q.front(), out);

  EXPECT_EQ(out, s);
}

TEST(SerializationTest, MixedRoundtripFails) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  try {
    TestStruct out;
    BinarySerializer::deserialize(q.front(), out);
    FAIL();
  } catch (...) {
    // Should underflow
  }
}
