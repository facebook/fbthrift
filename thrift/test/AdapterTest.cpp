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
#include <thrift/test/gen-cpp2/adapter_types.h>

namespace apache::thrift::test {
namespace {

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

TEST(AdaptTest, StructCodeGen) {
  AdaptTestStruct obj1;
  AssertSameType<decltype(*obj1.delay_ref()), std::chrono::milliseconds&>();
  AssertSameType<decltype(*obj1.custom_ref()), Num&>();

  EXPECT_EQ(obj1.delay_ref(), std::chrono::milliseconds(0));
  obj1.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1.custom_ref()->val, 13); // Defined in Num.
  obj1.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj1.custom_ref()->val, std::numeric_limits<int64_t>::min());

  auto data = CompactSerializer::serialize<std::string>(obj1);
  AdaptTestStruct obj2;
  CompactSerializer::deserialize(data, obj2);
  EXPECT_EQ(obj2, obj1);
  EXPECT_FALSE(obj2 < obj1);
  EXPECT_EQ(obj2.delay_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj2.custom_ref()->val, std::numeric_limits<int64_t>::min());

  obj2.custom_ref()->val = 1;
  EXPECT_NE(obj2, obj1);
  EXPECT_TRUE(obj1.custom_ref() < obj2.custom_ref());
  EXPECT_FALSE(obj2.custom_ref() < obj1.custom_ref());
  EXPECT_TRUE(obj1 < obj2);
  EXPECT_FALSE(obj2 < obj1);

  obj1.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj2, obj1);
  EXPECT_TRUE(obj2.delay_ref() < obj1.delay_ref());
  EXPECT_FALSE(obj1.delay_ref() < obj2.delay_ref());
  EXPECT_TRUE(obj2 < obj1);
  EXPECT_FALSE(obj1 < obj2);
}

TEST(AdaptTest, UnionCodeGen) {
  AdaptTestUnion obj1a;
  AdaptTestUnion obj1b;

  EXPECT_EQ(obj1a.delay_ref().ensure(), std::chrono::milliseconds(0));
  obj1a.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1a.delay_ref(), std::chrono::milliseconds(7));

  EXPECT_EQ(obj1b.custom_ref().ensure().val, 13); // Defined in Num.
  obj1b.custom_ref() = Num{std::numeric_limits<int64_t>::min()};
  EXPECT_EQ(obj1b.custom_ref()->val, std::numeric_limits<int64_t>::min());

  auto dataA = CompactSerializer::serialize<std::string>(obj1a);
  auto dataB = CompactSerializer::serialize<std::string>(obj1b);
  AdaptTestUnion obj2a;
  AdaptTestUnion obj2b;
  CompactSerializer::deserialize(dataA, obj2a);
  CompactSerializer::deserialize(dataB, obj2b);
  EXPECT_EQ(obj2a, obj1a);
  EXPECT_EQ(obj2b, obj1b);
  EXPECT_FALSE(obj2a < obj1a);
  EXPECT_FALSE(obj2b < obj1b);
  EXPECT_EQ(obj2a.delay_ref(), std::chrono::milliseconds(7));
  EXPECT_EQ(obj2b.custom_ref()->val, std::numeric_limits<int64_t>::min());

  obj2b.custom_ref()->val = 1;
  EXPECT_NE(obj2b, obj1b);
  EXPECT_TRUE(obj1b.custom_ref() < obj2b.custom_ref());
  EXPECT_FALSE(obj2b.custom_ref() < obj1b.custom_ref());
  EXPECT_TRUE(obj1b < obj2b);
  EXPECT_FALSE(obj2b < obj1b);

  obj1a.delay_ref() = std::chrono::milliseconds(8);
  EXPECT_NE(obj2a, obj1a);
  EXPECT_TRUE(obj2a.delay_ref() < obj1a.delay_ref());
  EXPECT_FALSE(obj1a.delay_ref() < obj2a.delay_ref());
  EXPECT_TRUE(obj2a < obj1a);
  EXPECT_FALSE(obj1a < obj2a);
}

} // namespace
} // namespace apache::thrift::test
