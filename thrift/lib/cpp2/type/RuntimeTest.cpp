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

#include <thrift/lib/cpp2/type/Runtime.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache::thrift::type {
namespace {

// Test the exact case of AlignedPtr used by RuntimeType.
TEST(AlignedPtr, InfoPointer) {
  using info_pointer = detail::AlignedPtr<const detail::TypeInfo, 2>;
  EXPECT_EQ(~info_pointer::kMask, 3); // Everything except the last 2 bits.
  const auto* fullPtr = (const detail::TypeInfo*)(info_pointer::kMask);

  info_pointer empty, full{fullPtr, 3};
  EXPECT_FALSE(empty.get<0>());
  EXPECT_FALSE(empty.get<1>());
  EXPECT_EQ(empty.get(), nullptr);
  EXPECT_TRUE(full.get<0>());
  EXPECT_TRUE(full.get<1>());
  EXPECT_EQ(full.get(), fullPtr);

  empty.set<0>();
  full.clear<0>();
  EXPECT_TRUE(empty.get<0>());
  EXPECT_FALSE(empty.get<1>());
  EXPECT_EQ(empty.get(), nullptr);
  EXPECT_FALSE(full.get<0>());
  EXPECT_TRUE(full.get<1>());
  EXPECT_EQ(full.get(), fullPtr);

  empty.set<1>();
  full.clear<1>();
  EXPECT_TRUE(empty.get<0>());
  EXPECT_TRUE(empty.get<1>());
  EXPECT_EQ(empty.get(), nullptr);
  EXPECT_FALSE(full.get<0>());
  EXPECT_FALSE(full.get<1>());
  EXPECT_EQ(full.get(), fullPtr);

  empty.clear<0>();
  full.set<0>();
  EXPECT_FALSE(empty.get<0>());
  EXPECT_TRUE(empty.get<1>());
  EXPECT_EQ(empty.get(), nullptr);
  EXPECT_TRUE(full.get<0>());
  EXPECT_FALSE(full.get<1>());
  EXPECT_EQ(full.get(), fullPtr);
}

TEST(RuntimeRefTest, Void) {
  Ref ref;
  EXPECT_EQ(ref.type(), Type::get<void_t>());
  EXPECT_TRUE(ref.empty());
  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::logic_error);
  EXPECT_THROW(ref.get("field1"), std::logic_error);

  EXPECT_THROW(ref.as<string_t>(), std::bad_any_cast);
  EXPECT_TRUE(ref.tryAs<string_t>() == nullptr);
}

TEST(RuntimeRefTest, Int) {
  int32_t value = 1;
  Ref ref = Ref::create<i32_t>(value);
  EXPECT_EQ(ref.type(), Type::get<i32_t>());
  EXPECT_FALSE(ref.empty());
  EXPECT_TRUE(ref.add(ref));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(ref, Ref::create<i32_t>(2));
  EXPECT_EQ(ref, Value::create<i32_t>(2));

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_EQ(value, 0);
  EXPECT_EQ(ref.as<i32_t>(), 0);
  EXPECT_EQ(ref, Ref::create<i32_t>(0));
  EXPECT_EQ(ref, Value::create<i32_t>());
}

