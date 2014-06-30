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

#ifndef THRIFT_PROTOCOL_STREAMSERIALIZERS_TCC_
#define THRIFT_PROTOCOL_STREAMSERIALIZERS_TCC_ 1

#include <thrift/lib/cpp2/protocol/StreamSerializers.h>

namespace apache { namespace thrift {

template <typename Deserializer, typename T>
void StreamReader::readItem(T& value) {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += Deserializer::deserialize(binaryReader_, value);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += Deserializer::deserialize(compactReader_, value);
  } else {
    CHECK(false);
  }
}

template <typename T>
std::exception_ptr StreamReader::readExceptionItem() {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  T value;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += value.read(&binaryReader_);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += value.read(&compactReader_);
  } else {
    CHECK(false);
  }

  return std::make_exception_ptr(value);
}

template <typename Serializer, typename T>
void StreamWriter::writeItem(int16_t paramId, const T& value) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  writeFlagAndId(StreamFlag::ITEM, paramId);

  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += Serializer::serialize(binaryWriter_, value);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += Serializer::serialize(compactWriter_, value);
  } else {
    CHECK(false);
  }
}

template <typename T>
void StreamWriter::writeExceptionItem(int16_t exceptionId, const T& value) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += binaryWriter_.writeI16(exceptionId);
    numBytesWritten_ += value.write(&binaryWriter_);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += compactWriter_.writeI16(exceptionId);
    numBytesWritten_ += value.write(&compactWriter_);
  } else {
    CHECK(false);
  }
}

}} // apache::thrift

#endif // THRIFT_PROTOCOL_STREAMSERIALIZERS_TCC_
