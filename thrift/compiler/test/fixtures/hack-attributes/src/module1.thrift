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

namespace hack test.fixtures.jsenum

enum MyThriftEnum {
  foo = 1;
  bar = 2;
  baz = 3;
} (hack.attributes="ApiEnum, JSEnum")

struct MyThriftStruct {
  1: string foo (hack.attributes="FieldAttribute"),
  2: string bar (hack.visibility="private" hack.getter
    hack.getter_attributes="FieldGetterAttribute"),
  3: string baz (hack.getter hack.getter_attributes="FieldGetterAttribute"),
} (hack.attributes="ClassAttribute")

struct MySecondThriftStruct {
  1: MyThriftEnum foo (hack.visibility="private"),
  2: MyThriftStruct bar (hack.visibility="protected" hack.getter
    hack.getter_attributes="FieldStructGetterAttribute"),
  3: i64 baz (hack.getter hack.getter_attributes="FieldGetterAttribute"),
}

union UnionTesting {
  1: string foo (hack.visibility="protected" hack.getter
    hack.getter_attributes="FieldUnionGetterAttribute"),
  3: i64 bar (hack.getter hack.getter_attributes="FieldGetterAttribute"),
} (hack.union_enum_attributes="EnumAttributes"
    hack.union_type_getter_attributes="GetTypeAttribute")
