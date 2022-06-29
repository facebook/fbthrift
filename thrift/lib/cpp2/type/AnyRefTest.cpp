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

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

namespace apache::thrift::type {
namespace {

TEST(AnyRefTest, Void) {
  AnyRef ref;
  EXPECT_EQ(ref.type(), Type::get<void_t>());
  EXPECT_TRUE(ref.empty());
  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::logic_error);
  EXPECT_THROW(ref.get("field1"), std::logic_error);

  EXPECT_THROW(ref.as<string_t>(), std::bad_any_cast);
  EXPECT_TRUE(ref.tryAs<string_t>() == nullptr);
}

TEST(AnyRefTest, Int) {
  int32_t value = 1;
  AnyRef ref = AnyRef::create<i32_t>(value);
  EXPECT_EQ(ref.type(), Type::get<i32_t>());
  EXPECT_FALSE(ref.empty());
  EXPECT_TRUE(ref.add(ref));
  EXPECT_EQ(value, 2);
  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_EQ(value, 0);
  EXPECT_EQ(ref.as<i32_t>(), 0);
}

TEST(AnyRefTest, List) {
  std::vector<std::string> value;
  std::string elem = "hi";
  auto ref = AnyRef::create<list<string_t>>(value);
  EXPECT_TRUE(ref.empty());
  ref.append(AnyRef::create<string_t>(elem));
  EXPECT_THAT(value, ::testing::ElementsAre("hi"));

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("field1"), std::runtime_error);
  int i;
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(i = 0)), std::runtime_error);
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(i = 1)), std::runtime_error);
  EXPECT_THROW(ref.add(AnyRef::create<string_t>(value[0])), std::runtime_error);
  EXPECT_THROW(ref.get(AnyRef::create<i32_t>(i)), std::runtime_error);

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(AnyRefTest, Set) {
  std::set<std::string> value;
  std::string key = "hi";
  auto ref = AnyRef::create<set<string_t>>(value);
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(ref.add(AnyRef::create<string_t>(key)));
  EXPECT_THAT(value, ::testing::ElementsAre("hi"));

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("hi"), std::runtime_error);
  EXPECT_THROW(ref.get(AnyRef::create<string_t>(key)), std::runtime_error);

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(AnyRefTest, Map) {
  std::map<std::string, int> value;
  std::string one = "one";
  int v;
  auto ref = AnyRef::create<map<string_t, i32_t>>(value);
  EXPECT_TRUE(ref.empty());
  EXPECT_FALSE(
      ref.put(AnyRef::create<string_t>(one), AnyRef::create<i32_t>(v = 1)));
  EXPECT_EQ(value["one"], 1);

  EXPECT_TRUE(ref.put("one", AnyRef::create<i32_t>(v = 2)));
  EXPECT_EQ(value["one"], 2);

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(
      ref.put(FieldId{1}, AnyRef::create<i32_t>(v = 2)), std::logic_error);
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("one"), std::runtime_error);
  EXPECT_THROW(ref.get(AnyRef::create<string_t>(one)), std::runtime_error);

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
