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

namespace hack test.fixtures.jsenum

include "thrift/annotation/hack.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

@hack.Attributes{attributes = ["ApiEnum", "JSEnum"]}
enum MyThriftEnum {
  foo = 1,
  bar = 2,
  baz = 3,
}

@hack.Attributes{attributes = ["ClassAttribute"]}
struct MyThriftStruct {
  @hack.Attributes{attributes = ["FieldAttribute"]}
  1: string foo;
  2: string bar;
  3: string baz;
}

struct MySecondThriftStruct {
  1: MyThriftEnum foo;
  2: MyThriftStruct bar;
  3: i64 baz;
}

@hack.Attributes{attributes = ["ApiEnum", "JSEnum"]}
struct MyThirdThriftStruct {
  @hack.Attributes{attributes = ["FieldAttribute"]}
  1: i32 foo;
}

@hack.UnionEnumAttributes{attributes = ["EnumAttributes"]}
union UnionTesting {
  1: string foo;
  3: i64 bar;
}

@hack.UnionEnumAttributes{attributes = ["EnumAttributes", "EnumAttributes2"]}
union UnionTestingStructured {
  1: string foo;
  3: i64 bar;
}

@hack.Attributes{attributes = ["Oncalls('thrift')"]}
service FooService {
  i32 ping(1: string str_arg);
}

@hack.Attributes{attributes = ["Oncalls('thrift')"]}
service FooService1 {
  i32 ping(1: string str_arg);
}

// Test @hack.FixmeWrongType on individual fields
struct FieldFixme {
  @hack.FixmeWrongType
  1: i64 bad_field;
  2: string good_field;
  @hack.FixmeWrongType
  3: list<string> bad_list;
}

// Test @hack.FixmeWrongType on a struct (all fields)
@hack.FixmeWrongType
struct AllFieldsFixme {
  1: i64 field_one;
  2: string field_two;
  3: list<i32> field_three;
}

// Test @hack.FixmeWrongType on optional fields (nullable)
struct NullableFixme {
  @hack.FixmeWrongType
  1: optional i64 nullable_fixme;
  @hack.FixmeWrongType
  2: optional list<string> nullable_fixme_list;
}

// Test @hack.FixmeWrongType on union fields
union UnionFixme {
  @hack.FixmeWrongType
  1: i64 int_field;
  2: string string_field;
}
