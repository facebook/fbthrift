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

#include <thrift/lib/cpp2/type/ThriftOp.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift {
namespace {
using conformance::Value;

TEST(ThriftOpTest, Double) {
  // 1 is equal and identical to itself.
  EXPECT_TRUE(op::equal<type::double_t>(1.0, 1.0));
  EXPECT_TRUE(op::identical<type::double_t>(1.0, 1.0));

  // 1 is neither equal or identical to 2.
  EXPECT_FALSE(op::equal<type::double_t>(1.0, 2.0));
  EXPECT_FALSE(op::identical<type::double_t>(1.0, 2.0));

  // -0 is equal to, but not identical to 0.
  EXPECT_TRUE(op::equal<type::double_t>(-0.0, +0.0));
  EXPECT_FALSE(op::identical<type::double_t>(-0.0, +0.0));

  // NaN is identical to, but not equal to itself.
  EXPECT_FALSE(op::equal<type::double_t>(
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN()));
  EXPECT_TRUE(op::identical<type::double_t>(
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN()));
}

TEST(ThriftOpTest, Float) {
  // 1 is equal and identical to itself.
  EXPECT_TRUE(op::equal<type::float_t>(1.0f, 1.0f));
  EXPECT_TRUE(op::identical<type::float_t>(1.0f, 1.0f));

  // 1 is neither equal or identical to 2.
  EXPECT_FALSE(op::equal<type::float_t>(1.0f, 2.0f));
  EXPECT_FALSE(op::identical<type::float_t>(1.0f, 2.0f));

  // -0 is equal to, but not identical to 0.
  EXPECT_TRUE(op::equal<type::float_t>(-0.0f, +0.0f));
  EXPECT_FALSE(op::identical<type::float_t>(-0.0f, +0.0f));

  // NaN is identical to, but not equal to itself.
  EXPECT_FALSE(op::equal<type::float_t>(
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::quiet_NaN()));
  EXPECT_TRUE(op::identical<type::float_t>(
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::quiet_NaN()));
}

TEST(ThriftOpTest, StructWithFloat) {
  Value lhs;
  Value rhs;
  op::equal_to<type::struct_t<Value>> equal;
  op::identical_to<type::struct_t<Value>> identical;

  lhs.floatValue_ref().ensure() = std::numeric_limits<float>::quiet_NaN();
  rhs.floatValue_ref().ensure() = std::numeric_limits<float>::quiet_NaN();
  EXPECT_FALSE(equal(lhs, rhs));
  EXPECT_FALSE(identical(lhs, rhs)); // Should be true!

  lhs.floatValue_ref().ensure() = -0.0f;
  rhs.floatValue_ref().ensure() = +0.0f;
  EXPECT_TRUE(equal(lhs, rhs));
  EXPECT_TRUE(identical(lhs, rhs)); // Should be false!
}

} // namespace
} // namespace apache::thrift
