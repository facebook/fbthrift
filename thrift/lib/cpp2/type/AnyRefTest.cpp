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

#include <thrift/lib/cpp2/type/AnyRef.h>

#include <folly/portability/GTest.h>

namespace apache::thrift::type {
namespace {

TEST(AnyRefTest, Void) {
  AnyRef ref;
  EXPECT_EQ(ref.type(), Type::get<void_t>());
  EXPECT_TRUE(ref.empty());
  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::out_of_range);
  EXPECT_THROW(ref.get("field1"), std::out_of_range);

  EXPECT_THROW(ref.as<string_t>(), std::bad_any_cast);
  EXPECT_TRUE(ref.tryAs<string_t>() == nullptr);
}

TEST(AnyRefTest, Int) {
  int32_t value = 1;
  AnyRef ref = AnyRef::create<i32_t>(value);
  EXPECT_EQ(ref.type(), Type::get<i32_t>());
  EXPECT_FALSE(ref.empty());
  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_EQ(value, 0);
  EXPECT_EQ(ref.as<i32_t>(), 0);
}

TEST(AnyRefTest, List) {
  std::vector<std::string> value;
  auto ref = AnyRef::create<list<string_t>>(value);
  EXPECT_TRUE(ref.empty());
  value.emplace_back("hi");

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("field1"), std::runtime_error);
  int index = 0;
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(index)), std::runtime_error);
  index = 1;
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(index)), std::runtime_error);
  EXPECT_THROW(ref.add(AnyRef::create<string_t>(value[0])), std::runtime_error);
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(index)), std::runtime_error);

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(AnyRefTest, Identical) {
  float value = 1.0f;
  auto ref = AnyRef::create<float_t>(value);
  EXPECT_FALSE(ref.empty());
  ref.clear();
  float zero = 0.0;
  float negZero = -0.0;
  double dblZero = 0.0;
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(ref.identical(AnyRef::create<float_t>(zero)));
  EXPECT_FALSE(ref.identical(AnyRef::create<float_t>(negZero)));
  EXPECT_FALSE(ref.identical(AnyRef::create<double_t>(dblZero)));
}

TEST(AnyRefTest, ConstRef) {
  constexpr int32_t one = 1;
  auto ref = AnyRef::create<i32_t>(one);
  EXPECT_FALSE(ref.empty());
  EXPECT_TRUE(ref.identical(AnyRef::create<i32_t>(1)));
  // Cannot be modified.
  EXPECT_THROW(ref.clear(), std::logic_error);
}

} // namespace
} // namespace apache::thrift::type
