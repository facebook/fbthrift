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

#pragma once

#include <string>
#include <memory>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp/TApplicationException.h>

namespace apache { namespace thrift {

/**
 * Serialize and Deserialize functions for <ThriftType>_(p)args and
 * <ThriftType>_(p)result.
 * These are the types sent on the wire for a method invocation.
 */

template <typename T>
std::unique_ptr<folly::IOBuf> PargsPresultCompactSerialize(
    const T& value, const char *methodName, MessageType messageType, int seqId) {
  IOBufQueue q;
  CompactProtocolWriter writer;
  writer.setOutput(&q);
  writer.writeMessageBegin(methodName, messageType, seqId);
  value.write(&writer);
  writer.writeMessageEnd();
  return q.move();
}

template <typename T>
std::pair<std::string, int> PargsPresultCompactDeserialize(
    T& valuep, const folly::IOBuf* iobuf, MessageType messageType) {
  CompactProtocolReader reader;
  reader.setInput(iobuf);
  std::string methodName;
  MessageType msgType;
  int privSeqId;
  reader.readMessageBegin(methodName, msgType, privSeqId);
  if (msgType != messageType) {
    throw TApplicationException(
        folly::to<std::string>("Bad message type ", msgType,
                          ". Expecting ", messageType));
  }
  valuep.read(&reader);
  reader.readMessageEnd();
  return std::make_pair(methodName, privSeqId);
}

}}
