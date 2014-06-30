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

#include <thrift/lib/cpp/protocol/TPhpSerializeProtocol.h>

#include <boost/lexical_cast.hpp>


// A glorified alias.
template <typename T>
std::string tostring(T val) {
  return boost::lexical_cast<std::string>(val);
}

using apache::thrift::transport::TMemoryBuffer;


namespace apache { namespace thrift { namespace protocol {

uint32_t TPhpSerializeProtocol::write(const char* buf, uint32_t len) {
  if (!structBufferStack_.empty()) {
    structBufferStack_.top()->write((uint8_t*)buf, len);
  } else {
    trans_->write((uint8_t*)buf, len);
  }
  return len;
}

uint32_t TPhpSerializeProtocol::write3(
    const char* v1, const char* v2, const char* v3) {
  uint32_t ret = 0;
  ret += write(v1, strlen(v1));
  ret += write(v2, strlen(v2));
  ret += write(v3, strlen(v3));
  return ret;
}


uint32_t TPhpSerializeProtocol::writeMessageBegin(
    const std::string& name,
    const TMessageType messageType,
    const int32_t seqid) {
  throw TProtocolException(TProtocolException::NOT_IMPLEMENTED);
}

uint32_t TPhpSerializeProtocol::writeMessageEnd() {
  throw TProtocolException(TProtocolException::NOT_IMPLEMENTED);
}

uint32_t TPhpSerializeProtocol::writeStructBegin(const char* name) {
  uint32_t ret = 0;
  ret += listKey();
  ret += doWriteString(name, true);
  structSizeStack_.push(0);
  structBufferStack_.push(std::shared_ptr<TMemoryBuffer>(new TMemoryBuffer()));
  listPosStack_.push(-1);
  return ret;
}

uint32_t TPhpSerializeProtocol::writeStructEnd() {
  int size = structSizeStack_.top();
  std::shared_ptr<TMemoryBuffer> buf = structBufferStack_.top();
  uint8_t* bufptr;
  uint32_t bufsize;
  buf->getBuffer(&bufptr, &bufsize);
  structSizeStack_.pop();
  structBufferStack_.pop();
  listPosStack_.pop();

  uint32_t ret = 0;
  ret += write3(":", tostring(size).c_str(), ":{");
  (void) write((const char*)bufptr, bufsize); // Size already counted.
  ret += write("}", 1);
  return ret;
}

uint32_t TPhpSerializeProtocol::writeFieldBegin(
    const char* name, const TType fieldType, const int16_t fieldId) {
  structSizeStack_.top() += 1;
  return doWriteString(name, false);
}

uint32_t TPhpSerializeProtocol::writeFieldEnd() {
  return 0;
}

uint32_t TPhpSerializeProtocol::writeFieldStop() {
  return 0;
}

/**
 * listPosStack_ maintains the position of elements within an aggregating unit
 * - For structs, listPosStack_.top() is always -1
 * - For lists, listPosStack_.top() is the number of list elements
 *   written so far
 * - For maps, listPosStack_.top() is also negative and < -1
 *   Moreover, abs(listPosStack_.top() + 2) is the number of keys and values
 *   written so far.
 */
uint32_t TPhpSerializeProtocol::listKey() {
  if (listPosStack_.empty()) {
    return 0;
  }
  if (listPosStack_.top() >= 0) {
    return doWriteInt(listPosStack_.top()++);
  } else if (listPosStack_.top() != -1) {
    listPosStack_.top()--;
  }
  return 0;
}

uint32_t TPhpSerializeProtocol::doWriteListBegin(uint32_t size, bool is_map) {
  uint32_t ret = 0;
  ret += listKey();
  ret += write3("a:", tostring(size).c_str(), ":{");
  listPosStack_.push(is_map ? -2 : 0);
  return ret;
}

uint32_t TPhpSerializeProtocol::writeListBegin(
    const TType elemType, const uint32_t size) {
  return doWriteListBegin(size, false);
}

uint32_t TPhpSerializeProtocol::writeListEnd() {
  listPosStack_.pop();
  return write("}", 1);
}

uint32_t TPhpSerializeProtocol::writeMapBegin(
    const TType keyType, const TType valType, const uint32_t size) {
  // TODO: Check key type for safety?
  return doWriteListBegin(size, true);
}

uint32_t TPhpSerializeProtocol::writeMapEnd() {
  return writeListEnd();
}

uint32_t TPhpSerializeProtocol::writeSetBegin(
    const TType elemType, const uint32_t size) {
  throw TProtocolException(TProtocolException::NOT_IMPLEMENTED);
}

uint32_t TPhpSerializeProtocol::writeSetEnd() {
  throw TProtocolException(TProtocolException::NOT_IMPLEMENTED);
}

uint32_t TPhpSerializeProtocol::writeBool(const bool value) {
  uint32_t ret = 0;
  ret += listKey();
  ret += write3("b:", (value ? "1" : "0"), ";");
  return ret;
}

uint32_t TPhpSerializeProtocol::writeByte(const int8_t byte) {
  return writeI64(byte);
}

uint32_t TPhpSerializeProtocol::writeI16(const int16_t i16) {
  return writeI64(i16);
}

uint32_t TPhpSerializeProtocol::writeI32(const int32_t i32) {
  return writeI64(i32);
}

uint32_t TPhpSerializeProtocol::writeI64(const int64_t i64) {
  uint32_t ret = 0;
  ret += listKey();
  ret += doWriteInt(i64);
  return ret;
}

uint32_t TPhpSerializeProtocol::doWriteInt(const int64_t val) {
  return doWriteInt(tostring(val));
}

uint32_t TPhpSerializeProtocol::doWriteInt(const std::string& val) {
  return write3("i:", val.c_str(), ";");
}

uint32_t TPhpSerializeProtocol::writeDouble(const double dub) {
  uint32_t ret = 0;
  ret += listKey();
  ret += write3("d:", tostring(dub).c_str(), ";");
  return ret;
}

// Map keys are to be treated as numeric if they can be cast to a
// decimal number with no leading 0s (except if it is actually 0)
bool isNumericMapKey(const std::string& key) {
  try {
    int64_t result = boost::lexical_cast<int64_t>(key);
    return (key[0] != '0' || 0 == result);
  } catch (const boost::bad_lexical_cast& e) {
    return false;
  }
}


uint32_t TPhpSerializeProtocol::writeString(const std::string& str) {
  uint32_t ret = 0;
  ret += listKey();

  if (!listPosStack_.empty() &&
      listPosStack_.top() < -1 &&
      listPosStack_.top() % 2 &&
      isNumericMapKey(str)) {
    ret += doWriteInt(str);
    return ret;
  }

  ret += doWriteString(str, false);
  return ret;
}

uint32_t TPhpSerializeProtocol::writeBinary(const std::string& str) {
  return TPhpSerializeProtocol::writeString(str);
}

uint32_t TPhpSerializeProtocol::doWriteString(
    const std::string& str, bool is_class) {
  uint32_t ret = 0;
  ret += write3((is_class ? "O:" : "s:"), tostring(str.size()).c_str(), ":\"");
  ret += write(str.data(), str.size());
  ret += write("\"", 1);
  if (!is_class) {
    ret += write(";", 1);
  }
  return ret;
}

}}} // apache::thrift::protocol
