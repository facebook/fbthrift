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

#include <gtest/gtest.h>

#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

using apache::thrift::CompactProtocolReader;
using apache::thrift::CompactProtocolWriter;
using apache::thrift::MessageType;

namespace apache::thrift {

class TCompactProtocolTest : public testing::Test {};

void testTMessageWriteAndRead(
    const std::string& name, MessageType msgType, int32_t seqId) {
  folly::IOBufQueue queue;
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  writer.writeMessageBegin(name, msgType, seqId);

  auto buf = queue.move();
  CompactProtocolReader reader;
  reader.setInput(buf.get());

  std::string readName;
  MessageType readMsgType;
  int32_t readSeqId = 0;
  reader.readMessageBegin(readName, readMsgType, readSeqId);

  EXPECT_EQ(readName, name);
  EXPECT_EQ(readMsgType, msgType);
  EXPECT_EQ(readSeqId, seqId);
}

TEST_F(TCompactProtocolTest, test_readMessageBegin) {
  testTMessageWriteAndRead("methodName", MessageType::T_CALL, 1);
  testTMessageWriteAndRead("", MessageType::T_CALL, 1);

  testTMessageWriteAndRead("methodName", MessageType::T_CALL, 0);
  testTMessageWriteAndRead("methodName", MessageType::T_CALL, -1);
  testTMessageWriteAndRead(
      "methodName", MessageType::T_CALL, std::numeric_limits<int32_t>::max());
  testTMessageWriteAndRead(
      "methodName", MessageType::T_CALL, std::numeric_limits<int32_t>::min());

  testTMessageWriteAndRead("methodName", MessageType::T_CALL, 1);
  testTMessageWriteAndRead("methodName", MessageType::T_REPLY, 1);
  testTMessageWriteAndRead("methodName", MessageType::T_EXCEPTION, 1);
  testTMessageWriteAndRead("methodName", MessageType::T_ONEWAY, 1);
}

} // namespace apache::thrift
