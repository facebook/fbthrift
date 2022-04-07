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

include "thrift/conformance/if/any.thrift"
cpp_include "<folly/io/IOBuf.h>"

namespace cpp2 apache.thrift.conformance
namespace php apache_thrift
namespace py thrift.conformance.object
namespace py.asyncio thrift_asyncio.conformance.object
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance
namespace java2 org.apache.thrift.conformance
namespace go thrift.conformance.object

// A dynamic struct/union/exception (e.g. a JSON Object).
struct Object {
  // The type of the object, if applicable.
  1: string type;

  // The members of the object.
  2: map<i16, Value> members;
} (thrift.uri = "facebook.com/thrift/Object")

// A dynamic value.
union Value {
  // Integers.
  1: bool boolValue;
  2: byte byteValue;
  3: i16 i16Value;
  4: i32 i32Value;
  5: i64 i64Value;

  // Floats.
  6: float floatValue;
  7: double doubleValue;

  // Strings.
  8: string stringValue;
  9: binary (cpp.type = "folly::IOBuf") binaryValue;

  // A dynamic object value.
  11: Object objectValue;

  // Containers of values.
  14: list<Value> listValue;
  15: set<Value> setValue;
  16: map<Value, Value> mapValue;

  // A static object value.
  17: any.Any anyValue;
} (thrift.uri = "facebook.com/thrift/Value")
