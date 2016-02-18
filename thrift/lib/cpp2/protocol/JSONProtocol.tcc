/*
 * Copyright 2016 Facebook, Inc.
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

#include <folly/portability/Constexpr.h>

namespace apache { namespace thrift {

namespace detail { namespace json {

static constexpr folly::StringPiece kTypeNameBool("tf");
static constexpr folly::StringPiece kTypeNameByte("i8");
static constexpr folly::StringPiece kTypeNameI16("i16");
static constexpr folly::StringPiece kTypeNameI32("i32");
static constexpr folly::StringPiece kTypeNameI64("i64");
static constexpr folly::StringPiece kTypeNameDouble("dbl");
static constexpr folly::StringPiece kTypeNameFloat("flt");
static constexpr folly::StringPiece kTypeNameStruct("rec");
static constexpr folly::StringPiece kTypeNameString("str");
static constexpr folly::StringPiece kTypeNameMap("map");
static constexpr folly::StringPiece kTypeNameList("lst");
static constexpr folly::StringPiece kTypeNameSet("set");

static folly::StringPiece getTypeNameForTypeID(TType typeID) {
  using namespace apache::thrift::protocol;
  switch (typeID) {
  case TType::T_BOOL:
    return kTypeNameBool;
  case TType::T_BYTE:
    return kTypeNameByte;
  case TType::T_I16:
    return kTypeNameI16;
  case TType::T_I32:
    return kTypeNameI32;
  case TType::T_I64:
    return kTypeNameI64;
  case TType::T_DOUBLE:
    return kTypeNameDouble;
  case TType::T_FLOAT:
    return kTypeNameFloat;
  case TType::T_STRING:
    return kTypeNameString;
  case TType::T_STRUCT:
    return kTypeNameStruct;
  case TType::T_MAP:
    return kTypeNameMap;
  case TType::T_SET:
    return kTypeNameSet;
  case TType::T_LIST:
    return kTypeNameList;
  default:
    throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
                             "Unrecognized type");
  }
}

static protocol::TType getTypeIDForTypeName(folly::StringPiece name) {
  using namespace apache::thrift::protocol;
  auto fail = [] {
    throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
                             "Unrecognized type");
    return TType::T_STOP;
  };
  if (name.size() <= 1) {
    return fail();
  }
  switch (name[0]) {
  case 'd': return TType::T_DOUBLE;
  case 'f': return TType::T_FLOAT;
  case 'i':
    switch (name[1]) {
    case '8': return TType::T_BYTE;
    case '1': return TType::T_I16;
    case '3': return TType::T_I32;
    case '6': return TType::T_I64;
    default: return fail();
    }
  case 'l': return TType::T_LIST;
  case 'm': return TType::T_MAP;
  case 'r': return TType::T_STRUCT;
  case 's':
    switch (name[1]) {
    case 't': return TType::T_STRING;
    case 'e': return TType::T_SET;
    default: return fail();
    }
  case 't': return TType::T_BOOL;
  default: return fail();
  }
}

static uint32_t clampSize(int64_t size) {
  if (size < 0) {
    throw TProtocolException(
        TProtocolException::NEGATIVE_SIZE,
        folly::to<std::string>(size, " < 0"));
  }
  constexpr auto sizeMax = std::numeric_limits<uint32_t>::max();
  if (size > sizeMax) {
    throw TProtocolException(
        TProtocolException::SIZE_LIMIT,
        folly::to<std::string>(size, " is too large (", sizeMax, ")"));
  }
  return uint32_t(size);
}

}}

/*
 * Public writing methods
 */

uint32_t JSONProtocolWriter::writeStructBegin(const char* name) {
  auto ret = writeContext();
  ret += beginContext(ContextType::MAP);
  return ret;
}

uint32_t JSONProtocolWriter::writeStructEnd() {
  return endContext();
}

