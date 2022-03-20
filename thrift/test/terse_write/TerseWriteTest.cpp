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

#include <map>
#include <set>
#include <vector>

#include <folly/portability/GTest.h>
#include <thrift/test/terse_write/gen-cpp2/deprecated_terse_write_types.h>
#include <thrift/test/terse_write/gen-cpp2/terse_write_types.h>

namespace apache::thrift::test {

template <class T>
struct TerseWriteTests : ::testing::Test {};

TYPED_TEST_CASE_P(TerseWriteTests);
TYPED_TEST_P(TerseWriteTests, assign) {
  TypeParam obj;
  terse_write::MyStruct s;
  s.field1() = 1;

  obj.bool_field() = true;
  obj.byte_field() = 1;
  obj.short_field() = 2;
  obj.int_field() = 3;
  obj.long_field() = 4;
  obj.float_field() = 5;
  obj.double_field() = 6;
  obj.string_field() = "7";
  obj.binary_field() = "8";
  obj.enum_field() = terse_write::MyEnum::ME1;
  obj.list_field() = {1};
  obj.set_field() = {1};
  obj.map_field() = std::map<int32_t, int32_t>{{1, 1}};
  obj.struct_field() = s;

  EXPECT_EQ(obj.bool_field(), true);
  EXPECT_EQ(obj.byte_field(), 1);
  EXPECT_EQ(obj.short_field(), 2);
  EXPECT_EQ(obj.int_field(), 3);
  EXPECT_EQ(obj.long_field(), 4);
  EXPECT_EQ(obj.float_field(), 5);
  EXPECT_EQ(obj.double_field(), 6);
  EXPECT_EQ(obj.string_field(), "7");
  EXPECT_EQ(obj.binary_field(), "8");
  EXPECT_EQ(obj.enum_field(), terse_write::MyEnum::ME1);
  EXPECT_FALSE(obj.list_field()->empty());
  EXPECT_FALSE(obj.set_field()->empty());
  EXPECT_FALSE(obj.map_field()->empty());
  EXPECT_EQ(obj.struct_field(), s);

  apache::thrift::clear(obj);

  EXPECT_EQ(obj.bool_field(), false);
  EXPECT_EQ(obj.byte_field(), 0);
  EXPECT_EQ(obj.short_field(), 0);
  EXPECT_EQ(obj.int_field(), 0);
  EXPECT_EQ(obj.long_field(), 0);
  EXPECT_EQ(obj.float_field(), 0);
  EXPECT_EQ(obj.double_field(), 0);
  EXPECT_EQ(obj.string_field(), "");
  EXPECT_EQ(obj.binary_field(), "");
  EXPECT_EQ(obj.enum_field(), terse_write::MyEnum::ME0);
  EXPECT_TRUE(obj.list_field()->empty());
  EXPECT_TRUE(obj.set_field()->empty());
  EXPECT_TRUE(obj.map_field()->empty());
  EXPECT_EQ(obj.struct_field(), terse_write::MyStruct());
}
REGISTER_TYPED_TEST_CASE_P(TerseWriteTests, assign);
using TerseWriteStructs = ::testing::Types<
    terse_write::FieldLevelTerseStruct,
    terse_write::StructLevelTerseStruct,
    deprecated_terse_write::FieldLevelTerseStruct,
    deprecated_terse_write::StructLevelTerseStruct>;
INSTANTIATE_TYPED_TEST_CASE_P(
    TerseWriteTest, TerseWriteTests, TerseWriteStructs);

} // namespace apache::thrift::test
