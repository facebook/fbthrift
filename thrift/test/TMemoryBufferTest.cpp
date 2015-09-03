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

#include <iostream>
#include <climits>
#include <cassert>
#include <folly/Memory.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/test/gen-cpp/test_types.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TTransportException;
using apache::thrift::protocol::TBinaryProtocol;

TEST(TMemoryBufferTest,  test_roundtrip) {
  auto strBuffer = make_shared<TMemoryBuffer>();
  auto binaryProtcol = make_shared<TBinaryProtocol>(strBuffer);

  test_cpp::struct1 a;
  a.a = 10;
  a.b = "holla back a";

  a.write(binaryProtcol.get());
  string serialized = strBuffer->getBufferAsString();

  auto strBuffer2 = make_shared<TMemoryBuffer>();
  auto binaryProtcol2 = make_shared<TBinaryProtocol>(strBuffer2);

  strBuffer2->resetBuffer((uint8_t*)serialized.data(), serialized.length());
  test_cpp::struct1 a2;
  a2.read(binaryProtcol2.get());

  EXPECT_EQ(a, a2);
}

TEST(TMemoryBufferTest,  test_copy) {
  auto str1 = make_unique<string>("abcd1234");
  TMemoryBuffer buf((uint8_t*)str1->data(), str1->length(), TMemoryBuffer::COPY);
  str1 = nullptr;

  string str3 = "wxyz", str4 = "6789";
  buf.readAppendToString(str3, 4);
  buf.readAppendToString(str4, std::numeric_limits<int>::max());

  EXPECT_EQ("wxyzabcd", str3);
  EXPECT_EQ("67891234", str4);
}

TEST(TMemoryBufferTest,  test_exceptions) {
  char data[] = "foo\0bar";

  TMemoryBuffer buf1((uint8_t*)data, 7, TMemoryBuffer::OBSERVE);
  string str = buf1.getBufferAsString();
  ASSERT_EQ(7, str.length());

  buf1.resetBuffer();
  EXPECT_THROW(buf1.write((const uint8_t*)"foo", 3), TTransportException);

  TMemoryBuffer buf2((uint8_t*)data, 7, TMemoryBuffer::COPY);
  buf2.write((const uint8_t*)"bar", 3);
}
