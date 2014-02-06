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


#include "thrift/lib/cpp2/protocol/DebugProtocol.h"

#include "folly/Conv.h"
#include "folly/String.h"

namespace apache { namespace thrift {

DebugProtocolWriter::DebugProtocolWriter() : out_(nullptr) { }

namespace {

std::string fieldTypeName(TType type) {
  switch (type) {
    case TType::T_STOP   : return "stop"   ;
    case TType::T_VOID   : return "void"   ;
    case TType::T_BOOL   : return "bool"   ;
    case TType::T_BYTE   : return "byte"   ;
    case TType::T_I16    : return "i16"    ;
    case TType::T_I32    : return "i32"    ;
    case TType::T_U64    : return "u64"    ;
    case TType::T_I64    : return "i64"    ;
    case TType::T_DOUBLE : return "double" ;
    case TType::T_FLOAT  : return "float"  ;
    case TType::T_STRING : return "string" ;
    case TType::T_STRUCT : return "struct" ;
    case TType::T_MAP    : return "map"    ;
    case TType::T_SET    : return "set"    ;
    case TType::T_LIST   : return "list"   ;
    case TType::T_UTF8   : return "utf8"   ;
    case TType::T_UTF16  : return "utf16"  ;
    case TType::T_STREAM : return "stream" ;
    default: return folly::format("unknown({})", int(type)).str();
  }
}

const int kIndent = 2;

}  // namespace

void DebugProtocolWriter::indentUp() {
  indent_.append(kIndent, ' ');
}

void DebugProtocolWriter::indentDown() {
  CHECK_GE(indent_.size(), kIndent);
  indent_.erase(indent_.size() - kIndent);
}

void DebugProtocolWriter::pushState(ItemType t) {
  indentUp();
  writeState_.push_back(t);
}

void DebugProtocolWriter::popState() {
  CHECK(!writeState_.empty());
  writeState_.pop_back();
  indentDown();
}

void DebugProtocolWriter::startItem() {
  if (writeState_.empty()) {  // top level
    return;
  }
  auto& ws = writeState_.back();
  switch (ws.type) {
  case STRUCT:
    break;
  case SET:
  case MAP_KEY:
    writeIndent();
    break;
  case MAP_VALUE:
    writePlain(" -> ");
    break;
  case LIST:
    writeIndented("[{}] = ", ws.index);
    break;
  }
}

void DebugProtocolWriter::endItem() {
  if (writeState_.empty()) {  // top level
    return;
  }
  auto& ws = writeState_.back();
  ++ws.index;
  switch (ws.type) {
  case LIST:
  case STRUCT:
  case SET:
    writePlain(",\n");
    break;
  case MAP_KEY:
    ws.type = MAP_VALUE;
    break;
  case MAP_VALUE:
    ws.type = MAP_KEY;
    writePlain(",\n");
  }
}

void DebugProtocolWriter::setOutput(folly::IOBufQueue* out, size_t maxGrowth) {
  out_ = out;
}

uint32_t DebugProtocolWriter::writeMessageBegin(const std::string& name,
                                                MessageType messageType,
                                                int32_t seqid) {
  std::string mtype;
  switch (messageType) {
  case T_CALL:      mtype = "call";   break;
  case T_REPLY:     mtype = "reply";  break;
  case T_EXCEPTION: mtype = "exn";    break;
  case T_ONEWAY:    mtype = "oneway"; break;
  }

  writeIndented("({}) {}(", mtype, name);
  indentUp();
  return 0;
}

uint32_t DebugProtocolWriter::writeMessageEnd() {
  indentDown();
  writeIndented(")\n");
  return 0;
}

uint32_t DebugProtocolWriter::writeStructBegin(const char* name) {
  startItem();
  writePlain("{} {{\n", name);
  pushState(STRUCT);
  return 0;
}

uint32_t DebugProtocolWriter::writeStructEnd() {
  popState();
  writeIndented("}}");
  endItem();
  return 0;
}

uint32_t DebugProtocolWriter::writeFieldBegin(const char* name,
                                              TType fieldType,
                                              int16_t fieldId) {
  writeIndented("{:0d}: {} ({}) = ", fieldId, name, fieldTypeName(fieldType));
  return 0;
}

uint32_t DebugProtocolWriter::writeFieldEnd() { return 0; }
uint32_t DebugProtocolWriter::writeFieldStop() { return 0; }

uint32_t DebugProtocolWriter::writeMapBegin(TType keyType,
                                            TType valueType,
                                            uint32_t size) {
  startItem();
  writePlain("map<{},{}>[{}] {{\n",
             fieldTypeName(keyType),
             fieldTypeName(valueType),
             size);
  pushState(MAP_KEY);
  return 0;
}

uint32_t DebugProtocolWriter::writeMapEnd() {
  popState();
  writeIndented("}}");
  endItem();
  return 0;
}

uint32_t DebugProtocolWriter::writeListBegin(TType elemType, uint32_t size) {
  startItem();
  writePlain("list<{}>[{}] {{\n", fieldTypeName(elemType), size);
  pushState(LIST);
  return 0;
}

uint32_t DebugProtocolWriter::writeListEnd() {
  popState();
  writeIndented("}}");
  endItem();
  return 0;
}

uint32_t DebugProtocolWriter::writeSetBegin(TType elemType, uint32_t size) {
  startItem();
  writePlain("set<{}>[{}] {{\n", fieldTypeName(elemType), size);
  pushState(SET);
  return 0;
}

uint32_t DebugProtocolWriter::writeSetEnd() {
  popState();
  writeIndented("}}");
  endItem();
  return 0;
}

uint32_t DebugProtocolWriter::writeBool(bool v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeByte(int8_t v) {
  writeItem("0x{:x}", uint8_t(v));
  return 0;
}

uint32_t DebugProtocolWriter::writeI16(int16_t v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeI32(int32_t v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeI64(int64_t v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeFloat(float v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeDouble(double v) {
  writeItem("{}", v);
  return 0;
}

uint32_t DebugProtocolWriter::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  writeSP(folly::StringPiece(str->clone()->coalesce()));
  return 0;
}

uint32_t DebugProtocolWriter::writeBinary(
    const folly::IOBuf& str) {
  writeSP(folly::StringPiece(str.clone()->coalesce()));
  return 0;
}

void DebugProtocolWriter::writeSP(folly::StringPiece str) {
  static constexpr size_t kStringLimit = 256;
  static constexpr size_t kStringPrefixSize = 128;

  std::string toShow = str.str();
  if (toShow.length() > kStringLimit) {
    toShow = str.subpiece(0, kStringPrefixSize).str();
    folly::toAppend("[...](", str.size(), ")", &toShow);
  }

  writeItem("\"{}\"", folly::cEscape<std::string>(toShow));
}

}}  // namespaces
