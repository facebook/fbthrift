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

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/op/Patch.h>
#include <thrift/lib/cpp2/op/Testing.h>
#include <thrift/lib/cpp2/op/detail/StructPatch.h>
#include <thrift/lib/cpp2/type/Field.h>
#include <thrift/test/gen-cpp2/StructPatchTest_types.h>

namespace apache::thrift {
using test::patch::MyStruct;
using test::patch::MyStructPatch;
using test::patch::MyStructValuePatch;

using TestStructPatch = op::detail::StructPatch<MyStructValuePatch>;

MyStruct testValue() {
  MyStruct val;
  val.boolVal() = true;
  val.byteVal() = 2;
  val.i16Val() = 3;
  val.i32Val() = 4;
  val.i64Val() = 5;
  val.floatVal() = 6;
  val.doubleVal() = 7;
  val.stringVal() = "8";
  val.binaryVal() = StringTraits<folly::IOBuf>::fromStringLiteral("9");
  return val;
}

TestStructPatch testPatch() {
  auto val = testValue();
  TestStructPatch patch;
  patch.patch().boolVal() = !op::BoolPatch{};
  *patch->byteVal() = val.byteVal();
  *patch->i16Val() += 2;
  *patch->i32Val() += 3;
  *patch->i64Val() += 4;
  *patch->floatVal() += 5;
  *patch->doubleVal() += 6;
  patch->stringVal() = "_" + op::StringPatch{} + "_";
  return patch;
}

TEST(StructPatchTest, Noop) {
  // Empty patch does nothing.
  TestStructPatch patch;
  test::expectPatch(patch, {}, {});
}

TEST(StructPatchTest, Assign) {
  // Assign in a single step.
  auto patch = TestStructPatch::createAssign(testValue());
  test::expectPatch(patch, {}, testValue());
}

TEST(StructPatchTest, AssignSplit) {
  auto patch = TestStructPatch::createAssign(testValue());
  // Break apart the assign patch and check the result;
  patch.patch();
  EXPECT_FALSE(patch.hasAssign());
  EXPECT_TRUE(*patch.get().clear());
  EXPECT_NE(*patch.get().patch(), MyStructPatch{});
  test::expectPatch(patch, {}, testValue());
}

TEST(StructPatchTest, Clear) {
  // Clear patch, clears all fields (even ones with defaults)
  TestStructPatch patch;
  patch.clear();
  test::expectPatch(patch, testValue(), {});
}

TEST(StructPatchTest, Patch) {
  auto patch = testPatch();

  MyStruct val;
  val.stringVal() = "hi";

  MyStruct expected1, expected2;
  expected1.boolVal() = true;
  expected2.boolVal() = false;
  expected1.byteVal() = 2;
  expected2.byteVal() = 2;
  expected1.i16Val() = 2;
  expected2.i16Val() = 4;
  expected1.i32Val() = 3;
  expected2.i32Val() = 6;
  expected1.i64Val() = 4;
  expected2.i64Val() = 8;
  expected1.floatVal() = 5;
  expected2.floatVal() = 10;
  expected1.doubleVal() = 6;
  expected2.doubleVal() = 12;
  expected1.stringVal() = "_hi_";
  expected2.stringVal() = "__hi__";

  test::expectPatch(patch, val, expected1, expected2);

  patch.merge(TestStructPatch::createClear());
  EXPECT_FALSE(patch.hasAssign());
  EXPECT_EQ(patch.patch(), MyStructPatch{});
  EXPECT_TRUE(*patch.get().clear());
  test::expectPatch(patch, testValue(), {});
}

TEST(StructPatchTest, ClearAssign) {
  auto patch = TestStructPatch::createClear();
  patch.merge(TestStructPatch::createAssign(testValue()));
  // Assign takes precedence, like usual.
  test::expectPatch(patch, {}, testValue());
}

TEST(StructPatchTest, AssignClear) {
  auto patch = TestStructPatch::createAssign(testValue());
  patch.merge(TestStructPatch::createClear());
  test::expectPatch(patch, testValue(), {});

  // Clear patch takes precedence (as it is smaller to encode and slightly
  // stronger in the presense of non-terse non-optional fields).
  EXPECT_FALSE(patch.hasAssign());
  EXPECT_TRUE(*patch.get().clear());
}

} // namespace apache::thrift
