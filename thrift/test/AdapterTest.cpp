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

#include <thrift/test/AdapterTest.h>

#include <chrono>
#include <limits>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/gen-cpp2/adapter_terse_types.h>
#include <thrift/test/gen-cpp2/adapter_types.h>

namespace apache::thrift::test {
template <typename Actual, typename Expected>
struct AssertSameType;
template <typename T>
struct AssertSameType<T, T> {};

struct AdapterTest : ::testing::Test {};

TEST_F(AdapterTest, AdaptedEnum) {
  using basic::AdaptedEnum;
  basic::AdaptTemplatedTestStruct myStruct;

  AssertSameType<AdaptedEnum&, decltype(*myStruct.adaptedEnum())>();
  EXPECT_EQ(myStruct.adaptedEnum(), AdaptedEnum::One);
  myStruct.adaptedEnum() = AdaptedEnum::Two;

  auto data = CompactSerializer::serialize<std::string>(myStruct);
  basic::AdaptTemplatedTestStruct myStruct2;
  CompactSerializer::deserialize(data, myStruct2);
  EXPECT_EQ(myStruct2.adaptedEnum(), AdaptedEnum::Two);

  EXPECT_EQ(myStruct, myStruct2);
}

TEST_F(AdapterTest, AdaptedT) {
  AssertSameType<adapt_detail::adapted_t<OverloadedAdapter, int64_t>, Num>();
  AssertSameType<
      adapt_detail::adapted_t<OverloadedAdapter, std::string>,
      String>();
}

TEST_F(AdapterTest, IsMutableRef) {
  EXPECT_FALSE(adapt_detail::is_mutable_ref<int>::value);
  EXPECT_FALSE(adapt_detail::is_mutable_ref<const int>::value);
  EXPECT_FALSE(adapt_detail::is_mutable_ref<const int&>::value);
  EXPECT_FALSE(adapt_detail::is_mutable_ref<const int&&>::value);
  EXPECT_TRUE(adapt_detail::is_mutable_ref<int&>::value);
  EXPECT_TRUE(adapt_detail::is_mutable_ref<int&&>::value);
  EXPECT_TRUE(adapt_detail::is_mutable_ref<volatile int&&>::value);
}

TEST_F(AdapterTest, HasInplaceToThrift) {
  EXPECT_TRUE((adapt_detail::has_inplace_toThrift<
               IndirectionAdapter<IndirectionString>,
               IndirectionString>::value));
  // Does not return a mutable ref.
  EXPECT_FALSE(
      (adapt_detail::has_inplace_toThrift<OverloadedAdapter, Num>::value));

  // Indirection currently defaults to the identity function if illformed.
  AssertSameType<decltype(apply_indirection(std::declval<int&>())), int&>();
  EXPECT_TRUE((
      adapt_detail::has_inplace_toThrift<IndirectionAdapter<int>, int>::value));

  // `IndirectionAdapter<IndirectionString>::toThrift(int&&)`
  // is invalid, so we get false.
  EXPECT_FALSE((adapt_detail::has_inplace_toThrift<
                IndirectionAdapter<IndirectionAdapter<IndirectionString>>,
                int>::value));
}

namespace basic {
TEST_F(AdapterTest, StructCodeGen_Empty) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.
  EXPECT_EQ(obj0a.timeout_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.indirectionString_ref()->val, "");

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 13);
  EXPECT_EQ(obj0b.timeout_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.indirectionString_ref()->val, "");

  EXPECT_EQ(obj0b, obj0a);
}

TEST_F(AdapterTest, StructCodeGen_Zero) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.
  obj0a.custom_ref()->val = 0;
  EXPECT_EQ(obj0a.timeout_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.indirectionString_ref()->val, "");

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 0);
  EXPECT_EQ(obj0b.timeout_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.indirectionString_ref()->val, "");

  EXPECT_EQ(obj0b, obj0a);
}

