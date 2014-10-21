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

#ifndef CPP2_PROTOCOL_PROTOCOL_H_
#define CPP2_PROTOCOL_PROTOCOL_H_ 1

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/util/BitwiseCast.h>
#include <thrift/lib/cpp2/CloneableIOBuf.h>

#include <map>
#include <memory>
#include <sys/types.h>
#include <string>
#include <vector>

/**
 * Protocol Readers and Writers are ducktyped in cpp2.
 * This means they have no base class or virtual methods,
 * and you must explicitly choose the correct class to use.
 *
 * This results in a 2x performance increase over a traditional
 * object oriented hierarchy. If you need a base class, you can
 * use the VirtualReader / VitualWriter in VirtualProtocol.h
 */

namespace apache { namespace thrift {

using apache::thrift::protocol::TType;
using apache::thrift::protocol::TProtocolException;
typedef apache::thrift::protocol::PROTOCOL_TYPES ProtocolType;

/*
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
enum MessageType {
  T_CALL       = 1,
  T_REPLY      = 2,
  T_EXCEPTION  = 3,
  T_ONEWAY     = 4
};

/**
 * Helper template for implementing Protocol::skip().
 *
 * Templatized to avoid having to make virtual function calls.
 */
template <class Protocol_>
uint32_t skip(Protocol_& prot, TType arg_type) {
  switch (arg_type) {
    case TType::T_BOOL:
    {
      bool boolv;
      return prot.readBool(boolv);
    }
    case TType::T_BYTE:
    {
      int8_t bytev;
      return prot.readByte(bytev);
    }
    case TType::T_I16:
    {
      int16_t i16;
      return prot.readI16(i16);
    }
    case TType::T_I32:
    {
      int32_t i32;
      return prot.readI32(i32);
    }
    case TType::T_I64:
    {
      int64_t i64;
      return prot.readI64(i64);
    }
    case TType::T_DOUBLE:
    {
      double dub;
      return prot.readDouble(dub);
    }
    case TType::T_FLOAT:
    {
      float flt;
      return prot.readFloat(flt);
    }
    case TType::T_STRING:
    {
      std::string str;
      return prot.readBinary(str);
    }
    case TType::T_STRUCT:
    {
      uint32_t result = 0;
      std::string name;
      int16_t fid;
      TType ftype;
      result += prot.readStructBegin(name);
      while (true) {
        result += prot.readFieldBegin(name, ftype, fid);
        if (ftype == TType::T_STOP) {
          break;
        }
        result += apache::thrift::skip(prot, ftype);
        result += prot.readFieldEnd();
      }
      result += prot.readStructEnd();
      return result;
    }
    case TType::T_MAP:
    {
      uint32_t result = 0;
      TType keyType;
      TType valType;
      uint32_t i, size;
      result += prot.readMapBegin(keyType, valType, size);
      for (i = 0; i < size; i++) {
        result += apache::thrift::skip(prot, keyType);
        result += apache::thrift::skip(prot, valType);
      }
      result += prot.readMapEnd();
      return result;
    }
    case TType::T_SET:
    {
      uint32_t result = 0;
      TType elemType;
      uint32_t i, size;
      result += prot.readSetBegin(elemType, size);
      for (i = 0; i < size; i++) {
        result += apache::thrift::skip(prot, elemType);
      }
      result += prot.readSetEnd();
      return result;
    }
    case TType::T_LIST:
    {
      uint32_t result = 0;
      TType elemType;
      uint32_t i, size;
      result += prot.readListBegin(elemType, size);
      for (i = 0; i < size; i++) {
        result += apache::thrift::skip(prot, elemType);
      }
      result += prot.readListEnd();
      return result;
    }
    default:
      return 0;
  }
}

template <class StrType>
struct StringTraits {
  static StrType fromStringLiteral(const char* str) {
    return StrType(str);
  }

  static bool isEmpty(const StrType& str) {
    return str.empty();
  }

  static bool isEqual(const StrType& lhs, const StrType& rhs) {
    return lhs == rhs;
  }
};

template <>
struct StringTraits<folly::IOBuf> {
  static bool isEqual(const folly::IOBuf& lhs, const folly::IOBuf& rhs) {
    auto llink = &lhs;
    auto rlink = &rhs;
    while (llink != nullptr &&
           rlink != nullptr) {
      if (rlink->length() != llink->length() ||
        0 != memcmp(llink->data(), rlink->data(), llink->length())) {
        return false;
      }

      // Advance
      llink = llink->next();
      rlink = rlink->next();
      if (llink == &lhs) {
        break;
      }
    }

    return true;
  }
};

template <>
struct StringTraits<std::unique_ptr<folly::IOBuf>> {
  // Use with string literals only!
  static std::unique_ptr<folly::IOBuf> fromStringLiteral(const char* str) {
    return (str[0] != '\0' ?
            folly::IOBuf::wrapBuffer(str, strlen(str)) :
            nullptr);
  }

  static bool isEmpty(const std::unique_ptr<folly::IOBuf>& str) {
    return !str || str->empty();
  }

  static bool isEqual(const std::unique_ptr<folly::IOBuf>& lhs,
                         const std::unique_ptr<folly::IOBuf>& rhs) {
    return StringTraits<folly::IOBuf>::isEqual(*lhs, *rhs);
  }
};

}} // apache::thrift

#endif
