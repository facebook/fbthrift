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

#include <array>

#include <thrift/lib/cpp2/type/ThriftOp.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

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
  op::EqualTo<type::struct_t<Value>> equal;
  op::IdenticalTo<type::struct_t<Value>> identical;

  lhs.floatValue_ref().ensure() = std::numeric_limits<float>::quiet_NaN();
  rhs.floatValue_ref().ensure() = std::numeric_limits<float>::quiet_NaN();
  EXPECT_FALSE(equal(lhs, rhs));
  EXPECT_FALSE(identical(lhs, rhs)); // Should be true!

  lhs.floatValue_ref().ensure() = -0.0f;
  rhs.floatValue_ref().ensure() = +0.0f;
  EXPECT_TRUE(equal(lhs, rhs));
  EXPECT_TRUE(identical(lhs, rhs)); // Should be false!
}

TEST(ThriftOpTest, ListWithDouble) {
  op::EqualTo<type::list<type::double_t>> equal;
  op::IdenticalTo<type::list<type::double_t>> identical;

  EXPECT_FALSE(equal(
      {1, std::numeric_limits<float>::quiet_NaN()},
      {1, std::numeric_limits<float>::quiet_NaN()}));
  EXPECT_TRUE(identical(
      {1, std::numeric_limits<float>::quiet_NaN()},
      {1, std::numeric_limits<float>::quiet_NaN()}));

  EXPECT_TRUE(equal({-0.0, 2.0}, {+0.0, 2.0}));
  EXPECT_FALSE(identical({-0.0, 2.0}, {+0.0, 2.0}));
}

TEST(ThriftOpTest, SetWithDouble) {
  op::EqualTo<type::set<type::double_t>> equal;
  op::IdenticalTo<type::set<type::double_t>> identical;

  // Note: NaN in a set is undefined behavior.

  EXPECT_TRUE(equal({-0.0, 2.0}, {+0.0, 2.0}));
  EXPECT_TRUE(identical({-0.0, 2.0}, {+0.0, 2.0})); // Should be false!
}

TEST(ThriftOpTest, MapWithDouble) {
  op::EqualTo<type::map<type::double_t, type::float_t>> equal;
  op::IdenticalTo<type::map<type::double_t, type::float_t>> identical;

  // Note: NaN in a map keys is undefined behavior.
  EXPECT_FALSE(equal(
      {{1, std::numeric_limits<float>::quiet_NaN()}},
      {{1, std::numeric_limits<float>::quiet_NaN()}}));
  EXPECT_FALSE(identical(
      {{1, std::numeric_limits<float>::quiet_NaN()}},
      {{1, std::numeric_limits<float>::quiet_NaN()}})); // Should be true!

  EXPECT_TRUE(equal({{-0.0, 2.0}}, {{+0.0, 2.0}}));
  EXPECT_TRUE(identical({{-0.0, 2.0}}, {{+0.0, 2.0}})); // Should be false!
  EXPECT_TRUE(equal({{2.0, +0.0}}, {{2.0, -0.0}}));
  EXPECT_TRUE(identical({{2.0, +0.0}}, {{2.0, -0.0}})); // Should be false!
  EXPECT_TRUE(equal({{-0.0, +0.0}}, {{+0.0, -0.0}}));
  EXPECT_TRUE(identical({{-0.0, +0.0}}, {{+0.0, -0.0}})); // Should be false!
}

// A test suite that check ops work correctly for a given test case.
template <typename OpTestCase>
class TypedOpTest : public testing::Test {};

// Helpers for defining test cases.
template <typename Tag, typename T = type::standard_type<Tag>>
struct BaseTestCase {
  using type_tag = Tag;
  using type = T;
};
template <typename Tag, typename T = type::standard_type<Tag>>
struct BaseDefaultTestCase : BaseTestCase<Tag, T> {
  const static inline T default_ = {};
};
template <typename Tag, typename T = type::standard_type<Tag>>
struct NumericTestCase : BaseDefaultTestCase<Tag, T> {
  constexpr static T one = 1;
  constexpr static T otherOne = 1;
  constexpr static std::array<T, 2> many = {2, static_cast<T>(-4)};
};

