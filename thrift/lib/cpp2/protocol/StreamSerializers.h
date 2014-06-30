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
#ifndef THRIFT_PROTOCOL_STREAMSERIALIZERS_H_
#define THRIFT_PROTOCOL_STREAMSERIALIZERS_H_ 1

#include <exception>
#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache { namespace thrift {

enum class StreamFlag : int8_t {
    DESCRIPTION = 0,
    ITEM = 1,
    ACKNOWLEDGE = 2,
    CLOSE = 3,
    FINISH = 4,
    EXCEPTION = 5,
    END = 6,
};

class StreamReader {
  public:
    StreamReader();

    bool hasSetProtocolType();
    void setProtocolType(ProtocolType type);

    StreamFlag readFlag();
    int16_t readId();

    template <typename Deserializer, typename T>
    void readItem(T& value);

    int16_t readExceptionId();

    template <typename T>
    std::exception_ptr readExceptionItem();

    void skipItem(TType type);
    void skipException();

    uint32_t readNumDescriptions();
    void readDescription(int16_t& paramId, TType& type);

    void insertBuffer(std::unique_ptr<folly::IOBuf>&& newBuffer);

    bool hasReadEnd();

    bool hasMoreToRead();
    void discardBuffer();

  private:
    bool hasSetProtocolType_;
    ProtocolType protocolType_;
    BinaryProtocolReader binaryReader_;
    CompactProtocolReader compactReader_;
    folly::IOBufQueue bufferQueue_;
    uint32_t numBytesRead_;
    bool hasReadEnd_;

    void skip(TType type);
};

class StreamWriter {
  public:
    StreamWriter();

    bool hasSetProtocolType();
    void setProtocolType(ProtocolType type);

    template <typename Serializer, typename T>
    void writeItem(int16_t paramId, const T& value);

    template <typename T>
    void writeExceptionItem(int16_t exceptionId, const T& value);

    void writeNumDescriptions(uint32_t numStreams);
    void writeDescription(int16_t paramId, TType type);

    void writeAcknowledgement();
    void writeException(int16_t paramId);
    void writeClose(int16_t paramId);
    void writeFinish(int16_t paramId);
    void writeEnd();

    bool hasWrittenEnd();

    uint32_t getNumBytesWritten();
    std::unique_ptr<folly::IOBuf> extractBuffer();

  private:
    bool hasSetProtocolType_;
    ProtocolType protocolType_;
    BinaryProtocolWriter binaryWriter_;
    CompactProtocolWriter compactWriter_;
    folly::IOBufQueue bufferQueue_;
    uint32_t numBytesWritten_;
    bool hasWrittenEnd_;

    void writeFlag(StreamFlag flag);
    void writeFlagAndId(StreamFlag flag, int16_t paramId);
    void linkBufferWithProtocol();
};

}} // apache::thrift

#include <thrift/lib/cpp2/protocol/StreamSerializers.tcc>

#endif // THRIFT_PROTOCOL_STREAMSERIALIZERS_H_
