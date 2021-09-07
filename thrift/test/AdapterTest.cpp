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

TEST(AdaptTest, AdaptedT) {
  AssertSameType<adapt_detail::adapted_t<OverloadedAdatper, int64_t>, Num>();
  AssertSameType<
      adapt_detail::adapted_t<OverloadedAdatper, std::string>,
      String>();
}

namespace basic {
TEST(AdaptTest, StructCodeGen_Empty) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.
  EXPECT_EQ(obj0a.timeout_ref(), std::chrono::milliseconds(0));

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 13);
  EXPECT_EQ(obj0b.timeout_ref(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj0b, obj0a);
}

TEST(AdaptTest, StructCodeGen_Zero) {
  AdaptTestStruct obj0a;
  EXPECT_EQ(obj0a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0a.custom_ref()->val, 13); // Defined in Num.
  obj0a.custom_ref()->val = 0;
  EXPECT_EQ(obj0a.timeout_ref(), std::chrono::milliseconds(0));

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestStruct obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj0b.custom_ref()->val, 0);
  EXPECT_EQ(obj0b.timeout_ref(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj0b, obj0a);
}

TEST(AdaptTest, StructCodeGen) {
  AdaptTestStruct obj1a;
  AssertSameType<decltype(*obj1a.delay_ref()), std::chrono::milliseconds&>();
  AssertSameType<decltype(*obj1a.custom_ref()), Num&>();
  AssertSameType<decltype(*obj1a.timeout_ref()), std::chrono::milliseconds&>();

  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1a.custom_ref()->val, 13);
  obj1a.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj1a.custom_ref()->val, std::numeric_limits<int64_t>::min());

  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(0));
  obj1a.timeout_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(7));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestStruct obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj1b.custom_ref()->val, std::numeric_limits<int64_t>::min());
  EXPECT_EQ(obj1b.timeout_ref(), std::chrono::milliseconds(7));

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

  obj1a.__clear();
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj1a.custom_ref()->val, 13);
  EXPECT_EQ(obj1a.timeout_ref(), std::chrono::milliseconds(0));
}
} // namespace basic

namespace terse {
TEST(AdaptTest, StructCodeGen_Empty_Terse) {
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

TEST(AdaptTest, StructCodeGen_Zero_Terse) {
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

TEST(AdaptTest, StructCodeGen_Terse) {
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

  obj1a.__clear();
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(0));
  EXPECT_EQ(obj1a.custom_ref()->val, 13);
}
} // namespace terse

namespace basic {
TEST(AdaptTest, UnionCodeGen_Empty) {
  AdaptTestUnion obj0a;
  EXPECT_EQ(obj0a.getType(), AdaptTestUnion::__EMPTY__);

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestUnion obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.getType(), AdaptTestUnion::__EMPTY__);

  EXPECT_EQ(obj0b, obj0a);
  EXPECT_FALSE(obj0b < obj0a);
}

TEST(AdaptTest, UnionCodeGen_Delay_Default) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref().ensure(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);
}

TEST(AdaptTest, UnionCodeGen_Delay) {
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

TEST(AdaptTest, UnionCodeGen_Custom_Default) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 13);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST(AdaptTest, UnionCodeGen_Custom_Zero) {
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

TEST(AdaptTest, UnionCodeGen_Custom) {
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
} // namespace basic

namespace terse {
TEST(AdaptTest, UnionCodeGen_Empty_Terse) {
  AdaptTestUnion obj0a;
  EXPECT_EQ(obj0a.getType(), AdaptTestUnion::__EMPTY__);

  auto data0 = CompactSerializer::serialize<std::string>(obj0a);
  AdaptTestUnion obj0b;
  CompactSerializer::deserialize(data0, obj0b);
  EXPECT_EQ(obj0b.getType(), AdaptTestUnion::__EMPTY__);

  EXPECT_EQ(obj0b, obj0a);
  EXPECT_FALSE(obj0b < obj0a);
}

TEST(AdaptTest, UnionCodeGen_Delay_Default_Terse) {
  AdaptTestUnion obj1a;
  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));

  auto data1 = CompactSerializer::serialize<std::string>(obj1a);
  AdaptTestUnion obj1b;
  CompactSerializer::deserialize(data1, obj1b);
  EXPECT_EQ(obj1b.delay_ref().ensure(), std::chrono::milliseconds(0));

  EXPECT_EQ(obj1b, obj1a);
  EXPECT_FALSE(obj1b < obj1a);
}

TEST(AdaptTest, UnionCodeGen_Delay_Terse) {
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

TEST(AdaptTest, UnionCodeGen_Custom_Default_Terse) {
  AdaptTestUnion obj2a;
  EXPECT_EQ(obj2a.custom_ref().ensure().val, 13); // Defined in Num.

  auto data2 = CompactSerializer::serialize<std::string>(obj2a);
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(data2, obj2b);
  EXPECT_EQ(obj2b.custom_ref()->val, 13);

  EXPECT_EQ(obj2b, obj2a);
  EXPECT_FALSE(obj2b < obj2a);
}

TEST(AdaptTest, UnionCodeGen_Custom_Zero_Terse) {
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

TEST(AdaptTest, UnionCodeGen_Custom_Terse) {
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

} // namespace apache::thrift::test
