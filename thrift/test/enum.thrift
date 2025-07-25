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

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

enum MyEnum1 {
  ME1_0 = 0,
  ME1_1 = 1,
  ME1_2 = 2,
  ME1_3 = 3,
  ME1_5 = 5,
  ME1_6 = 6,
}

@thrift.Uri{value = "facebook.com/thrift/test/MyEnum2"}
enum MyEnum2 {
  ME2_0 = 0,
  ME2_1 = 1,
  ME2_2 = 2,
}

enum MyEnum3 {
  ME3_0 = 0,
  ME3_1 = 1,
  ME3_N2 = -2,
  ME3_N1 = -1,
  ME3_9 = 9,
  ME3_10 = 10,
}

enum MyEnum4 {
  ME4_A = 0x7ffffffd,
  ME4_B = 0x7ffffffe,
  ME4_C = 0x7fffffff,
// attempting to define another enum value here fails
// with an overflow error, as we overflow values that can be
// represented with an i32.
}

enum MyEnum5 {
// attempting to explicitly use values out of the i32 range will also fail
// ME5_A = 0x80000000,
// ME5_B = 0x100000000,
}

struct MyStruct {
  1: MyEnum2 me2_2 = ME2_2;
  2: MyEnum3 me3_n2 = ME3_N2;
  4: MyEnum1 me1_t1 = 1;
  5: MyEnum1 me1_t2 = ME1_1;
  6: MyEnum1 me1_t3 = MyEnum1.ME1_1;
  7: MyEnum1 me1_nodefault;
  8: optional MyEnum1 me1_optional;
}

const MyEnum4 c_me4_a = ME4_A;

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.deprecated_enum_unscoped": "1"},
}
enum MyEnumUnscoped {
  MEU_A = 4,
  MEU_B = 3,
}

struct MyStructWithForwardRefEnum {
  1: MyForwardRefEnum a = NONZERO;
  2: MyForwardRefEnum b = MyForwardRefEnum.NONZERO;
}

enum MyForwardRefEnum {
  ZERO = 0,
  NONZERO = 12,
}

@cpp.EnumType{type = cpp.EnumUnderlyingType.I8}
enum I8 {
}

@cpp.EnumType{type = cpp.EnumUnderlyingType.U8}
enum U8 {
}

@cpp.EnumType{type = cpp.EnumUnderlyingType.I16}
enum I16 {
}

@cpp.EnumType{type = cpp.EnumUnderlyingType.U16}
enum U16 {
}

@cpp.EnumType{type = cpp.EnumUnderlyingType.U32}
enum U32 {
}

// For testing union tag enum
union MyUnion {
  1: i32 first_field;
  2: i32 i32_field;
  // Notice the gap in `id` is intentional for testing the case
  5: i32 last_field;
}

union EmptyUnion {}

enum EmptyEnum {
}
