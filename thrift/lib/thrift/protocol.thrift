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

// TODO(ytj): merge this file into thrift/lib/thrift/type.thrift

include "thrift/lib/thrift/standard.thrift"

package "facebook.com/thrift/protocol"

namespace cpp2 apache.thrift.protocol
namespace py3 apache.thrift.protocol
namespace php apache_thrift_protocol
namespace java com.facebook.thrift.protocol
namespace java.swift com.facebook.thrift.protocol_swift
namespace py.asyncio apache_thrift_asyncio.protocol
namespace go thrift.lib.thrift.protocol
namespace py thrift.lib.thrift.protocol

// A dynamic struct/union/exception
struct Object {
  // The type of the object, if applicable.
  1: standard.Uri type;

  // The members of the object.
  // TODO(ytj): use schema.FieldId as key
  2: map<i16, Value> members;
}

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

  9: standard.ByteBuffer binaryValue;

  // A dynamic object value.
  11: Object objectValue;

  // Containers of values.
  14: list<Value> listValue;
  15: set<Value> setValue;
  16: map<Value, Value> mapValue;
}
