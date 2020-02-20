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

struct MySimpleStruct {
  1: i64 id,
  2: string name,
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
