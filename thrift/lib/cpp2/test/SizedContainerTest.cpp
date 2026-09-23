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

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <gtest/gtest.h>
#include <folly/container/small_vector.h>
#include <folly/container/sorted_vector_types.h>
#include <thrift/lib/cpp2/test/gen-cpp2/SizedContainerTest_types.h>

namespace apache::thrift {
namespace {

TEST(SizedContainerTest, CurriesContainerSize) {
  EXPECT_EQ(
      test::SmallIntVector({1, 2}), (folly::small_vector<int32_t, 3>{1, 2}));
  EXPECT_EQ(
      test::SmallIntSet({3, 1}),
      (folly::small_sorted_vector_set<int32_t, 3>{1, 3}));
}

TEST(SizedContainerTest, CurriesContainerComparator) {
  static_assert(std::is_same_v<
                test::TransparentLessMap,
                std::map<std::string, int32_t, std::less<>>>);
  static_assert(std::is_same_v<
                test::GreaterMap,
                std::map<std::string, int32_t, std::greater<std::string>>>);
}

} // namespace
} // namespace apache::thrift
