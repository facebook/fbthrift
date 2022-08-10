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

#include <thrift/lib/cpp2/type/detail/RuntimeType.h>

#include <folly/portability/GTest.h>

namespace apache::thrift::type::detail {
namespace {

// Test the exact case of AlignedPtr used by RuntimeType.
TEST(AlignedPtr, InfoPointer) {
  using info_pointer = AlignedPtr<const TypeInfo, 2>;
  EXPECT_EQ(~info_pointer::kMask, 3); // Everything except the last 2 bits.
  const auto* fullPtr = (const TypeInfo*)(info_pointer::kMask);

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

} // namespace
} // namespace apache::thrift::type::detail
