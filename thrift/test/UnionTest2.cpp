/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/test/gen-cpp2/UnionTest2_types.h"

#include <gtest/gtest.h>

using namespace thrift::test::debug::cpp2;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace testing;

class UnionTestFixture: public Test {
 public:
  void serializeDeserialize(TestUnion &val) {
    BinaryProtocolWriter prot;
    size_t bufSize = val.serializedSize(&prot);
    IOBufQueue queue(IOBufQueue::cacheChainLength());

    prot.setOutput(&queue, bufSize);
    val.write(&prot);

    bufSize = queue.chainLength();
    auto buf = queue.move();

    TestUnion out;
    BinaryProtocolReader protReader;
    protReader.setInput(buf.get());
    out.read(&protReader);
    EXPECT_EQ(val, out);
  }
};

TEST_F(UnionTestFixture, Constructors) {
  auto f = [] (const TestUnion& u) {
    EXPECT_EQ(TestUnion::Type::i32_field, u.getType());
    EXPECT_EQ(100, u.get_i32_field());
  };

  TestUnion u;
  u.set_i32_field(100);
  f(u);

  auto v1(u);
  f(v1);

  auto v2 = u;
  f(v2);

  auto v3(std::move(u));
  f(v3);

  auto v4 = std::move(v2);
  f(v4);
}

TEST_F(UnionTestFixture, ChangeType) {
  TestUnion u;
  u.set_i32_field(100);
  EXPECT_EQ(TestUnion::Type::i32_field, u.getType());
  EXPECT_EQ(100, u.get_i32_field());

  u.set_other_i32_field(200);
  EXPECT_EQ(TestUnion::Type::other_i32_field, u.getType());
  EXPECT_EQ(200, u.get_other_i32_field());

  u.set_string_field("str");
  EXPECT_EQ(TestUnion::Type::string_field, u.getType());
  EXPECT_EQ("str", u.get_string_field());

  u.set_struct_list(std::vector<RandomStuff>());
  EXPECT_EQ(TestUnion::Type::struct_list, u.getType());
  EXPECT_EQ(std::vector<RandomStuff>(), u.get_struct_list());
}

TEST_F(UnionTestFixture, SerdeTest) {
  TestUnion u;
  u.set_i32_field(100);
  serializeDeserialize(u);

  u.set_other_i32_field(200);
  serializeDeserialize(u);

  u.set_string_field("str");
  serializeDeserialize(u);

  u.set_struct_list(std::vector<RandomStuff>());
  serializeDeserialize(u);
}

