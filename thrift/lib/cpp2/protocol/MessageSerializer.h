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

#pragma once

#include <memory>
#include <string>

#include <folly/Conv.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache {
namespace thrift {

/**
 * Serialize and Deserialize functions for <ThriftType>_(p)args and
 * <ThriftType>_(p)result.
 * These are the types sent on the wire for a method invocation.
 */

template <typename ProtocolWriter, typename T>
std::unique_ptr<folly::IOBuf> PargsPresultSerialize(
    const T& value,
    const char* methodName,
    MessageType messageType,
    int seqId) {
  IOBufQueue q;
  ProtocolWriter writer;
  writer.setOutput(&q, value.serializedSizeZC(&writer));
  writer.writeMessageBegin(methodName, messageType, seqId);
  value.write(&writer);
  writer.writeMessageEnd();
  return q.move();
}

template <typename ProtocolReader, typename T>
std::pair<std::string, int> PargsPresultDeserialize(
    T& valuep,
    const folly::IOBuf* iobuf,
    MessageType messageType) {
  ProtocolReader reader;
  reader.setInput(iobuf);
  std::string methodName;
  MessageType msgType;
  int privSeqId;
  try {
    reader.readMessageBegin(methodName, msgType, privSeqId);
    if (msgType != messageType) {
      throw TApplicationException(folly::to<std::string>(
          "Bad message type ", msgType, ". Expecting ", messageType));
    }
    valuep.read(&reader);
    reader.readMessageEnd();
  } catch (const std::out_of_range& e) {
    throw transport::TTransportException(
        transport::TTransportException::END_OF_FILE);
  }
  return std::make_pair(methodName, privSeqId);
}

template <typename T>
std::unique_ptr<folly::IOBuf> PargsPresultProtoSerialize(
    uint16_t protocol,
    const T& value,
    const char* methodName,
    MessageType messageType,
    int seqId) {
  switch (protocol) {
    case protocol::T_BINARY_PROTOCOL:
      return PargsPresultSerialize<BinaryProtocolWriter>(
          value, methodName, messageType, seqId);
    case protocol::T_COMPACT_PROTOCOL:
      return PargsPresultSerialize<CompactProtocolWriter>(
          value, methodName, messageType, seqId);
    default:
      throw TProtocolException(
          TProtocolException::NOT_IMPLEMENTED,
          "PargsPresultProtoSerialize doesn't implement this protocol: " +
              std::to_string(protocol));
  }
}

template <typename T>
std::pair<std::string, int> PargsPresultProtoDeserialize(
    uint16_t protocol,
    T& value,
    const folly::IOBuf* iobuf,
    MessageType messageType) {
  switch (protocol) {
    case protocol::T_BINARY_PROTOCOL:
      return PargsPresultDeserialize<BinaryProtocolReader>(
          value, iobuf, messageType);
    case protocol::T_COMPACT_PROTOCOL:
      return PargsPresultDeserialize<CompactProtocolReader>(
          value, iobuf, messageType);
    default:
      throw TProtocolException(
          TProtocolException::NOT_IMPLEMENTED,
          "PargsPresultProtoDeserialize doesn't implement this protocol: " +
              std::to_string(protocol));
  }
}

} // namespace thrift
} // namespace apache
