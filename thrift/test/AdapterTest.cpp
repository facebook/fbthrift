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

TEST(AdaptTest, CodeGen) {
  AdaptTestStruct obj1;
  AssertSameType<decltype(*obj1.delay_ref()), std::chrono::milliseconds&>();
  EXPECT_EQ(obj1.delay_ref(), std::chrono::milliseconds(0));
  obj1.delay_ref() = std::chrono::milliseconds(7);
  EXPECT_EQ(obj1.delay_ref(), std::chrono::milliseconds(7));

  auto data = CompactSerializer::serialize<std::string>(obj1);
  AdaptTestStruct obj2;
  CompactSerializer::deserialize(data, obj2);
  EXPECT_EQ(obj1, obj2);
}

} // namespace
} // namespace apache::thrift::test
