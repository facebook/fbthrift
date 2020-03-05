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

namespace java com.facebook.thrift.java.test
namespace java.swift com.facebook.thrift.javaswift.test
namespace android com.facebook.thrift.android.test

typedef map<i32, i64>
  (java.swift.type = "it.unimi.dsi.fastutil.ints.Int2LongArrayMap") FMap


struct MySimpleStruct {
  1: i64 id,
  2: string name,
}

struct MySensitiveStruct {
  1: i64 id,
  2: string password (java.sensitive),
}

union MySimpleUnion {
  1: i64 caseOne,
  2: i64 caseTwo,
  3: string caseThree,
  4: MySimpleStruct caseFour,
  5: list<string> caseFive,
}

struct NestedStruct {
  1: map<i32, string> myMap,
  2: MySimpleStruct mySimpleStruct,
  3: set<i32> mySet,
  4: list<string> myList,
  5: MySimpleUnion myUnion,
}

struct ComplexNestedStruct {
  1: set<set<i32>> setOfSetOfInt,
  2: list<list<SmallEnum>> listOfListOfEnum,
  3: list<list<MySimpleStruct>> listOfListOfMyStruct,
  4: set<list<list<string>>> setOfListOfListOfString,
  5: map<i32,list<list<SmallEnum>>> mapKeyIntValListOfListOfEnum,
  6: map<map<i32,string>,map<string,string>> mapKeyMapValMap,
  7: MySimpleUnion myUnion,
}

struct SimpleStructTypes  {
  1: string msg,
  2: bool b,
  3: byte y,
  4: i16 i,
  5: i32 j,
  6: i64 k,
  7: double d,
}

struct SimpleCollectionStruct {
   1: list<double> lDouble,
   2: list<i16> lShort,
   3: map<i32,string> mIntegerString,
   4: map<string,string> mStringString,
   5: set<i64> sLong,
}

enum SmallEnum {
  UNKNOWN = 0, // default value
  RED = 1,
  BLUE = 2,
  GREEN = 3,
}

enum BigEnum {
  ONE = 1,
  TWO = 2,
  THREE = 3,
  FOUR = 4,
  FIVE = 5,
  SIX = 6,
  SEVEN = 7,
  EIGHT = 8,
  NINE = 9,
  TEN = 10,
  ELEVEN = 11,
  TWELVE = 12,
  THIRTEEN = 13,
  FOURTEEN = 14,
  FIFTEEN = 15,
  SIXTEEN = 16,
  SEVENTEEN = 17,
  EIGHTEEN = 18,
  NINETEEN = 19,
  TWENTY = 20,
}

struct MyListStruct {
  1: list<i64> ids
}

struct MySetStruct {
  1: set<i64> ids
}

struct MyMapStruct {
  1: map<i64, string> mapping
}

struct MyStringStruct {
  1: string aLongString
}

typedef i64 PersonID

struct MyOptioalStruct {
  1: PersonID id;
  2: string name;
  3: optional i16 ageShortOptional;
  4: i16 ageShort;
  5: optional i64 ageLongOptional;
  6: i64 ageLong;
  7: optional MySimpleStruct mySimpleStructOptional;
  8: MySimpleStruct mySimpleStruct;
  9: optional map<i32, string> mIntegerStringOptional;
  10: map<i32, string> mIntegerString;
  11: optional SmallEnum smallEnumOptional;
  12: SmallEnum mySmallEnum;
}

struct TypeRemapped {
  1: map<i64, string>
  (java.swift.type =
  "it.unimi.dsi.fastutil.longs.Long2ObjectArrayMap<String>") lsMap,
  2: map<i32, FMap> (
    java.swift.type =
    "it.unimi.dsi.fastutil.ints.Int2ObjectArrayMap<it.unimi.dsi.fastutil.ints.Int2LongArrayMap>")
    ioMap,
  4: binary (java.swift.type = "java.nio.ByteBuffer") byteBufferForBinary,
  5: binary  byteArrayForBinary,
  6: list<FMap> myListOfFMaps,
}
