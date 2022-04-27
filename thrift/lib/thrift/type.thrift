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

cpp_include "<folly/io/IOBuf.h>"
cpp_include "<folly/FBString.h>"
cpp_include "thrift/lib/cpp2/type/BaseType.h"
cpp_include "thrift/lib/cpp2/type/UniversalHashAlgorithm.h"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java.swift com.facebook.thrift.type
namespace java com.facebook.thrift.type
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.type
namespace go thrift.lib.thrift.type
namespace py thrift.lib.thrift.type

// Typedef for binary data which can be represented as a string of 8-bit bytes
//
// Each language can map this type into a customized memory efficient object
@thrift.Experimental
typedef binary (cpp2.type = "folly::fbstring") ByteString

// Typedef for binary data
//
// Each language can map this type into a customized memory efficient object
// May be used for zero-copy slice of data
@thrift.Experimental
typedef binary (cpp2.type = "folly::IOBuf") ByteBuffer

// An enumeration of all base types in thrift.
//
// Base types are not parameterized. For example, the base
// type of map<int, string> is BaseType::Map and the base type of
// all thrift structs is BaseType::Struct.
//
// Similar to lib/cpp/protocol/TType.h, but IDL
// concepts instead of protocol concepts.
enum BaseType {
  Void = 0,

  // Integer types.
  Bool = 1,
  Byte = 2,
  I16 = 3,
  I32 = 4,
  I64 = 5,

  // Floating point types.
  Float = 6,
  Double = 7,

  // String types.
  String = 8,
  Binary = 9,

  // Enum type class.
  Enum = 10,

  // Structured type classes.
  Struct = 11,
  Union = 12,
  Exception = 13,

  // Container type classes.
  List = 14,
  Set = 15,
  Map = 16,
} (
  cpp.name = "BaseTypeEnum",
  cpp.adapter = "::apache::thrift::StaticCastAdapter<::apache::thrift::type::BaseType, ::apache::thrift::type::BaseTypeEnum>",
)

// The minimum and default number of bytes that can be used to identify
// a type.
//
// The expected number of types that can be hashed before a
// collision is 2^(8*{numBytes}/2).
// Which is ~4.3 billion types for the min, and ~18.45 quintillion
// types for the default.
const byte minTypeHashBytes = 8;
const byte defaultTypeHashBytes = 16;

// The hash algorithms that can be used with type names.
enum UniversalHashAlgorithm {
  Sha2_256 = 1,
} (
  cpp.name = "UniversalHashAlgorithmEnum",
  cpp.adapter = "::apache::thrift::StaticCastAdapter<::apache::thrift::type::UniversalHashAlgorithm, ::apache::thrift::type::UniversalHashAlgorithmEnum>",
)
