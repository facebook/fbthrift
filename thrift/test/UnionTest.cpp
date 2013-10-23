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

#include "thrift/lib/cpp2/protocol/CompactProtocol.h"
#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TCompactProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/test/gen-cpp/UnionTest_types.h"
#include "thrift/test/gen-cpp2/UnionTest_types.h"

#include <memory>
#include <gtest/gtest.h>

using namespace thrift::test::debug;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace testing;
using namespace std;

class UnionTestFixture: public Test {
 public:
  void serializeDeserialize(TestUnion val) {
    auto strBuffer = std::make_shared<TMemoryBuffer>();
    auto protocol = std::make_shared<TBinaryProtocol>(strBuffer);

    val.write(protocol.get());
    string serialized = strBuffer->getBufferAsString();

    TestUnion out;
    out.read(protocol.get());
    EXPECT_EQ(val, out);
  }

  void serializeDeserialize2(cpp2::TestUnion val) {
    BinaryProtocolWriter prot;
    size_t bufSize = cpp2::TestUnion_serializedSize(&prot, &val);
    IOBufQueue queue(IOBufQueue::cacheChainLength());

    prot.setOutput(&queue, bufSize);
    cpp2::TestUnion_write(&prot, &val);

    bufSize = queue.chainLength();
    auto buf = queue.move();

    cpp2::TestUnion out;
    BinaryProtocolReader protReader;
    protReader.setInput(buf.get());
    cpp2::TestUnion_read(&protReader, &out);
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

TEST_F(UnionTestFixture, SerdeTest2) {
  cpp2::TestUnion u;
  u.set_i32_field(100);
  serializeDeserialize2(u);

  u.set_other_i32_field(200);
  serializeDeserialize2(u);

  u.set_string_field("str");
  serializeDeserialize2(u);

  u.set_struct_list(std::vector<RandomStuff>());
  serializeDeserialize2(u);
}

TEST_F(UnionTestFixture, FromJson) {
  TestUnion u;

  string j = "{\"i32_field\": 100}";
  u.readFromJson(j.c_str());
  EXPECT_EQ(TestUnion::Type::i32_field, u.getType());
  EXPECT_EQ(100, u.get_i32_field());

  j = "{\"string_field\": \"str\"}";
  u.readFromJson(j.c_str());
  EXPECT_EQ(TestUnion::Type::string_field, u.getType());
  EXPECT_EQ("str", u.get_string_field());

  j = "{\"random\": 123765}";
  u.readFromJson(j.c_str());
  EXPECT_EQ(TestUnion::Type::__EMPTY__, u.getType());
}