uint32_t JSONProtocolWriter::writeFieldBegin(const char* /*name*/,
                                             TType fieldType,
                                             int16_t fieldId) {
  uint32_t ret = 0;
  ret += writeString(folly::to<std::string>(fieldId));
  ret += writeContext();
  ret += beginContext(ContextType::MAP);
  ret += writeString(detail::json::getTypeNameForTypeID(fieldType).str());
  return ret;
}

uint32_t JSONProtocolWriter::writeFieldEnd() {
  return endContext();
}

uint32_t JSONProtocolWriter::writeFieldStop() {
  return 0;
}

uint32_t JSONProtocolWriter::writeMapBegin(TType keyType,
                                           TType valType,
                                           uint32_t size) {
  auto ret = writeContext();
  ret += beginContext(ContextType::ARRAY);
  ret += writeString(detail::json::getTypeNameForTypeID(keyType).str());
  ret += writeString(detail::json::getTypeNameForTypeID(valType).str());
  ret += writeI32(size);
  ret += writeContext();
  ret += beginContext(ContextType::MAP);
  return ret;
}

uint32_t JSONProtocolWriter::writeMapEnd() {
  auto ret = endContext();
  ret += endContext();
  return ret;
}

uint32_t JSONProtocolWriter::writeListBegin(TType elemType,
                                            uint32_t size) {
  auto ret = writeContext();
  ret += beginContext(ContextType::ARRAY);
  ret += writeString(detail::json::getTypeNameForTypeID(elemType).str());
  ret += writeI32(size);
  return ret;
}

uint32_t JSONProtocolWriter::writeListEnd() {
  return endContext();
}

uint32_t JSONProtocolWriter::writeSetBegin(TType elemType,
                                           uint32_t size) {
  auto ret = writeContext();
  ret += beginContext(ContextType::ARRAY);
  ret += writeString(detail::json::getTypeNameForTypeID(elemType).str());
  ret += writeI32(size);
  return ret;
}

uint32_t JSONProtocolWriter::writeSetEnd() {
  return endContext();
}

uint32_t JSONProtocolWriter::writeBool(bool value) {
  auto ret = writeContext();
  return ret + writeJSONInt(value);
}

/**
 * Functions that return the serialized size
 */

uint32_t JSONProtocolWriter::serializedMessageSize(
    const std::string& name) {
  return 2 // list begin and end
    + serializedSizeI32() * 3
    + serializedSizeString(name);
}

uint32_t JSONProtocolWriter::serializedFieldSize(const char* name,
                                                 TType /*fieldType*/,
                                                 int16_t /*fieldId*/) {
  // string plus ":"
  return folly::constexpr_strlen(R"(,"32767":{"typ":})");
}

uint32_t JSONProtocolWriter::serializedStructSize(const char* /*name*/) {
  return folly::constexpr_strlen(R"({})");
}

uint32_t JSONProtocolWriter::serializedSizeMapBegin(TType /*keyType*/,
                                                    TType /*valType*/,
                                                    uint32_t /*size*/) {
  return folly::constexpr_strlen(R"(["typ","typ",4294967295,{)");
}

uint32_t JSONProtocolWriter::serializedSizeMapEnd() {
  return folly::constexpr_strlen(R"(}])");
}

uint32_t JSONProtocolWriter::serializedSizeListBegin(TType /*elemType*/,
                                                           uint32_t /*size*/) {
  return folly::constexpr_strlen(R"(["typ",4294967295)");
}

uint32_t JSONProtocolWriter::serializedSizeListEnd() {
  return folly::constexpr_strlen(R"(])");
}

uint32_t JSONProtocolWriter::serializedSizeSetBegin(TType /*elemType*/,
                                                          uint32_t /*size*/) {
  return folly::constexpr_strlen(R"(["typ",4294967295)");
}

uint32_t JSONProtocolWriter::serializedSizeSetEnd() {
  return folly::constexpr_strlen(R"(])");
}

uint32_t JSONProtocolWriter::serializedSizeStop() {
  return 0;
}

