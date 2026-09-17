/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/util/DebugString.h>

#include <fmt/core.h>
#include <folly/String.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache::thrift {
namespace {

using apache::thrift::protocol::TProtocolException;
using apache::thrift::protocol::TType;

const char* typeLabel(TType fieldType) {
  switch (fieldType) {
    case TType::T_STOP:
      return "stop";
    case TType::T_BOOL:
      return "bool";
    case TType::T_BYTE:
      return "byte";
    case TType::T_I16:
      return "i16";
    case TType::T_I32:
      return "i32";
    case TType::T_I64:
      return "i64";
    case TType::T_DOUBLE:
      return "double";
    case TType::T_FLOAT:
      return "float";
    case TType::T_STRING:
      return "string";
    case TType::T_UTF8:
      return "utf8";
    case TType::T_STRUCT:
      return "struct";
    case TType::T_LIST:
      return "list";
    case TType::T_SET:
      return "set";
    case TType::T_MAP:
      return "map";
    default:
      break;
  }
  return "";
}

std::string lookupTypeStringIfEmpty(std::string typeStr, TType type) {
  return typeStr.empty() ? typeLabel(type) : typeStr;
}

struct RenderState {
  DebugStringParams params;
  std::string indentStr;

  void incIndent() {
    if (!params.oneLine) {
      indentStr.append(params.indentAmount, ' ');
    }
  }
  void decIndent() {
    if (!params.oneLine) {
      assert(indentStr.size() >= params.indentAmount);
      indentStr.resize(indentStr.size() - params.indentAmount);
    }
  }
  void wrapLine(std::string* outVal) const {
    if (params.oneLine) {
      *outVal += ' ';
    } else {
      *outVal += fmt::format("\n{}", indentStr);
    }
  }
};

template <class ProtocolReader>
void toDebugStringForType(
    TType fieldType,
    ProtocolReader* inProtoReader,
    RenderState& rs,
    std::string* outType,
    std::string* outVal) {
  switch (fieldType) {
    case TType::T_BOOL: {
      bool val;
      inProtoReader->readBool(val);
      *outType = "bool";
      *outVal = val ? "true" : "false";
      break;
    }
    case TType::T_BYTE: {
      int8_t val;
      inProtoReader->readByte(val);
      *outType = "byte";
      *outVal = fmt::format("{}", val);
      break;
    }
    case TType::T_I16: {
      int16_t val;
      inProtoReader->readI16(val);
      *outType = "i16";
      *outVal = fmt::format("{}", val);
      break;
    }
    case TType::T_I32: {
      int32_t val;
      inProtoReader->readI32(val);
      *outType = "i32";
      *outVal = fmt::format("{}", val);
      break;
    }
    case TType::T_I64: {
      int64_t val;
      inProtoReader->readI64(val);
      *outType = "i64";
      *outVal = fmt::format("{}", val);
      break;
    }
    case TType::T_DOUBLE: {
      double val;
      inProtoReader->readDouble(val);
      *outType = "double";
      *outVal = fmt::format("{}", val);
      return;
    }
    case TType::T_FLOAT: {
      float val;
      inProtoReader->readFloat(val);
      *outType = "float";
      *outVal = fmt::format("{}", val);
      return;
    }
    case TType::T_STRING:
    case TType::T_UTF8: {
      std::string val, valOut;
      inProtoReader->readString(val);
      folly::humanify(val, valOut);
      *outType = "string";
      *outVal = fmt::format("\"{}\"", valOut);
      break;
    }
    case TType::T_STRUCT: {
      std::string nameIgnored;
      inProtoReader->readStructBegin(nameIgnored);
      *outVal = "{";
      rs.incIndent();
      TType elemType;
      int16_t fieldId;
      std::string valStr;
      for (;;) {
        inProtoReader->readFieldBegin(nameIgnored, elemType, fieldId);
        if (elemType == TType::T_STOP) {
          break;
        }
        toDebugStringForType(elemType, inProtoReader, rs, outType, &valStr);
        rs.wrapLine(outVal);
        *outVal += fmt::format("{}: {} = {}", fieldId, *outType, valStr);
        inProtoReader->readFieldEnd();
      }
      inProtoReader->readStructEnd();
      rs.decIndent();
      rs.wrapLine(outVal);
      *outVal += '}';
      *outType = "struct";
      break;
    }
    case TType::T_SET:
    case TType::T_LIST: {
      TType elemTypeT;
      uint32_t size = 0;
      if (fieldType == TType::T_LIST) {
        inProtoReader->readListBegin(elemTypeT, size);
        *outVal = "[";
      } else {
        inProtoReader->readSetBegin(elemTypeT, size);
        *outVal = "{";
      }
      rs.incIndent();
      std::string valStr, valTypeS;
      std::string* valTypeP = outType ? &valTypeS : nullptr;

      constexpr int kMaxLineTarget = 80;
      int lineLen = kMaxLineTarget;
      for (uint32_t num = 0; num < size; ++num) {
        toDebugStringForType(elemTypeT, inProtoReader, rs, valTypeP, &valStr);
        if (!rs.params.oneLine && lineLen + valStr.size() >= kMaxLineTarget) {
          rs.wrapLine(outVal);
          *outVal += fmt::format("{},", valStr);
          lineLen = rs.indentStr.size() + valStr.size();
        } else {
          *outVal += fmt::format(" {},", valStr);
          lineLen += valStr.size() + strlen(" ,");
        }
      }
      const char* thisType;
      const char* finalChar;
      if (fieldType == TType::T_LIST) {
        inProtoReader->readListEnd();
        thisType = "list";
        finalChar = "]";
      } else {
        inProtoReader->readSetEnd();
        thisType = "set";
        finalChar = "}";
      }
      rs.decIndent();
      rs.wrapLine(outVal);
      *outVal += finalChar;
      *outType = fmt::format(
          "{}<{}>", thisType, lookupTypeStringIfEmpty(valTypeS, elemTypeT));
      break;
    }
    case TType::T_MAP: {
      TType keyTypeT, valTypeT;
      uint32_t size = 0;
      inProtoReader->readMapBegin(keyTypeT, valTypeT, size);
      *outVal = "{";
      rs.incIndent();
      std::string keyStr, valStr, keyTypeS, valTypeS;
      std::string* keyTypeP = outType ? &keyTypeS : nullptr;
      std::string* valTypeP = outType ? &valTypeS : nullptr;

      for (uint32_t num = 0; num < size; ++num) {
        toDebugStringForType(keyTypeT, inProtoReader, rs, keyTypeP, &keyStr);
        toDebugStringForType(valTypeT, inProtoReader, rs, valTypeP, &valStr);
        rs.wrapLine(outVal);
        *outVal += fmt::format("{} : {},", keyStr, valStr);
      }
      inProtoReader->readMapEnd();
      rs.decIndent();
      rs.wrapLine(outVal);
      *outVal += "}";
      *outType = fmt::format(
          "map<{}, {}>",
          lookupTypeStringIfEmpty(keyTypeS, keyTypeT),
          lookupTypeStringIfEmpty(valTypeS, valTypeT));
      break;
    }
    default:
      TProtocolException::throwInvalidSkipType(fieldType);
      break;
  }
}
} // namespace

template <ThriftProtocolReader ProtocolReader>
std::string toDebugString(
    ProtocolReader& inProtoReader, DebugStringParams params) {
  RenderState rs;
  rs.params = params;
  std::string outType, outVal;
  toDebugStringForType(
      protocol::TType::T_STRUCT, &inProtoReader, rs, &outType, &outVal);
  return fmt::format("{} {}", outType, outVal);
}

// Explicit instantiations.
// Currently, we instantiate Compact and Binary, but we could instantiate
// others if it makes sense.
template std::string toDebugString<CompactProtocolReader>(
    CompactProtocolReader& inProtoReader, DebugStringParams p);
template std::string toDebugString<BinaryProtocolReader>(
    BinaryProtocolReader& inProtoReader, DebugStringParams p);

} // namespace apache::thrift