TEST_F(AdapterTest, StructCodeGen) {
  AdaptTestStruct obj1a;
  AssertSameType<decltype(*obj1a.delay_ref()), std::chrono::milliseconds&>();
  AssertSameType<decltype(*obj1a.custom_ref()), Num&>();
  AssertSameType<decltype(*obj1a.timeout_ref()), std::chrono::milliseconds&>();
  AssertSameType<
      decltype(*obj1a.indirectionString_ref()),
      IndirectionString&>();

  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1a.custom_ref()->val, 13);
  obj1a.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj1a.custom_ref()->val, std::numeric_limits<int64_t>::min());

  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(0));
  obj1a.timeout_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1a.indirectionString_ref()->val, "");
  obj1a.indirectionString_ref()->val = "hi";
  EXPECT_EQ(obj1a.indirectionString_ref()->val, "hi");

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestStruct obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj1b.custom_ref()->val, std::numeric_limits<int64_t>::min());
  EXPECT_EQ(obj1b.timeout_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj1a.indirectionString_ref()->val, "hi");

  EXPECT_EQ(obj1b, obj1b);
  EXPECT_FALSE(obj1b < obj1a);

  obj1b.custom_ref()->val = 1;
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1a.custom_ref() < obj1b.custom_ref());
  EXPECT_FALSE(obj1b.custom_ref() < obj1a.custom_ref());
  EXPECT_TRUE(obj1a < obj1b);
  EXPECT_FALSE(obj1b < obj1a);

  obj1a.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1b.delay_ref() < obj1a.delay_ref());
  EXPECT_FALSE(obj1a.delay_ref() < obj1b.delay_ref());
  EXPECT_TRUE(obj1b < obj1a);
  EXPECT_FALSE(obj1a < obj1b);

  obj1a.timeout_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1b.timeout_ref() < obj1a.timeout_ref());
  EXPECT_FALSE(obj1a.timeout_ref() < obj1b.timeout_ref());
  EXPECT_TRUE(obj1b < obj1a);
  EXPECT_FALSE(obj1a < obj1b);

  obj1a = {};
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj1a.custom_ref()->val, 13);
  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(0));
}
} // namespace basic

namespace terse {
TEST_F(AdapterTest, StructCodeGen_Empty_Terse) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 13);

  EXPECT_EQ(obj0b, obj0a);
}

TEST_F(AdapterTest, StructCodeGen_Zero_Terse) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.
  obj0a.custom_ref()->val = 0;

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 0);

  EXPECT_EQ(obj0b, obj0a);
}

TEST_F(AdapterTest, StructCodeGen_Terse) {
  AdaptTestStruct obj1a;
  AssertSameType<decltype(*obj1a.delay_ref()), std::chrono::milliseconds&>();
  AssertSameType<decltype(*obj1a.custom_ref()), Num&>();

  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1a.custom_ref()->val, 13);
  obj1a.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj1a.custom_ref()->val, std::numeric_limits<int64_t>::min());

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestStruct obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj1b.custom_ref()->val, std::numeric_limits<int64_t>::min());

  EXPECT_EQ(obj1b, obj1b);
  EXPECT_FALSE(obj1b < obj1a);

  obj1b.custom_ref()->val = 1;
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1a.custom_ref() < obj1b.custom_ref());
  EXPECT_FALSE(obj1b.custom_ref() < obj1a.custom_ref());
  EXPECT_TRUE(obj1a < obj1b);
  EXPECT_FALSE(obj1b < obj1a);

  obj1a.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1b.delay_ref() < obj1a.delay_ref());
  EXPECT_FALSE(obj1a.delay_ref() < obj1b.delay_ref());
  EXPECT_TRUE(obj1b < obj1a);
  EXPECT_FALSE(obj1a < obj1b);

  obj1a = {};
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj1a.custom_ref()->val, 13);
}
} // namespace terse

