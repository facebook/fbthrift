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

#pragma once

#include <utility>
#include <thrift/conformance/cpp2/internal/Object.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::conformance {

// Creates a Value struct for the given value.
//
// TT: The thrift type to use, for example
// apache::thrift::conformance::type::binary_t.
template <typename TT, typename T>
Value asValueStruct(T&& value) {
  Value result;
  detail::ValueHelper<TT>::set(result, std::forward<T>(value));
  return result;
}

// Schemaless deserialization of thrift serialized data of specified
// thrift type into conformance::Value
// Protocol: protocol to use eg. apache::thrift::BinaryProtocolReader
// TODO: handle jsonprotocol
template <class Protocol>
Value parseValue(Protocol& prot, TType arg_type) {
  Value result;
  switch (arg_type) {
    case protocol::T_BOOL: {
      bool boolv;
      prot.readBool(boolv);
      result.boolValue_ref() = boolv;
      return result;
    }
    case protocol::T_BYTE: {
      int8_t bytev = 0;
      prot.readByte(bytev);
      result.byteValue_ref() = bytev;
      return result;
    }
    case protocol::T_I16: {
      int16_t i16;
      prot.readI16(i16);
      result.i16Value_ref() = i16;
      return result;
    }
    case protocol::T_I32: {
      int32_t i32;
      prot.readI32(i32);
      result.i32Value_ref() = i32;
      return result;
    }
    case protocol::T_I64: {
      int64_t i64;
      prot.readI64(i64);
      result.i64Value_ref() = i64;
      return result;
    }
    case protocol::T_DOUBLE: {
      double dub;
      prot.readDouble(dub);
      result.doubleValue_ref() = dub;
      return result;
    }
    case protocol::T_FLOAT: {
      float flt;
      prot.readFloat(flt);
      result.floatValue_ref() = flt;
      return result;
    }
    case protocol::T_STRING: {
      std::string str;
      prot.readBinary(str);
      result.stringValue_ref() = str;
      return result;
    }
    case protocol::T_STRUCT: {
      std::string name;
      int16_t fid;
      TType ftype;
      result.objectValue_ref() = Object();
      prot.readStructBegin(name);
      while (true) {
        prot.readFieldBegin(name, ftype, fid);
        if (ftype == protocol::T_STOP) {
          break;
        }
        auto val = parseValue(prot, ftype);
        result.objectValue_ref()->members_ref()->emplace(fid, val);
        prot.readFieldEnd();
      }
      prot.readStructEnd();
      return result;
    }
    case protocol::T_MAP: {
      TType keyType;
      TType valType;
      uint32_t size;
      result.mapValue_ref() = std::map<Value, Value>();
      prot.readMapBegin(keyType, valType, size);
      for (uint32_t i = 0; i < size; i++) {
        auto key = parseValue(prot, keyType);
        auto val = parseValue(prot, valType);
        result.mapValue_ref()->emplace(key, val);
      }
      prot.readMapEnd();
      return result;
    }
    case protocol::T_SET: {
      TType elemType;
      uint32_t size;
      result.setValue_ref() = std::set<Value>();
      prot.readSetBegin(elemType, size);
      for (uint32_t i = 0; i < size; i++) {
        auto val = parseValue(prot, elemType);
        result.setValue_ref()->emplace(val);
      }
      prot.readSetEnd();
      return result;
    }
    case protocol::T_LIST: {
      TType elemType;
      uint32_t size;
      prot.readListBegin(elemType, size);
      result.listValue_ref() = std::vector<Value>();
      for (uint32_t i = 0; i < size; i++) {
        auto val = parseValue(prot, elemType);
        result.listValue_ref()->emplace_back(val);
      }
      prot.readListEnd();
      return result;
    }
    default: {
      TProtocolException::throwInvalidSkipType(arg_type);
    }
  }
}

// Schemaless deserialization of thrift serialized data
// into conformance::Object
// Protocol: protocol to use eg. apache::thrift::BinaryProtocolReader
// buf: serialized payload
template <class Protocol>
Object parseObject(const folly::IOBuf& buf) {
  Protocol prot;
  prot.setInput(&buf);
  auto result = parseValue(prot, protocol::T_STRUCT);
  return *result.objectValue_ref();
}
} // namespace apache::thrift::conformance
