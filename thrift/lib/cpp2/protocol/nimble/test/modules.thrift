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

struct BasicTypes {
  1: byte myByte;
  2: i16 myInt16,
  3: i32 myInt32,
  4: i64 myInt64,
  5: bool myBool,
  6: byte myUint8 (cpp2.type = "uint8_t"),
  7: i16 myUint16 (cpp2.type = "uint16_t"),
  8: i32 myUint32 (cpp2.type = "uint32_t"),
  9: i64 myUint64 (cpp2.type = "uint64_t"),
  10: float myFloat,
  11: double myDouble,
}

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBuf

struct StringTypes {
  1: string myStr,
  2: binary myBinary,
  3: IOBuf myIOBuf,
}

struct ContainerTypes {
  1: map<i32, i32> myIntMap,
  2: map<string, string> myStringMap,
  3: map<i32, string> myMap,
  4: list<i32> myIntList,
  5: list<string> myStringList,
  6: list<list<double>> myListOfList,
  7: set<i16> myI16Set,
  8: set<string> myStringSet,
  9: list<map<i32, string>> myListOfMap,
}

struct StructOfStruct {
  1: BasicTypes basicTypes,
  2: StringTypes strTypes,
  3: ContainerTypes containerTypes,
  4: string myStr,
  5: map<string, i64> myMap,
}

union SimpleUnion {
  1: string simpleStr,
  2: i32 simpleI32,
}
