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
#include <thrift/lib/cpp2/op/Testing.h>
#include <thrift/lib/cpp2/op/detail/Patch.h>
#include <thrift/test/gen-cpp2/StructPatchTest_types.h>

namespace apache::thrift {
using test::patch::MyStruct;
using test::patch::MyStructPatch;
using test::patch::MyStructValuePatch;

void applyPatch(const MyStructPatch& patch, MyStruct& val) {
  patch.boolVal()->apply(*val.boolVal());
  patch.byteVal()->apply(*val.byteVal());
  patch.i16Val()->apply(*val.i16Val());
  patch.i32Val()->apply(*val.i32Val());
  patch.i64Val()->apply(*val.i64Val());
  patch.floatVal()->apply(*val.floatVal());
  patch.doubleVal()->apply(*val.doubleVal());
  patch.stringVal()->apply(*val.stringVal());
  patch.binaryVal()->apply(*val.binaryVal());
}

bool emptyPatch(const MyStructPatch& patch) {
  return patch == MyStructPatch{};
}

using TestStructPatch = op::detail::StructPatch<MyStructValuePatch>;

TEST(StructPatchTest, Assign) {
  MyStruct actual;
  MyStructPatch patch;

  // Empty patch does nothing.
  EXPECT_TRUE(emptyPatch(patch));
  applyPatch(patch, actual);
  EXPECT_EQ(actual, MyStruct{});

  // Assign patch assigns in parallel.
  MyStruct expected;
  patch.boolVal() = *expected.boolVal() = true;
  patch.byteVal() = *expected.byteVal() = 2;
  patch.i16Val() = *expected.i16Val() = 3;
  patch.i32Val() = *expected.i32Val() = 4;
  patch.i64Val() = *expected.i64Val() = 5;
  patch.floatVal() = *expected.floatVal() = 6;
  patch.doubleVal() = *expected.doubleVal() = 7;
  patch.stringVal() = *expected.stringVal() = "8";
  patch.binaryVal() = *expected.binaryVal() =
      StringTraits<folly::IOBuf>::fromStringLiteral("9");
  EXPECT_FALSE(emptyPatch(patch));
  EXPECT_NE(actual, expected);
  applyPatch(patch, actual);
  EXPECT_EQ(actual, expected);

  // Apply the patch again to ensure it is idempotent.
  applyPatch(patch, actual);
  EXPECT_EQ(actual, expected);

  // Assign in a single step, via op::patch.
  TestStructPatch assignPatch;
  assignPatch = expected;
  test::expectPatch(assignPatch, {}, expected);

  // Check the individual fields for better failure messages.
  EXPECT_EQ(*actual.boolVal(), *expected.boolVal());
  EXPECT_EQ(*actual.byteVal(), *expected.byteVal());
  EXPECT_EQ(*actual.i16Val(), *expected.i16Val());
  EXPECT_EQ(*actual.i32Val(), *expected.i32Val());
  EXPECT_EQ(*actual.i64Val(), *expected.i64Val());
  EXPECT_EQ(*actual.floatVal(), *expected.floatVal());
  EXPECT_EQ(*actual.doubleVal(), *expected.doubleVal());
  EXPECT_EQ(*actual.stringVal(), *expected.stringVal());
  EXPECT_TRUE(StringTraits<folly::IOBuf>::isEqual(
      *actual.binaryVal(), *expected.binaryVal()));
}

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

TEST(StructPatchTest, Clear) {
  // Clear patch, clears all fields (even ones with defaults)
  TestStructPatch patch;
  patch.clear();
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
