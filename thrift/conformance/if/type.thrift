/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

namespace cpp2 apache.thrift.conformance
namespace php apache_thrift
namespace py thrift.conformance.type
namespace py.asyncio thrift_asyncio.conformance.type
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance
namespace go thrift.conformance.type

// An enumeration of all base types in thrift.
//
// Base types are unparametarized. For example, the base
// type of map<int, string> is BaseType::Map and the base type of
// all thrift structs is BaseType::Struct.
//
// Similar to lib/cpp/protocol/TType.h, but IDL
// concepts instead of protocol concepts.
enum BaseType {
  Void = 0,

  // Integral primitive types.
  Bool = 1,
  Byte = 2,
  I16 = 3,
  I32 = 4,
  I64 = 5,

  // Floating point primitive types.
  Float = 6,
  Double = 7,

  // Enum type classes.
  Enum = 8,

  // String primitive types.
  String = 9,
  Binary = 10,

  // Structured type classes.
  Struct = 11,
  Union = 12,
  Exception = 13,

  // Container type classes.
  List = 14,
  Set = 15,
  Map = 16,
}

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
enum TypeHashAlgorithm {
  Sha2_256 = 1,
}

// Language-independent type information.
struct ThriftTypeInfo {
  // The URI of the type. For example: "facebook.com/thrift/ThriftTypeInfo"
  //
  // The scheme "fbthrift://" is implied, and should not be included.
  1: string uri;

  // The other URIs for this type.
  //
  // The primary URI can be safely changed using the following steps:
  // 1. Add the new URI to altUris.
  // 2. Update all binaries/libraries, so they know about the new
  // URI.
  // 3. Move the old URI to altUris, and the new URI to uri.
  //
  // At this point all references to the old URI will continue to work, and
  // all knew references will use the new URI.
  2: set<string> altUris;

  // The default number of bytes to use in a type hash.
  //
  // 0 indicates a type hash should never be used.
  // Unset indicates that the implementation should decide.
  3: optional byte typeHashBytes;
} (thrift.uri = "facebook.com/thrift/ThriftTypeInfo")
