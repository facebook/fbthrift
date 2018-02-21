/*
 * Copyright 2004-present Facebook, Inc.
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

#ifndef THRIFT2_PROTOCOL_TSIMPLEJSONPROTOCOL_TCC_
#define THRIFT2_PROTOCOL_TSIMPLEJSONPROTOCOL_TCC_ 1

#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>

#include <limits>
#include <string>
#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/json.h>

namespace apache { namespace thrift {

/*
 * Public writing methods
 */
uint32_t SimpleJSONProtocolWriter::writeStructBegin(const char* /*name*/) {
  auto ret = writeContext();
  return ret + beginContext(ContextType::MAP);
}

uint32_t SimpleJSONProtocolWriter::writeStructEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolWriter::writeFieldBegin(const char* name,
                                               TType /*fieldType*/,
                                               int16_t /*fieldId*/) {
  auto ret = writeContext();
  return ret + writeJSONString(name);
}

uint32_t SimpleJSONProtocolWriter::writeFieldEnd() {
  return 0;
}

uint32_t SimpleJSONProtocolWriter::writeFieldStop() {
  return 0;
}

uint32_t SimpleJSONProtocolWriter::writeMapBegin(const TType /*keyType*/,
                                             TType /*valType*/,
                                             uint32_t /*size*/) {
  auto ret = writeContext();
  return ret + beginContext(ContextType::MAP);
}

uint32_t SimpleJSONProtocolWriter::writeMapEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolWriter::writeListBegin(TType /*elemType*/,
                                              uint32_t /*size*/) {
  auto ret = writeContext();
  return ret + beginContext(ContextType::ARRAY);
}

uint32_t SimpleJSONProtocolWriter::writeListEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolWriter::writeSetBegin(TType /*elemType*/,
                                             uint32_t /*size*/) {
  auto ret = writeContext();
  return ret + beginContext(ContextType::ARRAY);
}

uint32_t SimpleJSONProtocolWriter::writeSetEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolWriter::writeBool(bool value) {
  auto ret = writeContext();
  return ret + writeJSONBool(value);
}

/**
 * Functions that return the serialized size
 */

uint32_t SimpleJSONProtocolWriter::serializedMessageSize(
    const std::string& name) const {
  return 2 // list begin and end
    + serializedSizeI32() * 3
    + serializedSizeString(name);
}

uint32_t SimpleJSONProtocolWriter::serializedFieldSize(const char* name,
                                                   TType /*fieldType*/,
                                                   int16_t /*fieldId*/) const {
  // string plus ":"
  return strlen(name) * 6 + 3;
}

uint32_t SimpleJSONProtocolWriter::serializedStructSize(
  const char* /*name*/) const {
  return 2; // braces
}

uint32_t SimpleJSONProtocolWriter::serializedSizeMapBegin(
  TType /*keyType*/,
  TType /*valType*/,
  uint32_t /*size*/) const
{
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeMapEnd() const {
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeListBegin(
  TType /*elemType*/,
  uint32_t /*size*/) const
{
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeListEnd() const {
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeSetBegin(
  TType /*elemType*/,
  uint32_t /*size*/) const
{
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeSetEnd() const {
  return 1;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeStop() const {
  return 0;
}

uint32_t SimpleJSONProtocolWriter::serializedSizeBool(bool /*val*/) const {
  return 6;
}

/**
 * Protected reading functions
 */

/**
 * Public reading functions
 */

uint32_t SimpleJSONProtocolReader::readStructBegin(std::string& /*name*/) {
  return ensureAndBeginContext(ContextType::MAP);
}

uint32_t SimpleJSONProtocolReader::readStructEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolReader::readFieldBegin(std::string& name,
                                              TType& fieldType,
                                              int16_t& fieldId) {
  if (!peekMap()) {
    fieldType = TType::T_STOP;
    fieldId = 0;
    return 0;
  }
  fieldId = std::numeric_limits<int16_t>::min();
  fieldType = TType::T_VOID;
  auto ret = readString(name);
  ensureAndSkipContext();
  skipWhitespace();
  auto peek = *in_.peek().first;
  if (peek == 'n') {
    bool tmp;
    ret += ensureAndReadContext(tmp);
    ret += readWhitespace();
    ret += readJSONNull();
    ret += readFieldBegin(name, fieldType, fieldId);
  }
  return ret;
}

uint32_t SimpleJSONProtocolReader::readFieldEnd() {
  return 0;
}

uint32_t SimpleJSONProtocolReader::readMapBegin(TType& /*keyType*/,
                                            TType& /*valType*/,
                                            uint32_t& size) {
  size = std::numeric_limits<uint32_t>::max();
  return ensureAndBeginContext(ContextType::MAP);
}

uint32_t SimpleJSONProtocolReader::readMapEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolReader::readListBegin(TType& /*elemType*/,
                                             uint32_t& size) {
  size = std::numeric_limits<uint32_t>::max();
  return ensureAndBeginContext(ContextType::ARRAY);
}

uint32_t SimpleJSONProtocolReader::readListEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolReader::readSetBegin(TType& /*elemType*/,
                                            uint32_t& size) {
  size = std::numeric_limits<uint32_t>::max();
  return ensureAndBeginContext(ContextType::ARRAY);
}

uint32_t SimpleJSONProtocolReader::readSetEnd() {
  return endContext();
}

uint32_t SimpleJSONProtocolReader::readBool(bool& value) {
  return readInContext<bool>(value);
}

uint32_t SimpleJSONProtocolReader::readBool(
    std::vector<bool>::reference value) {
  bool tmp = false;
  auto ret = readInContext<bool>(tmp);
  value = tmp;
  return ret;
}

bool SimpleJSONProtocolReader::peekMap() {
  skipWhitespace();
  return *in_.peek().first != detail::json::kJSONObjectEnd;
}

bool SimpleJSONProtocolReader::peekList() {
  skipWhitespace();
  return *in_.peek().first != detail::json::kJSONArrayEnd;
}

bool SimpleJSONProtocolReader::peekSet() {
  return peekList();
}

}} // apache2::thrift

#endif // #ifndef THRIFT2_PROTOCOL_TSIMPLEJSONPROTOCOL_TCC_
