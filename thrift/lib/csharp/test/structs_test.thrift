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

package "test.dev/csharp/tests/structs"

namespace csharp FBThrift.Tests.Structs

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

struct MyDataItem {}

struct MyStruct {
  1: i64 MyIntField;
  2: string MyStringField;
  3: MyDataItem MyDataField;
  4: MyEnum myEnum;
  5: bool oneway;
  6: bool readonly;
  7: bool idempotent;
}

struct Containers {
  1: list<i32> I32List;
  2: set<string> StringSet;
  3: map<string, i64> StringToI64Map;
}

// Typedef-to-container types (regression test for D96628806:
// codegen must resolve through typedefs before accessing
// container element/key/value type properties).
typedef list<i32> IntList
typedef set<string> StringSet
typedef map<string, i64> StringToI64Map

struct TypedefContainerStruct {
  1: IntList int_list_field;
  2: StringSet string_set_field;
  3: StringToI64Map string_map_field;
}

struct OptionalFieldsStruct {
  1: optional string optional_string;
  2: optional MyDataItem optional_struct;
  3: optional list<i32> optional_list;
}

exception MyException {
  1: i64 MyIntField;
  2: string MyStringField;
}

exception MyExceptionWithMessage {
  1: i64 MyIntField;
  @thrift.ExceptionMessage
  2: string MyStringField;
}

exception MyExceptionWithOptionalMessage {
  @thrift.ExceptionMessage
  1: optional string errorMessage;
}
