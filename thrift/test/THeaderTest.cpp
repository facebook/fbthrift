/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>

#include <gtest/gtest.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

// Listed individually to make it easy to see in the unit runner

TEST(THeaderTest, unframedBadRead) {
  auto buffer = make_shared<TMemoryBuffer>();
  auto transport = make_shared<THeaderTransport>(buffer);
  auto protocol = make_shared<THeaderProtocol>(transport);
  string name = "test";
  TMessageType messageType = T_CALL;
  int32_t seqId = 0;
  uint8_t buf1 = 0x80;
  uint8_t buf2 = 0x01;
  uint8_t buf3 = 0x00;
  uint8_t buf4 = 0x00;
  buffer->write(&buf1, 1);
  buffer->write(&buf2, 1);
  buffer->write(&buf3, 1);
  buffer->write(&buf4, 1);

  EXPECT_THROW(
      protocol->readMessageBegin(name, messageType, seqId),
      TTransportException);
}

TEST(THeaderTest, removeBadHeaderStringSize) {
  uint8_t badHeader[] = {
    0x00, 0x00, 0x00, 0x13, // Frame size is corrupted here
    0x0F, 0xFF, 0x00, 0x00, // THRIFT_HEADER_CLIENT_TYPE
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, // Header size
    0x00, // Proto ID
    0x00, // Num transforms
    0x01, // Info ID Key value
    0x01, // Num headers
    0xFF, 0xFF, 0xFF, 0xFF, // Malformed varint32 string size
    0x00 // String should go here
  };
  folly::IOBufQueue queue;
  queue.append(folly::IOBuf::wrapBuffer(badHeader, sizeof(badHeader)));
  // Try to remove the bad header
  THeader header;
  size_t needed;
  std::map<std::string, std::string> persistentHeaders;
  EXPECT_THROW(
    auto buf = header.removeHeader(&queue, needed, persistentHeaders),
    TTransportException
  );
}
