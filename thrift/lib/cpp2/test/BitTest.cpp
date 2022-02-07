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

#include <thread>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/FieldRef.h>

namespace apache::thrift::detail::test {

template <class T>
BitSet<T> makeBitSet(T& storage) {
  if constexpr (std::is_reference_v<T>) {
    return BitSet<T>{storage};
  } else {
    return {};
  }
}

template <class>
struct IntTest : ::testing::Test {};

using Ints = ::testing::
    Types<uint8_t, uint8_t&, std::atomic<uint8_t>, std::atomic<uint8_t>&>;
TYPED_TEST_SUITE(IntTest, Ints);

TYPED_TEST(IntTest, Basic) {
  std::remove_reference_t<TypeParam> storage{0};
  auto b = makeBitSet<TypeParam>(storage);
  for (int i = 0; i < 8; i++) {
    b[i] = i % 3;
  }
  EXPECT_FALSE(b[0]);
  EXPECT_TRUE(b[1]);
  EXPECT_TRUE(b[2]);
  EXPECT_FALSE(b[3]);
  EXPECT_TRUE(b[4]);
  EXPECT_TRUE(b[5]);
  EXPECT_FALSE(b[6]);
  EXPECT_TRUE(b[7]);
}

template <class>
struct AtomicIntTest : ::testing::Test {};

using AtomicInts =
    ::testing::Types<std::atomic<uint8_t>, std::atomic<uint8_t>&>;
TYPED_TEST_SUITE(AtomicIntTest, AtomicInts);

TYPED_TEST(AtomicIntTest, Basic) {
  std::remove_reference_t<TypeParam> storage{0};
  auto b = makeBitSet<TypeParam>(storage);
  std::thread t[8];
  for (int i = 0; i < 8; i++) {
    t[i] = std::thread([&b, i] { b[i] = i % 3; });
  }
  for (auto& i : t) {
    i.join();
  }
  EXPECT_FALSE(b[0]);
  EXPECT_TRUE(b[1]);
  EXPECT_TRUE(b[2]);
  EXPECT_FALSE(b[3]);
  EXPECT_TRUE(b[4]);
  EXPECT_TRUE(b[5]);
  EXPECT_FALSE(b[6]);
  EXPECT_TRUE(b[7]);
}

} // namespace apache::thrift::detail::test
