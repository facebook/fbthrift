/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
namespace java.swift test.fixtures.basic_swift_bean

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
  DOMAIN = 2,
}

struct MyStruct1 {
  1: i64 MyIntField,
  2: string MyStringField,
  # use the type before it is defined. Thrift should be able to handle this
  3: MyDataItem MyDataField,
  # glibc has macros with this name, Thrift should be able to prevent collisions
  4: i64 major (cpp.name = "majorVer"),
}

struct MyStruct2 {
  1: MyStruct1 myStruct1,
  2: string myString,
} (java.swift.mutable = "true")

struct MyDataItem {
  1: i32 field1,
  2: i32 field2,
} (java.swift.mutable = "true")

struct LegacyStruct {
  1: i32 normal,
  -1: i32 bad,
} (java.swift.mutable = "true")

const MyStruct1 ms = {
  "MyIntField": 42,
  "MyStringField": "Meaning_of_life",
  "MyDataField": {
    "field1": 1,
    "field2": 2,
  },
  "major": 32,
}

service legacy_service {
  map<string, list<i32>> getPoints(1: set<string> key, -1: i64 legacyStuff);
}