namespace basic {
TEST_F(AdapterTest, UnionCodeGen_Empty) {
  AdaptTestUnion obj0a;
  EXPECT_EQ(obj0a.getType(), AdaptTestUnion::__EMPTY__);

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestUnion obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.getType(), AdaptTestUnion::__EMPTY__);

  EXPECT_EQ(obj0b, obj0a);
  EXPECT_FALSE(obj0b < obj0a);
}

TEST_F(AdapterTest, UnionCodeGen_Delay_Default) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref().ensure(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);
}

TEST_F(AdapterTest, UnionCodeGen_Delay) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);

  obj1a.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1b.delay_ref() < obj1a.delay_ref());
  EXPECT_FALSE(obj1a.delay_ref() < obj1b.delay_ref());
  EXPECT_TRUE(obj1b < obj1a);
  EXPECT_FALSE(obj1a < obj1b);
}

TEST_F(AdapterTest, UnionCodeGen_Custom_Default) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 13);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST_F(AdapterTest, UnionCodeGen_Custom_Zero) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.
  obj2a.custom_ref()->val = 0;

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 0);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST_F(AdapterTest, UnionCodeGen_Custom) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.
  obj2a.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj2a.custom_ref()->val, std::numeric_limits<int64_t>::min());

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, std::numeric_limits<int64_t>::min());

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);

  obj2b.custom_ref()->val = 1;
  EXPECT_NE(obj2b, obj2a);
  EXPECT_TRUE(obj2a.custom_ref() < obj2b.custom_ref());
  EXPECT_FALSE(obj2b.custom_ref() < obj2a.custom_ref());
  EXPECT_TRUE(obj2a < obj2b);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST_F(AdapterTest, TemplatedTestAdapter_AdaptTemplatedTestStruct) {
  auto obj = AdaptTemplatedTestStruct();
  EXPECT_EQ(obj.adaptedBoolDefault_ref()->value, true);
  EXPECT_EQ(obj.adaptedByteDefault_ref()->value, 1);
  EXPECT_EQ(obj.adaptedShortDefault_ref()->value, 2);
  EXPECT_EQ(obj.adaptedIntegerDefault_ref()->value, 3);
  EXPECT_EQ(obj.adaptedLongDefault_ref()->value, 4);
  EXPECT_EQ(obj.adaptedDoubleDefault_ref()->value, 5);
  EXPECT_EQ(obj.adaptedStringDefault_ref()->value, "6");

  obj.adaptedBool_ref() = Wrapper<bool>{true};
  obj.adaptedByte_ref() = Wrapper<int8_t>{1};
  obj.adaptedShort_ref() = Wrapper<int16_t>{2};
  obj.adaptedInteger_ref() = Wrapper<int32_t>{3};
  obj.adaptedLong_ref() = Wrapper<int64_t>{1};
  obj.adaptedDouble_ref() = Wrapper<double>{2};
  obj.adaptedString_ref() = Wrapper<std::string>{"3"};
  EXPECT_EQ(obj.adaptedBool_ref()->value, true);
  EXPECT_EQ(obj.adaptedByte_ref()->value, 1);
  EXPECT_EQ(obj.adaptedShort_ref()->value, 2);
  EXPECT_EQ(obj.adaptedInteger_ref()->value, 3);
  EXPECT_EQ(obj.adaptedLong_ref()->value, 1);
  EXPECT_EQ(obj.adaptedDouble_ref()->value, 2);
  EXPECT_EQ(obj.adaptedString_ref()->value, "3");

  auto objs = CompactSerializer::serialize<std::string>(obj);
  AdaptTemplatedTestStruct objd;
  CompactSerializer::deserialize(objs, objd);
  EXPECT_EQ(objd.adaptedBool_ref()->value, true);
  EXPECT_EQ(objd.adaptedByte_ref()->value, 1);
  EXPECT_EQ(objd.adaptedShort_ref()->value, 2);
  EXPECT_EQ(objd.adaptedInteger_ref()->value, 3);
  EXPECT_EQ(objd.adaptedLong_ref()->value, 1);
  EXPECT_EQ(objd.adaptedDouble_ref()->value, 2);
  EXPECT_EQ(objd.adaptedString_ref()->value, "3");
  EXPECT_EQ(obj, objd);

  // Adapted fields reset to the intrinsic default.
  apache::thrift::clear(obj);
  EXPECT_EQ(obj.adaptedBoolDefault_ref()->value, false);
  EXPECT_EQ(obj.adaptedByteDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedShortDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedIntegerDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedLongDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedDoubleDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStringDefault_ref()->value, "");
}

