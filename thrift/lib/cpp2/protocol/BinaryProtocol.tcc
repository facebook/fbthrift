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

#ifndef THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_
#define THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_ 1

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <limits>
#include <string>
#include <boost/static_assert.hpp>

namespace apache { namespace thrift {

uint32_t BinaryProtocolWriter::writeMessageBegin(const std::string& name,
                                                 MessageType messageType,
                                                 int32_t seqid) {
  int32_t version = (VERSION_1) | ((int32_t)messageType);
  uint32_t wsize = 0;
  wsize += writeI32(version);
  wsize += writeString(name);
  wsize += writeI32(seqid);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeMessageEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeStructBegin(const char* /*name*/) {
  return 0;
}

uint32_t BinaryProtocolWriter::writeStructEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeFieldBegin(const char* /*name*/,
                                               TType fieldType,
                                               int16_t fieldId) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)fieldType);
  wsize += writeI16(fieldId);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeFieldEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeFieldStop() {
  return writeByte((int8_t)TType::T_STOP);
}

uint32_t BinaryProtocolWriter::writeMapBegin(const TType keyType,
                                             TType valType,
                                             uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)keyType);
  wsize += writeByte((int8_t)valType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeMapEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeListBegin(TType elemType,
                                              uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t) elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeListEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeSetBegin(TType elemType,
                                             uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeSetEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeBool(bool value) {
  uint8_t tmp =  value ? 1 : 0;
  out_.write(tmp);
  return sizeof(value);
}

uint32_t BinaryProtocolWriter::writeByte(int8_t byte) {
  out_.write(byte);
  return sizeof(byte);
}

uint32_t BinaryProtocolWriter::writeI16(int16_t i16) {
  out_.writeBE(i16);
  return sizeof(i16);
}

uint32_t BinaryProtocolWriter::writeI32(int32_t i32) {
  out_.writeBE(i32);
  return sizeof(i32);
}

uint32_t BinaryProtocolWriter::writeI64(int64_t i64) {
  out_.writeBE(i64);
  return sizeof(i64);
}

uint32_t BinaryProtocolWriter::writeDouble(double dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = bitwise_cast<uint64_t>(dub);
  out_.writeBE(bits);
  return sizeof(bits);
}

uint32_t BinaryProtocolWriter::writeFloat(float flt) {
  BOOST_STATIC_ASSERT(sizeof(float) == sizeof(uint32_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<float>::is_iec559);

  uint32_t bits = bitwise_cast<uint32_t>(flt);
  out_.writeBE(bits);
  return sizeof(bits);
}


uint32_t BinaryProtocolWriter::writeString(folly::StringPiece str) {
  return writeBinary(str);
}

uint32_t BinaryProtocolWriter::writeBinary(folly::StringPiece str) {
  return writeBinary(folly::ByteRange(str));
}

uint32_t BinaryProtocolWriter::writeBinary(folly::ByteRange v) {
  uint32_t size = v.size();
  uint32_t result = writeI32((int32_t)size);
  out_.push(v.data(), size);
  return result + size;
}

uint32_t BinaryProtocolWriter::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  DCHECK(str);
  if (!str) {
    return writeI32(0);
  }
  return writeBinary(*str);
}

uint32_t BinaryProtocolWriter::writeBinary(
    const folly::IOBuf& str) {
  size_t size = str.computeChainDataLength();
  // leave room for size
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  uint32_t result = writeI32((int32_t)size);
  auto clone = str.clone();
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    clone->makeManaged();
  }
  out_.insert(std::move(clone));
  return result + size;
}

uint32_t BinaryProtocolWriter::writeSerializedData(
    const std::unique_ptr<IOBuf>& buf) {
  if (!buf) {
    return 0;
  }
  // TODO: insert() just chains IOBufs together. Consider copying data to the
  // output buffer as it was already preallocated with the correct size.
  auto clone = buf->clone();
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    clone->makeManaged();
  }
  out_.insert(std::move(clone));
  return buf->computeChainDataLength();
}

/**
 * Functions that return the serialized size
 */

uint32_t BinaryProtocolWriter::serializedMessageSize(
  const std::string& name) {
  // I32{version} + String{name} + I32{seqid}
  return 2*serializedSizeI32() + serializedSizeString(name);
}

uint32_t BinaryProtocolWriter::serializedFieldSize(const char* /*name*/,
                                                   TType /*fieldType*/,
                                                   int16_t /*fieldId*/) {
  // byte + I16
  return serializedSizeByte() + serializedSizeI16();
}

