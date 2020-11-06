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
namespace py thrift.conformance.thrift_type_info
namespace py.asyncio thrift_asyncio.conformance.thrift_type_info
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance
namespace go thrift.conformance.thrift_type_info

// The minimum number of bytes that can be used to identify a type.
//
// The expected number of names that can be hashed before a
// collision is 2^(8*minTypeHashBytes/2).
const byte minTypeHashBytes = 16;

// The hash algorithms that can be used with type names.
enum TypeHashAlgorithm {
  Sha2_256 = 1,
}

// Language-independent type information.
struct ThriftTypeInfo {
  // The name of the type.
  1: string name;

  // The other names for this type.
  2: set<string> aliases;

  // The default number of bytes to use in a type hash.
  //
  // 0 indicates a type hash should never be used.
  // unset indicates that the implemnation should decided.
  3: optional byte typeHashBytes;
}