template <typename Tag, typename T = type::standard_type<Tag>>
struct StringTestCase : BaseTestCase<Tag, T> {
  const static inline T default_ = StringTraits<T>::fromStringLiteral("");
  const static inline T one = StringTraits<T>::fromStringLiteral("one");
  const static inline T otherOne = StringTraits<T>::fromStringLiteral("one");
  const static inline std::array<T, 2> many = {
      StringTraits<T>::fromStringLiteral("two"),
      StringTraits<T>::fromStringLiteral("three")};
};

template <
    typename VTagCase,
    typename T = type::standard_type<type::list<typename VTagCase::type_tag>>>
struct ListTestCase
    : BaseDefaultTestCase<type::list<typename VTagCase::type_tag>, T> {
  const static inline T one = {VTagCase::many.begin(), VTagCase::many.end()};
  const static inline T otherOne = {
      VTagCase::many.begin(), VTagCase::many.end()};
  const static inline std::array<T, 3> many = {
      T{VTagCase::many.rbegin(), VTagCase::many.rend()},
      T{VTagCase::one},
      T{VTagCase::default_},
  };
};

template <
    typename KTagCase,
    typename T = type::standard_type<type::set<typename KTagCase::type_tag>>>
struct SetTestCase
    : BaseDefaultTestCase<type::set<typename KTagCase::type_tag>, T> {
  const static inline T one = {KTagCase::many.begin(), KTagCase::many.end()};
  const static inline T otherOne = {
      KTagCase::many.rbegin(), KTagCase::many.rend()};
  const static inline std::array<T, 3> many = {
      T{KTagCase::default_},
      T{KTagCase::default_, KTagCase::one},
      T{KTagCase::one},
  };
};

template <typename KTagCase, typename VTagCase>
using map_type_tag =
    type::map<typename KTagCase::type_tag, typename VTagCase::type_tag>;

template <
    typename KTagCase,
    typename VTagCase,
    typename T = type::standard_type<map_type_tag<KTagCase, VTagCase>>>
struct MapTestCase : BaseDefaultTestCase<map_type_tag<KTagCase, VTagCase>, T> {
  const static inline T one = {
      {KTagCase::one, VTagCase::one}, {KTagCase::default_, VTagCase::one}};
  const static inline T otherOne = {
      {KTagCase::default_, VTagCase::one}, {KTagCase::one, VTagCase::one}};
  const static inline std::array<T, 3> many = {
      T{{KTagCase::one, VTagCase::one}},
      T{{KTagCase::default_, VTagCase::one}},
      T{{KTagCase::one, VTagCase::default_}},
  };
};

// The tests cases to run.
using OpTestCases = ::testing::Types<
    NumericTestCase<type::byte_t>,
    NumericTestCase<type::i16_t>,
    NumericTestCase<type::i16_t, uint16_t>,
    NumericTestCase<type::i32_t>,
    NumericTestCase<type::i32_t, uint32_t>,
    NumericTestCase<type::i64_t>,
    NumericTestCase<type::i64_t, uint64_t>,
    NumericTestCase<type::float_t>,
    NumericTestCase<type::double_t>,
    StringTestCase<type::string_t>,
    StringTestCase<type::binary_t, folly::IOBuf>,
    // TODO(afuller): Fix 'copyability' for this type, so we can test this case.
    // StringTestCase<type::binary_t, std::unique_ptr<folly::IOBuf>>,
    ListTestCase<NumericTestCase<type::byte_t>>,
    ListTestCase<StringTestCase<type::binary_t>>,
    // TODO(afuller): Consider supporting non-default standard types in
    // the paramaterized types, for these tests.
    // ListTestCase<StringTestCase<type::binary_t, folly::IOBuf>>,
    SetTestCase<NumericTestCase<type::i32_t, uint32_t>>,
    MapTestCase<StringTestCase<type::string_t>, NumericTestCase<type::i32_t>>>;

