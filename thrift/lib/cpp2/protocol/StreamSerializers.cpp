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

#include "thrift/lib/cpp2/protocol/StreamSerializers.h"
#include "thrift/lib/cpp/protocol/TProtocolException.h"

using namespace apache::thrift;

StreamReader::StreamReader()
    : hasSetProtocolType_(false),
      bufferQueue_(folly::IOBufQueue::cacheChainLength()),
      numBytesRead_(0),
      hasReadEnd_(false) {
}

bool StreamReader::hasSetProtocolType() {
  return hasSetProtocolType_;
}

void StreamReader::setProtocolType(ProtocolType type) {
  CHECK(!hasSetProtocolType());

  if (type == ProtocolType::T_BINARY_PROTOCOL ||
      type == ProtocolType::T_COMPACT_PROTOCOL) {
    protocolType_ = type;
    hasSetProtocolType_ = true;

  } else {
    throw apache::thrift::protocol::TProtocolException(
          "Unsupported Reader Protocol");
  }
}

StreamFlag StreamReader::readFlag() {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  int8_t byteFlag;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.readByte(byteFlag);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.readByte(byteFlag);
  } else {
    CHECK(false);
  }

  StreamFlag flag = (StreamFlag) byteFlag;

  if (flag == StreamFlag::END) {
    hasReadEnd_ = true;
  }

  return flag;
}

int16_t StreamReader::readId() {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  int16_t paramId = 0;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.readI16(paramId);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.readI16(paramId);
  } else {
    CHECK(false);
  }
  return paramId;
}

int16_t StreamReader::readExceptionId() {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  int16_t exceptionId = 0;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.readI16(exceptionId);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.readI16(exceptionId);
  } else {
    CHECK(false);
  }
  return exceptionId;
}

uint32_t StreamReader::readNumDescriptions() {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  int32_t iNumDescriptions = 0;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.readI32(iNumDescriptions);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.readI32(iNumDescriptions);
  } else {
    CHECK(false);
  }
  return (uint32_t) iNumDescriptions;
}

void StreamReader::readDescription(int16_t& paramId, TType& type) {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  int8_t byteFlag;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.readI16(paramId);
    numBytesRead_ += binaryReader_.readByte(byteFlag);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.readI16(paramId);
    numBytesRead_ += compactReader_.readByte(byteFlag);
  } else {
    CHECK(false);
  }

  type = (TType) byteFlag;
}


void StreamReader::skipItem(TType type) {
  skip(type);
}

void StreamReader::skipException() {
  skip(TType::T_STRUCT);
}

void StreamReader::skip(TType type) {
  CHECK(hasSetProtocolType());
  CHECK(!hasReadEnd());

  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesRead_ += binaryReader_.skip(type);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesRead_ += compactReader_.skip(type);
  } else {
    CHECK(false);
  }
}

void StreamReader::insertBuffer(std::unique_ptr<folly::IOBuf>&& newBuffer) {
  CHECK(hasSetProtocolType());

  bool wasEmpty = bufferQueue_.empty();
  bufferQueue_.append(std::move(newBuffer));

  if (wasEmpty) {
    if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
      binaryReader_.setInput(bufferQueue_.front());
    } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
      compactReader_.setInput(bufferQueue_.front());
    } else {
      CHECK(false);
    }
  }
}

bool StreamReader::hasReadEnd() {
  return hasReadEnd_;
}

bool StreamReader::hasMoreToRead() {
  return numBytesRead_ < bufferQueue_.chainLength();
}

void StreamReader::discardBuffer() {
  numBytesRead_ = 0;
  // since IOBufQueue::clear() does not actually release the IOBufs, we use
  // this method to relase all the IOBufs in the queue
  bufferQueue_ = folly::IOBufQueue(folly::IOBufQueue::cacheChainLength());
}

StreamWriter::StreamWriter()
    : hasSetProtocolType_(false),
      bufferQueue_(folly::IOBufQueue::cacheChainLength()),
      numBytesWritten_(0),
      hasWrittenEnd_(false) {
}

