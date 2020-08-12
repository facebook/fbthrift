/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#ifndef THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_
#define THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_ 1

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <limits>
#include <string>

namespace apache {
namespace thrift {

uint32_t BinaryProtocolWriter::writeMessageBegin(
    folly::StringPiece name,
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

uint32_t BinaryProtocolWriter::writeFieldBegin(
    const char* /*name*/,
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

uint32_t BinaryProtocolWriter::writeMapBegin(
    const TType keyType,
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

uint32_t BinaryProtocolWriter::writeListBegin(TType elemType, uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeListEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeSetBegin(TType elemType, uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

uint32_t BinaryProtocolWriter::writeSetEnd() {
  return 0;
}

uint32_t BinaryProtocolWriter::writeBool(bool value) {
  uint8_t tmp = value ? 1 : 0;
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
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint64_t bits = folly::bit_cast<uint64_t>(dub);
  out_.writeBE(bits);
  return sizeof(bits);
}

uint32_t BinaryProtocolWriter::writeFloat(float flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = folly::bit_cast<uint32_t>(flt);
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
  uint32_t size = folly::to_narrow(v.size());
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

uint32_t BinaryProtocolWriter::writeBinary(const folly::IOBuf& str) {
  size_t size = str.computeChainDataLength();
  // leave room for size
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    TProtocolException::throwExceededSizeLimit();
  }
  uint32_t result = writeI32((int32_t)size);
  if (sharing_ != SHARE_EXTERNAL_BUFFER && !str.isManaged()) {
    auto clone = str.clone();
    clone->makeManaged();
    out_.insert(std::move(clone));
  } else {
    out_.insert(str);
  }
  return result + static_cast<uint32_t>(size);
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
  return folly::to_narrow(buf->computeChainDataLength());
}

/**
 * Functions that return the serialized size
 */

uint32_t BinaryProtocolWriter::serializedMessageSize(
    folly::StringPiece name) const {
  // I32{version} + String{name} + I32{seqid}
  return 2 * serializedSizeI32() + serializedSizeString(name);
}

uint32_t BinaryProtocolWriter::serializedFieldSize(
    const char* /*name*/,
    TType /*fieldType*/,
    int16_t /*fieldId*/) const {
  // byte + I16
  return serializedSizeByte() + serializedSizeI16();
}

uint32_t BinaryProtocolWriter::serializedStructSize(const char* /*name*/
                                                    ) const {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeMapBegin(
    TType /*keyType*/,
    TType /*valType*/,
    uint32_t /*size*/) const {
  return serializedSizeByte() + serializedSizeByte() + serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeMapEnd() const {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeListBegin(
    TType /*elemType*/,
    uint32_t /*size*/
    ) const {
  return serializedSizeByte() + serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeListEnd() const {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeSetBegin(
    TType /*elemType*/,
    uint32_t /*size*/) const {
  return serializedSizeByte() + serializedSizeI32();
}

uint32_t BinaryProtocolWriter::serializedSizeSetEnd() const {
  return 0;
}

uint32_t BinaryProtocolWriter::serializedSizeStop() const {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeBool(bool /*val*/) const {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeByte(int8_t /*val*/) const {
  return 1;
}

uint32_t BinaryProtocolWriter::serializedSizeI16(int16_t /*val*/) const {
  return 2;
}

uint32_t BinaryProtocolWriter::serializedSizeI32(int32_t /*val*/) const {
  return 4;
}

uint32_t BinaryProtocolWriter::serializedSizeI64(int64_t /*val*/) const {
  return 8;
}

uint32_t BinaryProtocolWriter::serializedSizeDouble(double /*val*/) const {
  return 8;
}

uint32_t BinaryProtocolWriter::serializedSizeFloat(float /*val*/) const {
  return 4;
}

uint32_t BinaryProtocolWriter::serializedSizeString(
    folly::StringPiece str) const {
  return serializedSizeBinary(str);
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    folly::StringPiece str) const {
  return serializedSizeBinary(folly::ByteRange(str));
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    folly::ByteRange str) const {
  // I32{length of string} + binary{string contents}
  return serializedSizeI32() + static_cast<uint32_t>(str.size());
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    std::unique_ptr<folly::IOBuf> const& v) const {
  return v ? serializedSizeBinary(*v) : 0;
}

uint32_t BinaryProtocolWriter::serializedSizeBinary(
    folly::IOBuf const& v) const {
  size_t size = v.computeChainDataLength();
  if (size > std::numeric_limits<uint32_t>::max() - serializedSizeI32()) {
    TProtocolException::throwExceededSizeLimit();
  }
  return serializedSizeI32() + static_cast<uint32_t>(size);
}

uint32_t BinaryProtocolWriter::serializedSizeZCBinary(
    folly::StringPiece str) const {
  return serializedSizeZCBinary(folly::ByteRange(str));
}
uint32_t BinaryProtocolWriter::serializedSizeZCBinary(
    folly::ByteRange v) const {
  return serializedSizeBinary(v);
}
uint32_t BinaryProtocolWriter::serializedSizeZCBinary(
    std::unique_ptr<folly::IOBuf> const& v) const {
  return v ? serializedSizeZCBinary(*v) : 0;
}

uint32_t BinaryProtocolWriter::serializedSizeZCBinary(
    folly::IOBuf const& v) const {
  size_t size = v.computeChainDataLength();
  return (size > folly::IOBufQueue::kMaxPackCopy)
      ? serializedSizeI32() // too big to pack: size only
      : static_cast<uint32_t>(size) + serializedSizeI32(); // size + packed data
}

uint32_t BinaryProtocolWriter::serializedSizeSerializedData(
    std::unique_ptr<IOBuf> const& /*buf*/) const {
  // writeSerializedData's implementation just chains IOBufs together. Thus
  // we don't expect external buffer space for it.
  return 0;
}

/**
 * Reading functions
 */

void BinaryProtocolReader::readMessageBegin(
    std::string& name,
    MessageType& messageType,
    int32_t& seqid) {
  int32_t sz;
  readI32(sz);

  if (sz < 0) {
    // Check for correct version number
    int32_t version = sz & VERSION_MASK;
    if (version != VERSION_1) {
      throwBadVersionIdentifier(sz);
    }
    messageType = (MessageType)(sz & 0x000000ff);
    readString(name);
    readI32(seqid);
  } else {
    if (this->strict_read_) {
      throwMissingVersionIdentifier(sz);
    } else {
      // Handle pre-versioned input
      int8_t type;
      readStringBody(name, sz);
      readByte(type);
      messageType = (MessageType)type;
      readI32(seqid);
    }
  }
}

void BinaryProtocolReader::readMessageEnd() {}

void BinaryProtocolReader::readStructBegin(std::string& name) {
  name = "";
}

void BinaryProtocolReader::readStructEnd() {}

void BinaryProtocolReader::readFieldBegin(
    std::string& /*name*/,
    TType& fieldType,
    int16_t& fieldId) {
  int8_t type;
  readByte(type);
  fieldType = (TType)type;
  if (fieldType == TType::T_STOP) {
    fieldId = 0;
    return;
  }
  readI16(fieldId);
}

void BinaryProtocolReader::readFieldEnd() {}

void BinaryProtocolReader::readMapBegin(
    TType& keyType,
    TType& valType,
    uint32_t& size) {
  int8_t k, v;
  int32_t sizei;
  readByte(k);
  keyType = (TType)k;
  readByte(v);
  valType = (TType)v;
  readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
}

void BinaryProtocolReader::readMapEnd() {}

void BinaryProtocolReader::readListBegin(TType& elemType, uint32_t& size) {
  int8_t e;
  int32_t sizei;
  readByte(e);
  elemType = (TType)e;
  readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
}

void BinaryProtocolReader::readListEnd() {}

void BinaryProtocolReader::readSetBegin(TType& elemType, uint32_t& size) {
  int8_t e;
  int32_t sizei;
  readByte(e);
  elemType = (TType)e;
  readI32(sizei);
  checkContainerSize(sizei);
  size = (uint32_t)sizei;
}

void BinaryProtocolReader::readSetEnd() {}

void BinaryProtocolReader::readBool(bool& value) {
  auto byte = in_.read<uint8_t>();
  if (byte >= 2) {
    TProtocolException::throwBoolValueOutOfRange(byte);
  }
  value = static_cast<bool>(byte);
}

void BinaryProtocolReader::readBool(std::vector<bool>::reference value) {
  bool ret = false;
  readBool(ret);
  value = ret;
}

void BinaryProtocolReader::readByte(int8_t& byte) {
  byte = in_.read<int8_t>();
}

void BinaryProtocolReader::readI16(int16_t& i16) {
  i16 = in_.readBE<int16_t>();
}

void BinaryProtocolReader::readI32(int32_t& i32) {
  i32 = in_.readBE<int32_t>();
}

void BinaryProtocolReader::readI64(int64_t& i64) {
  i64 = in_.readBE<int64_t>();
}

void BinaryProtocolReader::readDouble(double& dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint64_t bits = in_.readBE<int64_t>();
  dub = folly::bit_cast<double>(bits);
}

void BinaryProtocolReader::readFloat(float& flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint32_t bits = in_.readBE<int32_t>();
  flt = folly::bit_cast<float>(bits);
}

void BinaryProtocolReader::checkStringSize(int32_t size) {
  // Catch error cases
  if (size < 0) {
    TProtocolException::throwNegativeSize();
  }
  if (string_limit_ > 0 && size > string_limit_) {
    TProtocolException::throwExceededSizeLimit();
  }
}

void BinaryProtocolReader::checkContainerSize(int32_t size) {
  if (size < 0) {
    TProtocolException::throwNegativeSize();
  } else if (this->container_limit_ && size > this->container_limit_) {
    TProtocolException::throwExceededSizeLimit();
  }
}

template <typename StrType>
void BinaryProtocolReader::readString(StrType& str) {
  int32_t size;
  readI32(size);
  readStringBody(str, size);
}

template <typename StrType>
void BinaryProtocolReader::readBinary(StrType& str) {
  readString(str);
}

void BinaryProtocolReader::readBinary(std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    str = std::make_unique<folly::IOBuf>();
  }
  readBinary(*str);
}

void BinaryProtocolReader::readBinary(folly::IOBuf& str) {
  int32_t size;
  readI32(size);
  checkStringSize(size);

  in_.clone(str, size);
  if (sharing_ != SHARE_EXTERNAL_BUFFER) {
    str.makeManaged();
  }
}

template <typename StrType>
void BinaryProtocolReader::readStringBody(StrType& str, int32_t size) {
  checkStringSize(size);

  // Catch empty string case
  if (size == 0) {
    str.clear();
    return;
  }

  if (static_cast<int32_t>(in_.length()) < size) {
    if (!in_.canAdvance(size)) {
      protocol::TProtocolException::throwTruncatedData();
    }
    str.reserve(size); // only reserve for multi iter case below
  }
  str.clear();
  size_t size_left = size;
  while (size_left > 0) {
    auto data = in_.peekBytes();
    auto data_avail = std::min(data.size(), size_left);
    if (data.empty()) {
      TProtocolException::throwExceededSizeLimit();
    }

    str.append((const char*)data.data(), data_avail);
    size_left -= data_avail;
    in_.skipNoAdvance(data_avail);
  }
}

uint32_t BinaryProtocolReader::readFromPositionAndAppend(
    Cursor& snapshot,
    std::unique_ptr<IOBuf>& ser) {
  int32_t size =
      folly::to_narrow(folly::to_signed(folly::io::Cursor(in_) - snapshot));

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

  return (uint32_t)size;
}

bool BinaryProtocolReader::advanceToNextField(
    int16_t nextFieldId,
    TType nextFieldType,
    StructReadState& state) {
  if (nextFieldType == TType::T_STOP) {
    if (in_.length() && *in_.data() == TType::T_STOP) {
      in_.skipNoAdvance(1);
      return true;
    }
  } else {
    if (in_.length() >= 3) {
      uint8_t type = *in_.data();
      if (nextFieldType == type) {
        int16_t fieldId =
            folly::Endian::big(folly::loadUnaligned<int16_t>(in_.data() + 1));
        in_.skipNoAdvance(3);
        if (nextFieldId == fieldId) {
          return true;
        }
        state.fieldType = (TType)type;
        state.fieldId = fieldId;
        return false;
      }

      state.fieldType = (TType)type;
      if (type != TType::T_STOP) {
        state.fieldId =
            folly::Endian::big(folly::loadUnaligned<int16_t>(in_.data() + 1));
        in_.skipNoAdvance(3);
      } else {
        in_.skipNoAdvance(1);
      }
      return false;
    }
  }
  state.readFieldBeginNoInline(this);
  return false;
}

void BinaryProtocolReader::readFieldBeginWithState(StructReadState& state) {
  int8_t type;
  readByte(type);
  state.fieldType = (TType)type;
  if (state.fieldType == TType::T_STOP) {
    return;
  }
  readI16(state.fieldId);
}
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT2_PROTOCOL_TBINARYPROTOCOL_TCC_