TEST_F(AdapterTest, TemplatedTestAdapter_AdaptTemplatedNestedTestStruct) {
  auto obj = AdaptTemplatedNestedTestStruct();
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedBoolDefault_ref()->value, true);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedByteDefault_ref()->value, 1);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedShortDefault_ref()->value, 2);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedIntegerDefault_ref()->value, 3);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedLongDefault_ref()->value, 4);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedDoubleDefault_ref()->value, 5);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedStringDefault_ref()->value, "6");

  obj.adaptedStruct_ref()->adaptedBool_ref() = Wrapper<bool>{true};
  obj.adaptedStruct_ref()->adaptedByte_ref() = Wrapper<int8_t>{1};
  obj.adaptedStruct_ref()->adaptedShort_ref() = Wrapper<int16_t>{2};
  obj.adaptedStruct_ref()->adaptedInteger_ref() = Wrapper<int32_t>{3};
  obj.adaptedStruct_ref()->adaptedLong_ref() = Wrapper<int64_t>{1};
  obj.adaptedStruct_ref()->adaptedDouble_ref() = Wrapper<double>{2};
  obj.adaptedStruct_ref()->adaptedString_ref() = Wrapper<std::string>{"3"};

  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedBool_ref()->value, true);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedByte_ref()->value, 1);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedShort_ref()->value, 2);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedInteger_ref()->value, 3);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedLong_ref()->value, 1);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedDouble_ref()->value, 2);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedString_ref()->value, "3");

  auto objs = CompactSerializer::serialize<std::string>(obj);
  AdaptTemplatedNestedTestStruct objd;
  CompactSerializer::deserialize(objs, objd);

  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedBoolDefault_ref()->value, true);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedByteDefault_ref()->value, 1);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedShortDefault_ref()->value, 2);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedIntegerDefault_ref()->value, 3);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedLongDefault_ref()->value, 4);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedDoubleDefault_ref()->value, 5);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedStringDefault_ref()->value, "6");

  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedBool_ref()->value, true);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedByte_ref()->value, 1);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedShort_ref()->value, 2);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedInteger_ref()->value, 3);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedLong_ref()->value, 1);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedDouble_ref()->value, 2);
  EXPECT_EQ(objd.adaptedStruct_ref()->adaptedString_ref()->value, "3");
  EXPECT_EQ(obj, objd);

  // Adapted fields reset to the intrinsic default.
  apache::thrift::clear(obj);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedBoolDefault_ref()->value, false);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedByteDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedShortDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedIntegerDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedLongDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedDoubleDefault_ref()->value, 0);
  EXPECT_EQ(obj.adaptedStruct_ref()->adaptedStringDefault_ref()->value, "");
}

TEST_F(AdapterTest, StructFieldAdaptedStruct) {
  StructFieldAdaptedStruct obj;
  auto wrapper = Wrapper<AdaptedStruct>();
  wrapper.value.data_ref() = 42;
  obj.adaptedStruct_ref() = wrapper;
  EXPECT_EQ(obj.adaptedStruct_ref()->value.data_ref(), 42);

  auto objs = CompactSerializer::serialize<std::string>(obj);
  StructFieldAdaptedStruct objd;
  CompactSerializer::deserialize(objs, objd);

  EXPECT_EQ(objd.adaptedStruct_ref()->value.data_ref(), 42);
  EXPECT_EQ(obj, objd);

  // Adapted fields reset to the intrinsic default.
  apache::thrift::clear(obj);
  EXPECT_EQ(obj.adaptedStruct_ref()->value.data_ref(), 0);
}
} // namespace basic