uint32_t BinaryProtocolWriter::serializedStructSize(const char* /*name*/) {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeMapBegin(TType /*keyType*/,
                                                      TType /*valType*/,
                                                      uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeByte() +
         serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeMapEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeListBegin(TType /*elemType*/,
                                                       uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeListEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeSetBegin(TType /*elemType*/,
                                                      uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeSetEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeStop() {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeBool(bool /*val*/) {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeByte(int8_t /*val*/) {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeI16(int16_t /*val*/) {
  return 2;
}

uint32_t BinaryProtocolWriter::serializedSizeI32(int32_t /*val*/) {
  return 4;
}

uint32_t BinaryProtocolWriter::serializedSizeI64(int64_t /*val*/) {
  return 8;
}

uint32_t BinaryProtocolWriter::serializedSizeDouble(double /*val*/) {
  return 8;
}

uint32_t BinaryProtocolWriter::serializedSizeFloat(float /*val*/) {
  return 4;
}

uint32_t BinaryProtocolWriter::serializedSizeString(folly::StringPiece str) {
  return serializedSizeBinary(str);
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(folly::StringPiece str) {
  return serializedSizeBinary(folly::ByteRange(str));
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(folly::ByteRange str) {
  // I32{length of string} + binary{string contents}
  return serializedSizeI32() + str.size();
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    const std::unique_ptr<folly::IOBuf>& v) {
  return v ? serializedSizeBinary(*v) : 0;
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    const folly::IOBuf& v) {
  size_t size = v.computeChainDataLength();
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  return serializedSizeI32() + size;
}

uint32_t BinaryProtocolWriter::serializedSizeZCBinary(folly::StringPiece str) {
  return serializedSizeZCBinary(folly::ByteRange(str));
}
uint32_t BinaryProtocolWriter::serializedSizeZCBinary(folly::ByteRange v) {
  return serializedSizeBinary(v);
}
uint32_t BinaryProtocolWriter::serializedSizeZCBinary(
    const std::unique_ptr<folly::IOBuf>&) {
  // size only
  return serializedSizeI32();
}
uint32_t BinaryProtocolWriter::serializedSizeZCBinary(const folly::IOBuf&) {
  // size only
  return serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeSerializedData(
    const std::unique_ptr<IOBuf>& /*buf*/) {
  // writeSerializedData's implementation just chains IOBufs together. Thus
  // we don't expect external buffer space for it.
  return 0;
}

/**
 * Reading functions
 */

uint32_t BinaryProtocolReader::readMessageBegin(std::string& name,
                                                MessageType& messageType,
                                                int32_t& seqid) {
  uint32_t result = 0;
  int32_t sz;
  result += readI32(sz);

  if (sz < 0) {
    // Check for correct version number
    int32_t version = sz & VERSION_MASK;
    if (version != VERSION_1) {
      throw TProtocolException(TProtocolException::BAD_VERSION,
                               "Bad version identifier, sz=" +
                                 std::to_string(sz));
    }
    messageType = (MessageType)(sz & 0x000000ff);
    result += readString(name);
    result += readI32(seqid);
  } else {
    if (this->strict_read_) {
      throw TProtocolException (
        TProtocolException::BAD_VERSION,
        "No version identifier... old protocol client in strict mode? sz=" +
          std::to_string(sz));
    } else {
      // Handle pre-versioned input
      int8_t type;
      result += readStringBody(name, sz);
      result += readByte(type);
      messageType = (MessageType)type;
      result += readI32(seqid);
    }
  }

  return result;
}

uint32_t BinaryProtocolReader::readMessageEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readStructBegin(std::string& name) {
  name = "";
  return 0;
}

uint32_t BinaryProtocolReader::readStructEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readFieldBegin(std::string& /*name*/,
                                              TType& fieldType,
                                              int16_t& fieldId) {
  uint32_t result = 0;
  int8_t type;
  result += readByte(type);
  fieldType = (TType)type;
  if (fieldType == TType::T_STOP) {
    fieldId = 0;
    return result;
  }
  result += readI16(fieldId);
  return result;
}

uint32_t BinaryProtocolReader::readFieldEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readMapBegin(TType& keyType,
                                            TType& valType,
                                            uint32_t& size) {
  int8_t k, v;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(k);
  keyType = (TType)k;
  result += readByte(v);
  valType = (TType)v;
  result += readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
  return result;
}

uint32_t BinaryProtocolReader::readMapEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readListBegin(TType& elemType,
                                             uint32_t& size) {
  int8_t e;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(e);
  elemType = (TType)e;
  result += readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
  return result;
}

uint32_t BinaryProtocolReader::readListEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readSetBegin(TType& elemType,
                                            uint32_t& size) {
  int8_t e;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(e);
  elemType = (TType)e;
  result += readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
  return result;
}

uint32_t BinaryProtocolReader::readSetEnd() {
  return 0;
}

uint32_t BinaryProtocolReader::readBool(bool& value) {
  value = in_.read<bool>();
  return 1;
}

uint32_t BinaryProtocolReader::readBool(std::vector<bool>::reference value) {
  bool ret = false;
  ret = in_.read<bool>();
  value = ret;
  return 1;
}

uint32_t BinaryProtocolReader::readByte(int8_t& byte) {
  byte = in_.read<int8_t>();
  return 1;
}

uint32_t BinaryProtocolReader::readI16(int16_t& i16) {
  i16 = in_.readBE<int16_t>();
  return 2;
}

uint32_t BinaryProtocolReader::readI32(int32_t& i32) {
  i32 = in_.readBE<int32_t>();
  return 4;
}

uint32_t BinaryProtocolReader::readI64(int64_t& i64) {
  i64 = in_.readBE<int64_t>();
  return 8;
}

uint32_t BinaryProtocolReader::readDouble(double& dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = in_.readBE<int64_t>();
  dub = bitwise_cast<double>(bits);
  return 8;
}

uint32_t BinaryProtocolReader::readFloat(float& flt) {
  BOOST_STATIC_ASSERT(sizeof(float) == sizeof(uint32_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint32_t bits = in_.readBE<int32_t>();
  flt = bitwise_cast<float>(bits);
  return 4;
}

void BinaryProtocolReader::checkStringSize(int32_t size) {
  // Catch error cases
  if (size < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  }
  if (string_limit_ > 0 && size > string_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
}

void BinaryProtocolReader::checkContainerSize(int32_t size) {
  if (size < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (this->container_limit_ && size > this->container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
}

template<typename StrType>
uint32_t BinaryProtocolReader::readString(StrType& str) {
  uint32_t result;
  int32_t size;
  result = readI32(size);
  return result + readStringBody(str, size);
}

template <typename StrType>
uint32_t BinaryProtocolReader::readBinary(StrType& str) {
  return readString(str);
}

uint32_t BinaryProtocolReader::readBinary(std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    str = folly::make_unique<folly::IOBuf>();
  }
  return readBinary(*str);
}

uint32_t BinaryProtocolReader::readBinary(folly::IOBuf& str) {
  uint32_t result;
  int32_t size;
  result = readI32(size);
  checkStringSize(size);

  in_.clone(str, size);
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    str.makeManaged();
  }
  return result + (uint32_t)size;
}

template<typename StrType>
uint32_t BinaryProtocolReader::readStringBody(StrType& str,
                                              int32_t size) {
  checkStringSize(size);

  uint32_t result = 0;

  // Catch empty string case
  if (size == 0) {
    str.clear();
    return result;
  }

  str.reserve(size);
  str.clear();
  size_t size_left = size;
  while (size_left > 0) {
    std::pair<const uint8_t*, size_t> data = in_.peek();
    int32_t data_avail = std::min(data.second, size_left);
    if (data.second <= 0) {
      throw TProtocolException(TProtocolException::SIZE_LIMIT);
    }

    str.append((const char*)data.first, data_avail);
    size_left -= data_avail;
    in_.skip(data_avail);
  }

  return (uint32_t) size;
}

uint32_t BinaryProtocolReader::readFromPositionAndAppend(
    Cursor& snapshot,
    std::unique_ptr<IOBuf>& ser) {

  int32_t size = in_ - snapshot;

  if (ser) {
    std::unique_ptr<IOBuf> newBuf;
    snapshot.clone(newBuf, size);
    if (sharing_ != SHARE_EXTERNAL_BUFFER) {
      newBuf->makeManaged();
    }
    // IOBuf are circular, so prependChain called on head is the same as
    // appending the whole chain at the tail.
    ser->prependChain(std::move(newBuf));
  } else {
    // cut a chunk of things directly
    snapshot.clone(ser, size);
    if (sharing_ != SHARE_EXTERNAL_BUFFER) {
      ser->makeManaged();
    }
  }

  return (uint32_t) size;
}

}} // apache2::thrift

#endif // #ifndef THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_