bool StreamWriter::hasSetProtocolType() {
  return hasSetProtocolType_;
}

void StreamWriter::setProtocolType(ProtocolType type) {
  CHECK(!hasSetProtocolType());

  if (type == ProtocolType::T_BINARY_PROTOCOL ||
      type == ProtocolType::T_COMPACT_PROTOCOL) {
    protocolType_ = type;
    hasSetProtocolType_ = true;
    linkBufferWithProtocol();

  } else {
    throw apache::thrift::protocol::TProtocolException(
          "Unsupported Writer Protocol");
  }
}

void StreamWriter::writeNumDescriptions(uint32_t numStreams) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  int8_t byteFlag = (int8_t) StreamFlag::DESCRIPTION;
  int32_t iNumStreams = (int32_t) numStreams;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += binaryWriter_.writeByte(byteFlag);
    numBytesWritten_ += binaryWriter_.writeI32(iNumStreams);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += compactWriter_.writeByte(byteFlag);
    numBytesWritten_ += compactWriter_.writeI32(iNumStreams);
  } else {
    CHECK(false);
  }
}

void StreamWriter::writeDescription(int16_t paramId, TType type) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  int8_t byteType = (int8_t) type;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += binaryWriter_.writeI16(paramId);
    numBytesWritten_ += binaryWriter_.writeByte(byteType);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += compactWriter_.writeI16(paramId);
    numBytesWritten_ += compactWriter_.writeByte(byteType);
  } else {
    CHECK(false);
  }
}

void StreamWriter::writeAcknowledgement() {
  writeFlag(StreamFlag::ACKNOWLEDGE);
}

void StreamWriter::writeException(int16_t paramId) {
  writeFlagAndId(StreamFlag::EXCEPTION, paramId);
}

void StreamWriter::writeClose(int16_t paramId) {
  writeFlagAndId(StreamFlag::CLOSE, paramId);
}

void StreamWriter::writeFinish(int16_t paramId) {
  writeFlagAndId(StreamFlag::FINISH, paramId);
}

void StreamWriter::writeEnd() {
  writeFlag(StreamFlag::END);
  hasWrittenEnd_ = true;
}

bool StreamWriter::hasWrittenEnd() {
  return hasWrittenEnd_;
}

void StreamWriter::writeFlag(StreamFlag flag) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  int8_t byteFlag = (int8_t) flag;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += binaryWriter_.writeByte(byteFlag);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += compactWriter_.writeByte(byteFlag);
  } else {
    CHECK(false);
  }
}

void StreamWriter::writeFlagAndId(StreamFlag flag, int16_t paramId) {
  CHECK(hasSetProtocolType());
  CHECK(!hasWrittenEnd());

  int8_t byteFlag = (int8_t) flag;
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    numBytesWritten_ += binaryWriter_.writeByte(byteFlag);
    numBytesWritten_ += binaryWriter_.writeI16(paramId);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    numBytesWritten_ += compactWriter_.writeByte(byteFlag);
    numBytesWritten_ += compactWriter_.writeI16(paramId);
  } else {
    CHECK(false);
  }
}

uint32_t StreamWriter::getNumBytesWritten() {
  return numBytesWritten_;
}

std::unique_ptr<folly::IOBuf> StreamWriter::extractBuffer() {
  auto writtenPortion = bufferQueue_.split(numBytesWritten_);
  linkBufferWithProtocol();
  numBytesWritten_ = 0;
  return writtenPortion;
}

void StreamWriter::linkBufferWithProtocol() {
  CHECK(hasSetProtocolType());
  // TODO decide on a maximum size of buffers
  if (protocolType_ == ProtocolType::T_BINARY_PROTOCOL) {
    binaryWriter_.setOutput(&bufferQueue_);
  } else if (protocolType_ == ProtocolType::T_COMPACT_PROTOCOL) {
    compactWriter_.setOutput(&bufferQueue_);
  } else {
    CHECK(false);
  }
}