uint32_t JSONProtocolWriter::serializedSizeBool(bool /*val*/) {
  return 2;
}

/**
 * Protected reading functions
 */

/**
 * Public reading functions
 */

uint32_t JSONProtocolReader::readStructBegin(std::string& name) {
  uint32_t ret = 0;
  ret += ensureAndBeginContext(ContextType::MAP);
  return ret;
}

uint32_t JSONProtocolReader::readStructEnd() {
  uint32_t ret = 0;
  ret += endContext();
  return ret;
}

uint32_t JSONProtocolReader::readFieldBegin(std::string& name,
                                            TType& fieldType,
                                            int16_t& fieldId) {
  skipWhitespace();
  auto peek = *in_.peek().first;
  if (peek == TJSONProtocol::kJSONObjectEnd) {
    fieldType = TType::T_STOP;
    fieldId = 0;
    return 0;
  }
  uint32_t ret = 0;
  ret += readI16(fieldId);
  ret += ensureAndBeginContext(ContextType::MAP);
  std::string fieldTypeS;
  ret += readString(fieldTypeS);
  fieldType = detail::json::getTypeIDForTypeName(fieldTypeS);
  return ret;
}

uint32_t JSONProtocolReader::readFieldEnd() {
  uint32_t ret = 0;
  ret += endContext();
  return ret;
}

uint32_t JSONProtocolReader::readMapBegin(TType& keyType,
                                          TType& valType,
                                          uint32_t& size) {
  uint32_t ret = 0;
  ret += ensureAndBeginContext(ContextType::ARRAY);
  std::string keyTypeS;
  ret += readString(keyTypeS);
  keyType = detail::json::getTypeIDForTypeName(keyTypeS);
  std::string valTypeS;
  ret += readString(valTypeS);
  valType = detail::json::getTypeIDForTypeName(valTypeS);
  int64_t sizeRead = 0;
  ret += readI64(sizeRead);
  size = detail::json::clampSize(sizeRead);
  ret += ensureAndBeginContext(ContextType::MAP);
  return ret;
}

uint32_t JSONProtocolReader::readMapEnd() {
  uint32_t ret = 0;
  ret += endContext();
  ret += endContext();
  return ret;
}

uint32_t JSONProtocolReader::readListBegin(TType& elemType,
                                           uint32_t& size) {
  uint32_t ret = 0;
  ret += ensureAndBeginContext(ContextType::ARRAY);
  std::string elemTypeS;
  ret += readString(elemTypeS);
  elemType = detail::json::getTypeIDForTypeName(elemTypeS);
  int64_t sizeRead = 0;
  ret += readI64(sizeRead);
  size = detail::json::clampSize(sizeRead);
  return ret;
}

uint32_t JSONProtocolReader::readListEnd() {
  uint32_t ret = 0;
  ret += endContext();
  return ret;
}

uint32_t JSONProtocolReader::readSetBegin(TType& elemType,
                                          uint32_t& size) {
  return readListBegin(elemType, size);
}

uint32_t JSONProtocolReader::readSetEnd() {
  return readListEnd();
}

uint32_t JSONProtocolReader::readBool(bool& value) {
  int8_t tmp = false;
  auto ret = readInContext<int8_t>(tmp);
  if (tmp < 0 || tmp > 1) {
    throw TProtocolException(
      TProtocolException::INVALID_DATA,
      folly::to<std::string>(tmp, " is not a valid bool"));
  }
  value = bool(tmp);
  return ret;
}

uint32_t JSONProtocolReader::readBool(
    std::vector<bool>::reference value) {
  bool tmp = false;
  auto ret = readBool(tmp);
  value = tmp;
  return ret;
}

bool JSONProtocolReader::peekMap() {
  return false;
}

bool JSONProtocolReader::peekSet() {
  return false;
}

bool JSONProtocolReader::peekList() {
  return false;
}

}}
