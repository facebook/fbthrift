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

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

using namespace apache::thrift;

TEST(ProtocolSkipTest, SkipInt) {
  IOBufQueue queue;
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  writer.writeI32(123);
  auto buf = queue.move();
  CompactProtocolReader reader;
  reader.setInput(buf.get());
  reader.skip(TType::T_I32);
}

TEST(ProtocolSkipTest, SkipStop) {
  IOBufQueue queue;
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  writer.writeFieldStop();
  auto buf = queue.move();
  CompactProtocolReader reader;
  reader.setInput(buf.get());
  bool thrown = false;
  try {
    reader.skip(TType::T_STOP);
  } catch (const TProtocolException& ex) {
    EXPECT_EQ(TProtocolException::INVALID_DATA, ex.getType());
    thrown = true;
  }
  EXPECT_TRUE(thrown);
}

TEST(ProtocolSkipTest, SkipStopInContainer) {
  IOBufQueue queue;
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  writer.writeListBegin(TType::T_STOP, 1u << 30);
  auto buf = queue.move();
  CompactProtocolReader reader;
  reader.setInput(buf.get());
  bool thrown = false;
  try {
    reader.skip(TType::T_LIST);
  } catch (const TProtocolException& ex) {
    EXPECT_EQ(TProtocolException::INVALID_DATA, ex.getType());
    thrown = true;
  }
  EXPECT_TRUE(thrown);
}
