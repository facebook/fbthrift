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

namespace java.swift test.fixtures.complex_struct

typedef string stringTypedef
typedef i64 longTypeDef
typedef map<i16,string> mapTypedef
typedef list<double> listTypedef
typedef float floatTypedef
typedef map<i32, i64>
  (java.swift.type = "it.unimi.dsi.fastutil.ints.Int2LongArrayMap") FMap

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

struct MyStructFloatFieldThrowExp {
  1: i64 myLongField,
  2: byte MyByteField,
  3: string myStringField,
  4: float myFloatField,
}

struct MyStructMapFloatThrowExp {
  1: i64 myLongField,
  2: map<i32,list<list<floatTypedef>>> mapListOfFloats,
}
struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
  3: MyDataItem MyDataField,
  4: MyEnum myEnum,
  5: bool MyBoolField,
  6: byte MyByteField,
  7: i16 MyShortField,
  8: i64 MyLongField,
  9: double MyDoubleField,
  10: list<double> lDouble,
  11: list<i16> lShort,
  12: list<i32> lInteger,
  13: list<i64> lLong,
  14: list<string> lString,
  15: list<bool> lBool,
  16: list<byte> lByte,
  17: map<i16,string> mShortString,
  18: map<i32,string> mIntegerString,
  19: map<string,MyStruct> mStringMyStruct,
  20: map<string,bool> mStringBool,
  21: map<i32,i32> mIntegerInteger,
  22: map<i32,bool> mIntegerBool,
  23: set<i16> sShort,
  24: set<MyStruct> sMyStruct,
  25: set<i64> sLong,
  26: set<string> sString,
  27: set<byte> sByte,
}

struct MyStructTypeDef {
  1: i64 myLongField,
  2: longTypeDef  myLongTypeDef,
  3: string myStringField,
  4: stringTypedef myStringTypedef,
  5: map<i16,string> myMapField,
  6: mapTypedef myMapTypedef,
  7: list<double> myListField,
  8: listTypedef myListTypedef,
  9: map<i16,list<listTypedef>> myMapListOfTypeDef,
}

struct MyDataItem {}

union MyUnion {
  1: MyEnum myEnum,
  2: MyStruct myStruct,
  3: MyDataItem myDataItem,
  4: ComplexNestedStruct complexNestedStruct,
}

union MyUnionFloatFieldThrowExp {
  1: MyEnum myEnum,
  2: list<list<float>> setFloat,
  3: MyDataItem myDataItem,
  4: ComplexNestedStruct complexNestedStruct,
}
struct ComplexNestedStruct {
    // collections of collections
  1: set<set<i32>> setOfSetOfInt,
  2: list<list<list<list<MyEnum>>>> listofListOfListOfListOfEnum,
  3: list<list<MyStruct>> listOfListOfMyStruct,
  4: set<list<list<i64>>> setOfListOfListOfLong,
  5: set<set<set<i64>>> setOfSetOfsetOfLong,
  6: map<i32,list<list<MyStruct>>> mapStructListOfListOfLong,
  7: map<MyStruct,i32> mKeyStructValInt,
  8: list<map<i32, i32>> listOfMapKeyIntValInt
  9: list<map<string, list<MyStruct>>> listOfMapKeyStrValList,
    // Maps with collections as keys
  10: map<set<i32>,i64> mapKeySetValLong,
  11: map<list<string>,i32> mapKeyListValLong,
    // Map with collections as keys and values
  12: map<map<i32, string>, map<i32, string>> mapKeyMapValMap,
  13: map<set<list<i32>>, map<list<set<string>>, string>> mapKeySetValMap,
  14: map<map<map<i32, string>,string>, map<i32, string>> NestedMaps,
  15: map<i32,list<MyStruct>> mapKeyIntValList,
  16: map<i32,set<bool>> mapKeyIntValSet,
  17: map<set<bool>,MyEnum> mapKeySetValInt,
  18: map<list<i32>,set<map<double,string>>> mapKeyListValSet,
}

struct TypeRemapped {
  1: map<i64, string>
  (java.swift.type = "it.unimi.dsi.fastutil.longs.Long2ObjectArrayMap<String>")
  lsMap,
  2: map<i32, FMap> (
    java.swift.type =
    "it.unimi.dsi.fastutil.ints.Int2ObjectArrayMap<it.unimi.dsi.fastutil.ints.Int2LongArrayMap>")
     ioMap,
  3: i32 (java.swift.type = "java.math.BigInteger") BigInteger,
  4: binary (java.swift.type = "java.nio.ByteBuffer" ) binaryTestBuffer,
}