TYPED_TEST_SUITE(TypedOpTest, OpTestCases);

TYPED_TEST(TypedOpTest, Equal) {
  using Tag = typename TypeParam::type_tag;

  EXPECT_TRUE(op::equal<Tag>(TypeParam::default_, TypeParam::default_));
  EXPECT_TRUE(op::equal<Tag>(TypeParam::one, TypeParam::one));
  EXPECT_TRUE(op::equal<Tag>(TypeParam::one, TypeParam::otherOne));

  EXPECT_FALSE(op::equal<Tag>(TypeParam::default_, TypeParam::one));
  EXPECT_FALSE(op::equal<Tag>(TypeParam::one, TypeParam::default_));
  for (const auto& other : TypeParam::many) {
    EXPECT_FALSE(op::equal<Tag>(TypeParam::one, other));
    EXPECT_FALSE(op::equal<Tag>(other, TypeParam::one));
  }
}

TYPED_TEST(TypedOpTest, Empty) {
  using Tag = typename TypeParam::type_tag;

  EXPECT_TRUE(op::isEmpty<Tag>(TypeParam::default_));
  EXPECT_FALSE(op::isEmpty<Tag>(TypeParam::one));
  for (const auto& other : TypeParam::many) {
    EXPECT_FALSE(op::isEmpty<Tag>(other));
  }
}

TYPED_TEST(TypedOpTest, Clear) {
  using T = typename TypeParam::type;
  using Tag = typename TypeParam::type_tag;

  T value = TypeParam::one;
  EXPECT_FALSE(op::isEmpty<Tag>(value));
  EXPECT_FALSE(op::equal<Tag>(value, TypeParam::default_));
  EXPECT_TRUE(op::equal<Tag>(value, TypeParam::one));
  op::clear<Tag>(value);
  EXPECT_TRUE(op::isEmpty<Tag>(value));
  EXPECT_TRUE(op::equal<Tag>(value, TypeParam::default_));
  EXPECT_FALSE(op::equal<Tag>(value, TypeParam::one));
}

TEST(TypedOpTest, HashType) {
  test::OneOfEach value;
  using Tag = type::struct_t<test::OneOfEach>;

  std::unordered_set<std::size_t> s;
  auto check_and_add = [&s](auto tag, const auto& v) {
    using Tag = decltype(tag);
    EXPECT_EQ(s.count(op::hash<Tag>(v)), 0);
    s.insert(op::hash<Tag>(v));
    EXPECT_EQ(s.count(op::hash<Tag>(v)), 1);
  };

  for (auto i = 0; i < 10; i++) {
    value.myI32_ref() = i + 100;
    check_and_add(Tag{}, value);
    value.myList_ref() = {std::to_string(i + 200)};
    check_and_add(Tag{}, value);
    value.mySet_ref() = {std::to_string(i + 300)};
    check_and_add(Tag{}, value);
    value.myMap_ref() = {{std::to_string(i + 400), 0}};
    check_and_add(Tag{}, value);

    check_and_add(type::i32_t{}, *value.myI32_ref());
    check_and_add(type::list<type::string_t>{}, *value.myList_ref());
    check_and_add(type::set<type::string_t>{}, *value.mySet_ref());
    check_and_add(type::map<type::string_t, type::i64_t>{}, *value.myMap_ref());
  }
}

TEST(TypedOpTest, HashDouble) {
  EXPECT_EQ(op::hash<type::double_t>(-0.0), op::hash<type::double_t>(+0.0));
  EXPECT_EQ(
      op::hash<type::double_t>(std::numeric_limits<double>::quiet_NaN()),
      op::hash<type::double_t>(std::numeric_limits<double>::quiet_NaN()));
}

} // namespace
} // namespace apache::thrift
