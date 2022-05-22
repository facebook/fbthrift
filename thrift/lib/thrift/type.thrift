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

include "thrift/lib/thrift/type_rep.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

cpp_include "<thrift/lib/cpp2/type/BaseType.h>"
cpp_include "<thrift/lib/cpp2/type/Protocol.h>"
cpp_include "<thrift/lib/cpp2/type/Type.h>"
cpp_include "<thrift/lib/cpp2/type/UniversalHashAlgorithm.h>"

// Canonical representations for well-known Thrift types.
package "facebook.com/thrift/type"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java com.facebook.thrift.type
namespace java2 com.facebook.thrift.type
namespace java.swift com.facebook.thrift.type_swift
namespace py.asyncio apache_thrift_asyncio.type
namespace go thrift.lib.thrift.type
namespace py thrift.lib.thrift.type

// An enumeration of all base types in thrift.
//
// Base types are not parameterized. For example, the base
// type of map<int, string> is BaseType::Map and the base type of
// all thrift structs is BaseType::Struct.
//
// Similar to lib/cpp/protocol/TType.h, but IDL
// concepts instead of protocol concepts.
@thrift.Experimental
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

// The hash algorithms that can be used with type names.
@thrift.Experimental
enum UniversalHashAlgorithm {
  Sha2_256 = 2, // = getFieldId(TypeNameUnion::typeHashPrefixSha2_256).
} (
  cpp.name = "UniversalHashAlgorithmEnum",
  cpp.adapter = "::apache::thrift::StaticCastAdapter<::apache::thrift::type::UniversalHashAlgorithm, ::apache::thrift::type::UniversalHashAlgorithmEnum>",
)

@cpp.Adapter{
  name = "::apache::thrift::InlineAdapter<::apache::thrift::type::Protocol>",
}
@thrift.Experimental
typedef type_rep.ProtocolUnion Protocol (thrift.uri = "")

@cpp.Adapter{
  name = "::apache::thrift::InlineAdapter<::apache::thrift::type::Type>",
}
@thrift.Experimental
typedef type_rep.TypeStruct Type (thrift.uri = "")
