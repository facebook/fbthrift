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

#ifndef THRIFT2_PROTOCOL_COMPACTPROTOCOL_TCC_
#define THRIFT2_PROTOCOL_COMPACTPROTOCOL_TCC_ 1

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp/util/VarintUtils.h>

#include <limits>

namespace apache { namespace thrift {

namespace detail { namespace compact {

enum Types {
  CT_STOP           = 0x00,
  CT_BOOLEAN_TRUE   = 0x01,
  CT_BOOLEAN_FALSE  = 0x02,
  CT_BYTE           = 0x03,
  CT_I16            = 0x04,
  CT_I32            = 0x05,
  CT_I64            = 0x06,
  CT_DOUBLE         = 0x07,
  CT_BINARY         = 0x08,
  CT_LIST           = 0x09,
  CT_SET            = 0x0A,
  CT_MAP            = 0x0B,
  CT_STRUCT         = 0x0C,
  CT_FLOAT          = 0x0D,
};

const int8_t TTypeToCType[20] = {
  CT_STOP, // T_STOP
  0, // unused
  CT_BOOLEAN_TRUE, // T_BOOL
  CT_BYTE, // T_BYTE
  CT_DOUBLE, // T_DOUBLE
  0, // unused
  CT_I16, // T_I16
  0, // unused
  CT_I32, // T_I32
  0, // unused
  CT_I64, // T_I64
  CT_BINARY, // T_STRING
  CT_STRUCT, // T_STRUCT
  CT_MAP, // T_MAP
  CT_SET, // T_SET
  CT_LIST, // T_LIST
  0, // unused
  0, // unused
  0, // unused
  CT_FLOAT, // T_FLOAT
};

const TType CTypeToTType[14] = {
    TType::T_STOP,    // CT_STOP
    TType::T_BOOL,    // CT_BOOLEAN_TRUE
    TType::T_BOOL,    // CT_BOOLEAN_FASLE
    TType::T_BYTE,    // CT_BYTE
    TType::T_I16,     // CT_I16
    TType::T_I32,     // CT_I32
    TType::T_I64,     // CT_I64
    TType::T_DOUBLE,  // CT_DOUBLE
    TType::T_STRING,  // CT_BINARY
    TType::T_LIST,    // CT_LIST
    TType::T_SET,     // CT_SET
    TType::T_MAP,     // CT_MAP
    TType::T_STRUCT,  // CT_STRUCT
    TType::T_FLOAT,   // CT_FLOAT
};

}} // end detail::compact namespace


template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeMessageBegin(
    const std::string& name,
    MessageType messageType,
    int32_t seqid) {
  uint32_t wsize = 0;
  wsize += writeByte(detail::compact::PROTOCOL_ID);
  wsize += writeByte(detail::compact::COMPACT_PROTOCOL_VERSION |
                     ((messageType << detail::compact::TYPE_SHIFT_AMOUNT) &
                      detail::compact::TYPE_MASK ));
  wsize += apache::thrift::util::writeVarint(out_, seqid);
  wsize += writeString(name);
  return wsize;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeMessageEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeStructBegin(
    const char* /* name */) {
  lastField_.push(lastFieldId_);
  lastFieldId_ = 0;
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeStructEnd() {
  lastFieldId_ = lastField_.top();
  lastField_.pop();
  return 0;
}

/**
 * The workhorse of writeFieldBegin. It has the option of doing a
 * 'type override' of the type header. This is used specifically in the
 * boolean field case.
 */
template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeFieldBeginInternal(
    const char* /*name*/,
    const TType fieldType,
    const int16_t fieldId,
    int8_t typeOverride) {
  uint32_t wsize = 0;

  // if there's a type override, use that.
  int8_t typeToWrite = (typeOverride == -1 ?
                        detail::compact::TTypeToCType[fieldType] :
                        typeOverride);

  // check if we can use delta encoding for the field id
  if (fieldId > lastFieldId_ && fieldId - lastFieldId_ <= 15) {
    // write them together
    wsize += writeByte((fieldId - lastFieldId_) << 4 | typeToWrite);
  } else {
    // write them separate
    wsize += writeByte(typeToWrite);
    wsize += writeI16(fieldId);
  }

  lastFieldId_ = fieldId;
  return wsize;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeFieldBegin(
    const char* name,
    TType fieldType,
    int16_t fieldId) {
  if (fieldType == TType::T_BOOL) {
    booleanField_.name = name;
    booleanField_.fieldType = fieldType;
    booleanField_.fieldId = fieldId;
  } else {
    return writeFieldBeginInternal(name, fieldType, fieldId, -1);
  }
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeFieldEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeFieldStop() {
  return writeByte((int8_t)TType::T_STOP);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeMapBegin(
    const TType keyType,
    TType valType,
    uint32_t size) {
  uint32_t wsize = 0;

  if (size == 0) {
    wsize += writeByte(0);
  } else {
    wsize += apache::thrift::util::writeVarint(out_, size);
    wsize += writeByte(detail::compact::TTypeToCType[keyType] << 4 |
                       detail::compact::TTypeToCType[valType]);
  }
  return wsize;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeMapEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeCollectionBegin(
    int8_t elemType,
    int32_t size) {
  uint32_t wsize = 0;
  if (size <= 14) {
    wsize += writeByte(size << 4 | detail::compact::TTypeToCType[elemType]);
  } else {
    wsize += writeByte(0xf0 | detail::compact::TTypeToCType[elemType]);
    wsize += apache::thrift::util::writeVarint(out_, size);
  }
  return wsize;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeListBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeListEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeSetBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeSetEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeBool(bool value) {
  uint32_t wsize = 0;

  if (booleanField_.name != NULL) {
    // we haven't written the field header yet
    wsize += writeFieldBeginInternal(booleanField_.name,
                                     booleanField_.fieldType,
                                     booleanField_.fieldId,
                                     value ? detail::compact::CT_BOOLEAN_TRUE :
                                     detail::compact::CT_BOOLEAN_FALSE);
    booleanField_.name = NULL;
  } else {
    // we're not part of a field, so just write the value
    wsize += writeByte(value ? detail::compact::CT_BOOLEAN_TRUE :
                       detail::compact::CT_BOOLEAN_FALSE);
  }
  return wsize;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeByte(int8_t byte) {
  out_.write(byte);
  return 1;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeI16(int16_t i16) {
  uint32_t sz = apache::thrift::util::writeVarint(out_, apache::thrift::util::i32ToZigzag(i16));
  return sz;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeI32(int32_t i32) {
  uint32_t sz = apache::thrift::util::writeVarint(out_, apache::thrift::util::i32ToZigzag(i32));
  return sz;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeI64(int64_t i64) {
  return apache::thrift::util::writeVarint(out_, apache::thrift::util::i64ToZigzag(i64));
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeDouble(double dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint64_t bits = bitwise_cast<uint64_t>(dub);
  out_.writeBE(bits);
  return sizeof(bits);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeFloat(float flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = bitwise_cast<uint32_t>(flt);
  out_.writeBE(bits);
  return sizeof(bits);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeString(
    folly::StringPiece str) {
  return writeBinary(str);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeBinary(
    folly::StringPiece str) {
  return writeBinary(folly::ByteRange(str));
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeBinary(
    folly::ByteRange str) {
  uint32_t size = str.size();
  uint32_t result = apache::thrift::util::writeVarint(out_, (int32_t)size);
  out_.push(str.data(), size);
  return result + size;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    return writeI32(0);
  }
  return writeBinary(*str);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::writeBinary(
    const folly::IOBuf& str) {
  size_t size = str.computeChainDataLength();
  // leave room for varint size
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  uint32_t result = apache::thrift::util::writeVarint(out_, (int32_t)size);
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    auto clone = str.clone();
    clone->makeManaged();
    out_.insert(std::move(clone));
  } else {
    out_.insert(str);
  }
  return result + size;
}

/**
 * Functions that return the serialized size
 */

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedMessageSize(
    const std::string& name) {
  // I32{version} + String{name} + I32{seqid}
  return 2*serializedSizeI32() + serializedSizeString(name);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedFieldSize(
    const char* /*name*/,
    TType /*fieldType*/,
    int16_t /*fieldId*/) {
  // byte + I16
  return serializedSizeByte() + serializedSizeI16();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedStructSize(
    const char* /*name*/) {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeMapBegin(
    TType /*keyType*/,
    TType /*valType*/,
    uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeByte() +
         serializedSizeI32();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeMapEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeListBegin(
    TType /*elemType*/,
    uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeI32();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeListEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeSetBegin(
    TType /*elemType*/,
    uint32_t /*size*/) {
  return serializedSizeByte() + serializedSizeI32();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeSetEnd() {
  return 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeStop() {
  return 1;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeBool(
    bool /* val */) {
  return 1;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeByte(
    int8_t /* val */) {
  return 1;
}

// Varint writes can be up to one additional byte
template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeI16(
    int16_t /*val*/) {
  return 3;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeI32(
    int32_t /*val*/) {
  return 5;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeI64(
    int64_t /*val*/) {
  return 10;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeDouble(
    double /*val*/) {
  return 8;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeFloat(
    float /*val*/) {
  return 4;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeString(
    folly::StringPiece str) {
  return serializedSizeBinary(str);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeBinary(
    folly::StringPiece str) {
  return serializedSizeBinary(folly::ByteRange(str));
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeBinary(
    folly::ByteRange v) {
  // I32{length of string} + binary{string contents}
  return serializedSizeI32() + v.size();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeBinary(
    const std::unique_ptr<folly::IOBuf>& v) {
  return v ? serializedSizeBinary(*v) : 0;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeBinary(
    const folly::IOBuf& v) {
  size_t size = v.computeChainDataLength();
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  return serializedSizeI32() + size;
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeZCBinary(
    folly::StringPiece str) {
  return serializedSizeZCBinary(folly::ByteRange(str));
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeZCBinary(
    folly::ByteRange v) {
  return serializedSizeBinary(v);
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeZCBinary(
    const std::unique_ptr<IOBuf>& /*v*/) {
  // size only
  return serializedSizeI32();
}

template <class Appender, class Storage>
uint32_t CompactProtocolWriterImpl<Appender, Storage>::serializedSizeZCBinary(
    const IOBuf& /*v*/) {
  // size only
  return serializedSizeI32();
}

/**
 * Reading functions
 */

uint32_t CompactProtocolReader::readMessageBegin(std::string& name,
                                                MessageType& messageType,
                                                int32_t& seqid) {
  uint32_t rsize = 0;
  int8_t protocolId;
  int8_t versionAndType;

  rsize += readByte(protocolId);
  if (protocolId != detail::compact::PROTOCOL_ID) {
    throw TProtocolException(TProtocolException::BAD_VERSION,
                             "Bad protocol identifier");
  }

  rsize += readByte(versionAndType);
  if ((int8_t)(versionAndType & VERSION_MASK) !=
      detail::compact::COMPACT_PROTOCOL_VERSION) {
    throw TProtocolException(TProtocolException::BAD_VERSION,
                             "Bad protocol version");
  }

  messageType = (MessageType)
    ((versionAndType & detail::compact::TYPE_MASK) >>
     detail::compact::TYPE_SHIFT_AMOUNT);
  rsize += apache::thrift::util::readVarint(in_, seqid);
  rsize += readString(name);

  return rsize;
}

uint32_t CompactProtocolReader::readMessageEnd() {
  return 0;
}

uint32_t CompactProtocolReader::readStructBegin(std::string& name) {
  if (!name.empty()) {
    name.clear();
  }
  lastField_.push(lastFieldId_);
  lastFieldId_ = 0;
  return 0;
}

uint32_t CompactProtocolReader::readStructEnd() {
  lastFieldId_ = lastField_.top();
  lastField_.pop();
  return 0;
}

uint32_t CompactProtocolReader::readFieldBegin(std::string& /*name*/,
                                              TType& fieldType,
                                              int16_t& fieldId) {
  uint32_t rsize = 0;
  int8_t byte;
  int8_t type;

  rsize += readByte(byte);
  type = (byte & 0x0f);

  // if it's a stop, then we can return immediately, as the struct is over.
  if (type == TType::T_STOP) {
    fieldType = TType::T_STOP;
    fieldId = 0;
    return rsize;
  }

  // mask off the 4 MSB of the type header. it could contain a field id delta.
  int16_t modifier = (int16_t)(((uint8_t)byte & 0xf0) >> 4);
  if (modifier == 0) {
    // not a delta, look ahead for the zigzag varint field id.
    rsize += readI16(fieldId);
  } else {
    fieldId = (int16_t)(lastFieldId_ + modifier);
  }
  fieldType = getType(type);

  // if this happens to be a boolean field, the value is encoded in the type
  if (type == detail::compact::CT_BOOLEAN_TRUE ||
      type == detail::compact::CT_BOOLEAN_FALSE) {
    // save the boolean value in a special instance variable.
    boolValue_.hasBoolValue = true;
    boolValue_.boolValue =
      (type == detail::compact::CT_BOOLEAN_TRUE ? true : false);
  }

  // push the new field onto the field stack so we can keep the deltas going.
  lastFieldId_ = fieldId;
  return rsize;
}

uint32_t CompactProtocolReader::readFieldEnd() {
  return 0;
}

uint32_t CompactProtocolReader::readMapBegin(TType& keyType,
                                            TType& valType,
                                            uint32_t& size) {
  uint32_t rsize = 0;
  int8_t kvType = 0;
  int32_t msize = 0;

  rsize += apache::thrift::util::readVarint(in_, msize);
  if (msize != 0)
    rsize += readByte(kvType);

  if (msize < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (container_limit_ && msize > container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }

  keyType = getType((int8_t)((uint8_t)kvType >> 4));
  valType = getType((int8_t)((uint8_t)kvType & 0xf));
  size = (uint32_t)msize;

  return rsize;
}

uint32_t CompactProtocolReader::readMapEnd() {
  return 0;
}

uint32_t CompactProtocolReader::readListBegin(TType& elemType,
                                             uint32_t& size) {
  int8_t size_and_type;
  uint32_t rsize = 0;
  int32_t lsize;

  rsize += readByte(size_and_type);

  lsize = ((uint8_t)size_and_type >> 4) & 0x0f;
  if (lsize == 15) {
    rsize += apache::thrift::util::readVarint(in_, lsize);
  }

  if (lsize < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (container_limit_ && lsize > container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }

  elemType = getType((int8_t)(size_and_type & 0x0f));
  size = (uint32_t)lsize;

  return rsize;
}

uint32_t CompactProtocolReader::readListEnd() {
  return 0;
}

uint32_t CompactProtocolReader::readSetBegin(TType& elemType,
                                            uint32_t& size) {
  return readListBegin(elemType, size);
}

uint32_t CompactProtocolReader::readSetEnd() {
  return 0;
}

uint32_t CompactProtocolReader::readBool(bool& value) {
  if (boolValue_.hasBoolValue == true) {
    value = boolValue_.boolValue;
    boolValue_.hasBoolValue = false;
    return 0;
  } else {
    int8_t val;
    readByte(val);
    value = (val == detail::compact::CT_BOOLEAN_TRUE);
    return 1;
  }
}

uint32_t CompactProtocolReader::readBool(std::vector<bool>::reference value) {
  bool ret = false;
  uint32_t sz = readBool(ret);
  value = ret;
  return sz;
}

uint32_t CompactProtocolReader::readByte(int8_t& byte) {
  byte = in_.read<int8_t>();
  return 1;
}

uint32_t CompactProtocolReader::readI16(int16_t& i16) {
  int32_t value;
  uint32_t rsize = apache::thrift::util::readVarint(in_, value);
  i16 = (int16_t)apache::thrift::util::zigzagToI32(value);
  return rsize;
}

uint32_t CompactProtocolReader::readI32(int32_t& i32) {
  int32_t value;
  uint32_t rsize = apache::thrift::util::readVarint(in_, value);
  i32 = apache::thrift::util::zigzagToI32(value);
  return rsize;
}

uint32_t CompactProtocolReader::readI64(int64_t& i64) {
  uint64_t value;
  uint32_t rsize = apache::thrift::util::readVarint(in_, value);
  i64 = apache::thrift::util::zigzagToI64(value);
  return rsize;
}

uint32_t CompactProtocolReader::readDouble(double& dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint64_t bits = in_.readBE<int64_t>();
  dub = bitwise_cast<double>(bits);
  return 8;
}

uint32_t CompactProtocolReader::readFloat(float& flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = in_.readBE<int32_t>();
  flt = bitwise_cast<float>(bits);
  return 4;
}

uint32_t CompactProtocolReader::readStringSize(int32_t& size) {
  uint32_t rsize = apache::thrift::util::readVarint(in_, size);

  // Catch error cases
  if (size < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  }
  if (string_limit_ > 0 && size > string_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }

  return rsize;
}

template<typename StrType>
uint32_t CompactProtocolReader::readStringBody(StrType& str, int32_t size) {
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
  return (uint32_t)size;
}

template<typename StrType>
uint32_t CompactProtocolReader::readString(StrType& str) {
  int32_t size = 0;
  uint32_t rsize = readStringSize(size);

  return rsize + readStringBody(str, size);
}

template <class StrType>
uint32_t CompactProtocolReader::readBinary(StrType& str) {
  return readString(str);
}

uint32_t CompactProtocolReader::readBinary(std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    str = folly::make_unique<folly::IOBuf>();
  }
  return readBinary(*str);
}

uint32_t CompactProtocolReader::readBinary(folly::IOBuf& str) {
  int32_t size = 0;
  uint32_t rsize = readStringSize(size);

  in_.clone(str, size);
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    str.makeManaged();
  }
  return rsize + (uint32_t) size;
}

TType CompactProtocolReader::getType(int8_t type) {
  using detail::compact::CTypeToTType;
  if (LIKELY(static_cast<uint8_t>(type) <
             sizeof(CTypeToTType)/sizeof(*CTypeToTType))) {
    return CTypeToTType[type];
  }
  throw TProtocolException("don't know what type: " + std::to_string(type));
}

}} // apache2::thrift

#endif // #ifndef THRIFT2_PROTOCOL_COMPACTPROTOCOL_TCC_
