/*
 * Copyright 2016-present Facebook, Inc.
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

/*
 * Deprecated. For backcompat only with stored data serialized with this version
 * of compact protocol.
 *
 * Deprecated because it serializes doubles incorrectly, in little-endian byte
 * order, and is therefore incompatible with all other language implementations.
 */

#pragma once

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache { namespace thrift {

class CompactV1ProtocolReader;

class CompactV1ProtocolWriter : protected CompactProtocolWriter {

 public:

  using ProtocolReader = CompactV1ProtocolReader;

  using CompactProtocolWriter::CompactProtocolWriter;
  using CompactProtocolWriter::protocolType;
  using CompactProtocolWriter::setOutput;

  inline uint32_t writeMessageBegin(
      const std::string& name,
      MessageType messageType,
      int32_t seqid);
  using CompactProtocolWriter::writeMessageEnd;
  using CompactProtocolWriter::writeStructBegin;
  using CompactProtocolWriter::writeStructEnd;
  using CompactProtocolWriter::writeFieldBegin;
  using CompactProtocolWriter::writeFieldEnd;
  using CompactProtocolWriter::writeFieldStop;
  using CompactProtocolWriter::writeMapBegin;
  using CompactProtocolWriter::writeMapEnd;
  using CompactProtocolWriter::writeCollectionBegin;
  using CompactProtocolWriter::writeListBegin;
  using CompactProtocolWriter::writeListEnd;
  using CompactProtocolWriter::writeSetBegin;
  using CompactProtocolWriter::writeSetEnd;
  using CompactProtocolWriter::writeBool;
  using CompactProtocolWriter::writeByte;
  using CompactProtocolWriter::writeI16;
  using CompactProtocolWriter::writeI32;
  using CompactProtocolWriter::writeI64;
  inline uint32_t writeDouble(double dub);
  using CompactProtocolWriter::writeFloat;
  using CompactProtocolWriter::writeString;
  using CompactProtocolWriter::writeBinary;
  using CompactProtocolWriter::writeSerializedData;

  using CompactProtocolWriter::serializedMessageSize;
  using CompactProtocolWriter::serializedFieldSize;
  using CompactProtocolWriter::serializedStructSize;
  using CompactProtocolWriter::serializedSizeMapBegin;
  using CompactProtocolWriter::serializedSizeMapEnd;
  using CompactProtocolWriter::serializedSizeListBegin;
  using CompactProtocolWriter::serializedSizeListEnd;
  using CompactProtocolWriter::serializedSizeSetBegin;
  using CompactProtocolWriter::serializedSizeSetEnd;
  using CompactProtocolWriter::serializedSizeStop;
  using CompactProtocolWriter::serializedSizeBool;
  using CompactProtocolWriter::serializedSizeByte;
  using CompactProtocolWriter::serializedSizeI16;
  using CompactProtocolWriter::serializedSizeI32;
  using CompactProtocolWriter::serializedSizeI64;
  using CompactProtocolWriter::serializedSizeDouble;
  using CompactProtocolWriter::serializedSizeFloat;
  using CompactProtocolWriter::serializedSizeString;
  using CompactProtocolWriter::serializedSizeBinary;
  using CompactProtocolWriter::serializedSizeZCBinary;
  using CompactProtocolWriter::serializedSizeSerializedData;

};

class CompactV1ProtocolReader : protected CompactProtocolReader {

 public:

  using ProtocolWriter = CompactV1ProtocolWriter;

  using CompactProtocolReader::CompactProtocolReader;
  using CompactProtocolReader::protocolType;
  using CompactProtocolReader::kUsesFieldNames;
  using CompactProtocolReader::kOmitsContainerSizes;

  using CompactProtocolReader::setStringSizeLimit;
  using CompactProtocolReader::setContainerSizeLimit;
  using CompactProtocolReader::setInput;

  inline uint32_t readMessageBegin(
      std::string& name,
      MessageType& messageType,
      int32_t& seqid);
  using CompactProtocolReader::readMessageEnd;
  using CompactProtocolReader::readStructBegin;
  using CompactProtocolReader::readStructEnd;
  using CompactProtocolReader::readFieldBegin;
  using CompactProtocolReader::readFieldEnd;
  using CompactProtocolReader::readMapBegin;
  using CompactProtocolReader::readMapEnd;
  using CompactProtocolReader::readListBegin;
  using CompactProtocolReader::readListEnd;
  using CompactProtocolReader::readSetBegin;
  using CompactProtocolReader::readSetEnd;
  using CompactProtocolReader::readBool;
  using CompactProtocolReader::readByte;
  using CompactProtocolReader::readI16;
  using CompactProtocolReader::readI32;
  using CompactProtocolReader::readI64;
  inline uint32_t readDouble(double& dub);
  using CompactProtocolReader::readFloat;
  using CompactProtocolReader::readString;
  using CompactProtocolReader::readBinary;
  using CompactProtocolReader::skip;
  using CompactProtocolReader::peekMap;
  using CompactProtocolReader::peekSet;
  using CompactProtocolReader::peekList;

  using CompactProtocolReader::getCurrentPosition;
  using CompactProtocolReader::readFromPositionAndAppend;

};

}}

#include <thrift/lib/cpp2/protocol/CompactV1Protocol.tcc>
