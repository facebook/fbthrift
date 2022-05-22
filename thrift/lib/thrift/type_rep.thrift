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

include "thrift/annotation/thrift.thrift"
include "thrift/annotation/java.thrift"

cpp_include "<folly/io/IOBuf.h>"
cpp_include "<folly/FBString.h>"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java com.facebook.thrift.type
namespace java.swift com.facebook.thrift.type_swift
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.type_rep
namespace go thrift.lib.thrift.type_rep
namespace py thrift.lib.thrift.type_rep

// Typedef for binary data which can be represented as a string of 8-bit bytes
//
// Each language can map this type into a customized memory efficient object
@thrift.Experimental
@java.Adapter{
  adapterClassName = "com.facebook.thrift.adapter.common.UnpooledByteBufTypeAdapter",
  typeClassName = "io.netty.buffer.ByteBuf",
}
typedef binary (cpp2.type = "folly::fbstring") ByteString

// Typedef for binary data
//
// Each language can map this type into a customized memory efficient object
// May be used for zero-copy slice of data
@thrift.Experimental
@java.Adapter{
  adapterClassName = "com.facebook.thrift.adapter.common.UnpooledByteBufTypeAdapter",
  typeClassName = "io.netty.buffer.ByteBuf",
}
typedef binary (cpp2.type = "folly::IOBuf") ByteBuffer

// Standard protocols.
@thrift.Experimental
enum StandardProtocol {
  Custom = 0,

  // Standard protocols.
  Binary = 1,
  Compact = 2,

  // Deprecated protocols.
  Json = 3,
  SimpleJson = 4,
}

// A union representation of a protocol.
@thrift.Experimental
union ProtocolUnion {
  1: StandardProtocol standard;
  2: string custom;
} (thrift.uri = "facebook.com/thrift/type/Protocol")

// TODO(afuller): Allow 'void' type for union fields.
@thrift.Experimental
enum Void {
  NoValue = 0,
}

// A (scheme-less) URI.
//
// See rfc3986.
// TODO(afuller): Adapt.
@thrift.Experimental
typedef string Uri

// The uri of an IDL defined type.
@thrift.Experimental
union TypeNameUnion {
  // The unique Thrift URI for this type.
  1: Uri uri;
  // A prefix of the SHA2-256 hash of the URI.
  2: ByteString typeHashPrefixSha2_256;
}

// Uniquely identifies a type.
@thrift.Experimental
union TypeId {
  1: Void boolType;
  2: Void byteType;
  3: Void i16Type;
  4: Void i32Type;
  5: Void i64Type;
  6: Void floatType;
  7: Void doubleType;
  8: Void stringType;
  9: Void binaryType;
  10: TypeNameUnion enumType;
  11: TypeNameUnion structType;
  12: TypeNameUnion unionType;
  13: TypeNameUnion exceptionType;
  14: Void listType;
  15: Void setType;
  16: Void mapType;
} (thrift.uri = "facebook.com/thrift/type/TypeId")

// A concrete type.
struct TypeStruct {
  // The type id.
  1: TypeId id;
  // The type params, if appropriate.
  2: list<TypeStruct> params;
} (thrift.uri = "facebook.com/thrift/type/Type")