TEST(RuntimeRefTest, List) {
  std::vector<std::string> value;
  std::string elem = "hi";
  auto ref = Ref::create<list<string_t>>(value);
  EXPECT_TRUE(ref.empty());
  ref.append(Ref::create<string_t>(elem));
  EXPECT_THAT(value, ::testing::ElementsAre("hi"));

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("field1"), std::runtime_error);
  int i;
  EXPECT_THROW(ref.get(Ref::create<i32_t>(i = 0)), std::runtime_error);
  EXPECT_THROW(ref.get(Ref::create<i32_t>(i = 1)), std::runtime_error);
  EXPECT_THROW(ref.add(Ref::create<string_t>(value[0])), std::runtime_error);
  EXPECT_THROW(ref.get(Ref::create<i32_t>(i)), std::runtime_error);

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(RuntimeRefTest, Set) {
  std::set<std::string> value;
  std::string key = "hi";
  auto ref = Ref::create<set<string_t>>(value);
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(ref.add(Ref::create<string_t>(key)));
  EXPECT_THAT(value, ::testing::ElementsAre("hi"));

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(ref.get(FieldId{1}), std::runtime_error);
  EXPECT_THROW(ref.get("hi"), std::runtime_error);
  EXPECT_THROW(ref.get(Ref::create<string_t>(key)), std::runtime_error);

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(RuntimeRefTest, Map) {
  std::map<std::string, int> value;
  std::string one = "one";
  int v;
  auto ref = Ref::create<map<string_t, i32_t>>(value);
  EXPECT_TRUE(ref.empty());
  EXPECT_FALSE(ref.put(Ref::create<string_t>(one), Ref::create<i32_t>(v = 1)));
  EXPECT_EQ(value["one"], 1);

  EXPECT_TRUE(ref.put("one", Ref::create<i32_t>(v = 2)));
  EXPECT_EQ(value["one"], 2);

  EXPECT_FALSE(ref.empty());
  EXPECT_THROW(
      ref.put(FieldId{1}, Ref::create<i32_t>(v = 2)), std::logic_error);
  EXPECT_THROW(ref.get(FieldId{1}), std::logic_error);
  EXPECT_EQ(ref.get("one"), Ref::create<i32_t>(2));

  ref.clear();
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(value.empty());
}

TEST(RuntimeRefTest, Identical) {
  float value = 1.0f;
  auto ref = Ref::create<float_t>(value);
  EXPECT_FALSE(ref.empty());
  ref.clear();
  float zero = 0.0;
  float negZero = -0.0;
  double dblZero = 0.0;
  EXPECT_TRUE(ref.empty());
  EXPECT_TRUE(ref.identical(Ref::create<float_t>(zero)));
  EXPECT_FALSE(ref.identical(Ref::create<float_t>(negZero)));
  EXPECT_FALSE(ref.identical(Ref::create<double_t>(dblZero)));
}

TEST(RuntimeRefTest, ConstRef) {
  constexpr int32_t one = 1;
  auto ref = Ref::create<i32_t>(one);
  EXPECT_FALSE(ref.empty());
  EXPECT_TRUE(ref.identical(Ref::create<i32_t>(1)));
  // Cannot be modified.
  EXPECT_THROW(ref.clear(), std::logic_error);
}

TEST(RuntimeValueTest, Void) {
  Value value;
  EXPECT_EQ(value.type(), Type::get<void_t>());
  EXPECT_TRUE(value.empty());
  value.clear();
  EXPECT_TRUE(value.empty());

  EXPECT_THROW(value.as<string_t>(), std::bad_any_cast);
  EXPECT_TRUE(value.tryAs<string_t>() == nullptr);
}

TEST(RuntimeValueTest, Int) {
  Value value = Value::create<i32_t>(1);
  EXPECT_EQ(value.type(), Type::get<i32_t>());
  EXPECT_FALSE(value.empty());
  value.clear();
  EXPECT_TRUE(value.empty());

  EXPECT_EQ(value.as<i32_t>(), 0);
  value.as<i32_t>() = 2;
  EXPECT_FALSE(value.empty());
}

TEST(RuntimeValueTest, List) {
  Value value;
  value = Value::create<list<string_t>>();
  EXPECT_TRUE(value.empty());
  value.as<list<string_t>>().emplace_back("hi");
  EXPECT_FALSE(value.empty());
  Value other(value);
  EXPECT_FALSE(other.empty());
  value.clear();
  EXPECT_TRUE(value.empty());
  EXPECT_TRUE(value.as<list<string_t>>().empty());
  value = other;
  EXPECT_FALSE(value.empty());
}

TEST(RuntimeValueTest, Identical) {
  Value value;
  value = Value::create<float_t>(1.0f);
  EXPECT_FALSE(value.empty());
  value.clear();
  EXPECT_TRUE(value.empty());
  EXPECT_TRUE(value.identical(Value::create<float_t>(0.0f)));
  EXPECT_FALSE(value.identical(Value::create<float_t>(-0.0f)));
  EXPECT_FALSE(value.identical(Value::create<double_t>(0.0)));
}

} // namespace
} // namespace apache::thrift::type
