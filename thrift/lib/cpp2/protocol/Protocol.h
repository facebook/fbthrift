/*
 * Copyright 2011-present Facebook, Inc.
 *
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

#include <sys/types.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/util/BitwiseCast.h>
#include <thrift/lib/cpp2/CloneableIOBuf.h>

/**
 * Protocol Readers and Writers are ducktyped in cpp2.
 * This means they have no base class or virtual methods,
 * and you must explicitly choose the correct class to use.
 *
 * This results in a 2x performance increase over a traditional
 * object oriented hierarchy. If you need a base class, you can
 * use the VirtualReader / VitualWriter in VirtualProtocol.h
 */

namespace apache {
namespace thrift {

/**
 * Certain serialization / deserialization operations allow sharing
 * external (user-owned) buffers. This means that the external buffers must
 * remain allocated (and unchanged) until the serialization / deserialization
 * is complete.
 *
 * This is often counter-intuitive; for example, deserializing from a string
 * wouldn't work with a temporary string if sharing is allowed. So sharing
 * external buffers must be requested explicitly.
 *
 * Note that we always share memory that is under IOBuf's control (that is,
 * IOBuf chains for which isManaged() is true). To prevent that, call unshare()
 * on the IOBuf chain as appropriate.
 */
enum ExternalBufferSharing {
  COPY_EXTERNAL_BUFFER,
  SHARE_EXTERNAL_BUFFER,
};

using apache::thrift::protocol::TProtocolException;
using apache::thrift::protocol::TType;
typedef apache::thrift::protocol::PROTOCOL_TYPES ProtocolType;

/*
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
enum MessageType { T_CALL = 1, T_REPLY = 2, T_EXCEPTION = 3, T_ONEWAY = 4 };

/**
 * Helper template for implementing Protocol::skip().
 *
 * Templatized to avoid having to make virtual function calls.
 */
template <class Protocol_>
void skip(Protocol_& prot, TType arg_type) {
  switch (arg_type) {
    case TType::T_BOOL: {
      bool boolv;
      prot.readBool(boolv);
      return;
    }
    case TType::T_BYTE: {
      int8_t bytev;
      prot.readByte(bytev);
      return;
    }
    case TType::T_I16: {
      int16_t i16;
      prot.readI16(i16);
      return;
    }
    case TType::T_I32: {
      int32_t i32;
      prot.readI32(i32);
      return;
    }
    case TType::T_I64: {
      int64_t i64;
      prot.readI64(i64);
      return;
    }
    case TType::T_DOUBLE: {
      double dub;
      prot.readDouble(dub);
      return;
    }
    case TType::T_FLOAT: {
      float flt;
      prot.readFloat(flt);
      return;
    }
    case TType::T_STRING: {
      std::string str;
      prot.readBinary(str);
      return;
    }
    case TType::T_STRUCT: {
      std::string name;
      int16_t fid;
      TType ftype;
      prot.readStructBegin(name);
      while (true) {
        prot.readFieldBegin(name, ftype, fid);
        if (ftype == TType::T_STOP) {
          break;
        }
        apache::thrift::skip(prot, ftype);
        prot.readFieldEnd();
      }
      prot.readStructEnd();
      return;
    }
    case TType::T_MAP: {
      TType keyType;
      TType valType;
      uint32_t i, size;
      prot.readMapBegin(keyType, valType, size);
      for (i = 0; i < size; i++) {
        apache::thrift::skip(prot, keyType);
        apache::thrift::skip(prot, valType);
      }
      prot.readMapEnd();
      return;
    }
    case TType::T_SET: {
      TType elemType;
      uint32_t i, size;
      prot.readSetBegin(elemType, size);
      for (i = 0; i < size; i++) {
        apache::thrift::skip(prot, elemType);
      }
      prot.readSetEnd();
      return;
    }
    case TType::T_LIST: {
      TType elemType;
      uint32_t i, size;
      prot.readListBegin(elemType, size);
      for (i = 0; i < size; i++) {
        apache::thrift::skip(prot, elemType);
      }
      prot.readListEnd();
      return;
    }
    default:
      return;
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
  // Use with string literals only!
  static folly::IOBuf fromStringLiteral(const char* str) {
    return folly::IOBuf::wrapBufferAsValue(str, strlen(str));
  }

  static bool isEmpty(const folly::IOBuf& str) {
    return str.empty();
  }

  static bool isEqual(const folly::IOBuf& lhs, const folly::IOBuf& rhs) {
    auto llink = &lhs;
    auto rlink = &rhs;
    while (llink != nullptr && rlink != nullptr) {
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
    return (
        str[0] != '\0' ? folly::IOBuf::wrapBuffer(str, strlen(str)) : nullptr);
  }

  static bool isEmpty(const std::unique_ptr<folly::IOBuf>& str) {
    return !str || str->empty();
  }

  static bool isEqual(
      const std::unique_ptr<folly::IOBuf>& lhs,
      const std::unique_ptr<folly::IOBuf>& rhs) {
    return StringTraits<folly::IOBuf>::isEqual(*lhs, *rhs);
  }
};

} // namespace thrift
} // namespace apache

#endif