namespace terse {
TEST_F(AdapterTest, UnionCodeGen_Empty_Terse) {
  AdaptTestUnion obj0a;
  EXPECT_EQ(obj0a.getType(), AdaptTestUnion::__EMPTY__);

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestUnion obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.getType(), AdaptTestUnion::__EMPTY__);

  EXPECT_EQ(obj0b, obj0a);
  EXPECT_FALSE(obj0b < obj0a);
}

TEST_F(AdapterTest, UnionCodeGen_Delay_Default_Terse) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref().ensure(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);
}

TEST_F(AdapterTest, UnionCodeGen_Delay_Terse) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);

  obj1a.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj1b, obj1a);
  EXPECT_TRUE(obj1b.delay_ref() < obj1a.delay_ref());
  EXPECT_FALSE(obj1a.delay_ref() < obj1b.delay_ref());
  EXPECT_TRUE(obj1b < obj1a);
  EXPECT_FALSE(obj1a < obj1b);
}

TEST_F(AdapterTest, UnionCodeGen_Custom_Default_Terse) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 13);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST_F(AdapterTest, UnionCodeGen_Custom_Zero_Terse) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.
  obj2a.custom_ref()->val = 0;

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 0);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST_F(AdapterTest, UnionCodeGen_Custom_Terse) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.
  obj2a.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj2a.custom_ref()->val, std::numeric_limits<int64_t>::min());

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, std::numeric_limits<int64_t>::min());

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);

  obj2b.custom_ref()->val = 1;
  EXPECT_NE(obj2b, obj2a);
  EXPECT_TRUE(obj2a.custom_ref() < obj2b.custom_ref());
  EXPECT_FALSE(obj2b.custom_ref() < obj2a.custom_ref());
  EXPECT_TRUE(obj2a < obj2b);
  EXPECT_FALSE(obj2b < obj2a);
}
} // namespace terse

struct ReferenceAdapter {
  static const int32_t& fromThrift(const int32_t&);
};
struct ReferenceAdapterWithContext {
  template <typename Context>
  static int64_t&& fromThriftField(const int64_t&, Context&&);
};

TEST_F(AdapterTest, FromThriftField) {
  auto obj = basic::AdaptTestStruct();
  AssertSameType<
      decltype(adapt_detail::fromThriftField<ReferenceAdapter, 0>(0, obj)),
      const int32_t&>();
  AssertSameType<
      decltype(adapt_detail::fromThriftField<ReferenceAdapterWithContext, 0>(
          0, obj)),
      int64_t&&>();
}

TEST(AdaptTest, AdapterWithContext) {
  static_assert(folly::is_detected_v<
                adapt_detail::FromThriftFieldType,
                AdapterWithContext,
                int64_t,
                basic::AdaptTestStruct>);

  auto obj = basic::AdaptTestStruct();
  EXPECT_EQ(obj.data_ref()->meta, &*obj.meta_ref());

  auto copy = basic::AdaptTestStruct(obj);
  EXPECT_EQ(copy.data_ref()->meta, &*copy.meta_ref());

  auto move = basic::AdaptTestStruct(std::move(copy));
  EXPECT_EQ(move.data_ref()->meta, &*move.meta_ref());

  obj.data_ref() = {};
  obj.data_ref()->value = 42;
  obj.meta_ref() = "foo";
  auto serialized = CompactSerializer::serialize<std::string>(obj);
  auto obj2 = basic::AdaptTestStruct();
  CompactSerializer::deserialize(serialized, obj2);
  EXPECT_EQ(obj2.data_ref()->value, 42);
  EXPECT_EQ(obj2.data_ref()->fieldId, 4);
  EXPECT_EQ(*obj2.data_ref()->meta, "foo");
}

} // namespace apache::thrift::test
