/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include "thrift/test/gen-cpp2/QualifiedEnumTest_types.h"

using namespace apache::thrift;
using namespace cpp2;

// If field1 is unspecified, default should be BAR
TEST(QualifiedEnums, AmbiguousDefault1) {
  MyStruct expected;
  expected.field1 = MyEnum::BAR;

  MyStruct original;
  original.__isset.field1 = false;

  BinaryProtocolWriter protWriter;
  size_t bufSize = Cpp2Ops<MyStruct>::serializedSize(&protWriter, &expected);
  folly::IOBufQueue queue;
  protWriter.setOutput(&queue, bufSize);
  Cpp2Ops<MyStruct>::write(&protWriter, &expected);

  auto buf = queue.move();
  BinaryProtocolReader protReader;
  protReader.setInput(buf.get());
  MyStruct actual;
  Cpp2Ops<MyStruct>::read(&protReader, &actual);
  EXPECT_EQ(expected.field1, actual.field1);
}

// If field2 is unspecified, default should be FOO
TEST(QualifiedEnums, AmbiguousDefault2) {
  MyStruct expected;
  expected.field2 = MyEnum::FOO;

  MyStruct original;
  original.__isset.field2 = false;

  BinaryProtocolWriter protWriter;
  size_t bufSize = Cpp2Ops<MyStruct>::serializedSize(&protWriter, &expected);
  folly::IOBufQueue queue;
  protWriter.setOutput(&queue, bufSize);
  Cpp2Ops<MyStruct>::write(&protWriter, &expected);

  auto buf = queue.move();
  BinaryProtocolReader protReader;
  protReader.setInput(buf.get());
  MyStruct actual;
  Cpp2Ops<MyStruct>::read(&protReader, &actual);
  EXPECT_EQ(expected.field2, actual.field2);
}
