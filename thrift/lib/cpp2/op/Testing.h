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

// gtest matchers for thrift ops.
#pragma once

#include <utility>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/op/Clear.h>
#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::test {

template <typename Tag, typename T = type::native_type<Tag>>
struct IdenticalToMatcher {
  T expected;
  template <typename U>
  bool MatchAndExplain(const U& actual, ::testing::MatchResultListener*) const {
    return op::identical<Tag>(actual, expected);
  }
  void DescribeTo(std::ostream* os) const {
    *os << "is identical to " << ::testing::PrintToString(expected);
  }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not identical to " << ::testing::PrintToString(expected);
  }
};

template <typename Tag, typename T = type::native_type<Tag>>
auto IsIdenticalTo(T expected) {
  return ::testing::MakePolymorphicMatcher(
      IdenticalToMatcher<Tag, T>{std::move(expected)});
}

template <typename Tag, typename T>
struct EqualToMatcher {
  T expected;
  template <typename U>
  bool MatchAndExplain(const U& actual, ::testing::MatchResultListener*) const {
    return op::equal<Tag>(actual, expected);
  }
  void DescribeTo(std::ostream* os) const {
    *os << "is equal to " << ::testing::PrintToString(expected);
  }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not equal to " << ::testing::PrintToString(expected);
  }
};

template <typename Tag, typename T = type::native_type<Tag>>
auto IsEqualTo(T expected) {
  return ::testing::MakePolymorphicMatcher(
      EqualToMatcher<Tag, T>{std::move(expected)});
}

template <typename Tag>
struct EmptyMatcher {
  template <typename U>
  bool MatchAndExplain(const U& actual, ::testing::MatchResultListener*) const {
    return op::isEmpty<Tag>(actual);
  }
  void DescribeTo(std::ostream* os) const { *os << "is empty"; }
  void DescribeNegationTo(std::ostream* os) const { *os << "is not empty"; }
};

template <typename Tag>
auto IsEmpty() {
  return ::testing::MakePolymorphicMatcher(EmptyMatcher<Tag>{});
}

// Applies a patch twice and checks the results.
template <
    typename P,
    typename T1 = typename P::value_type,
    typename T2 = typename P::value_type>
void expectPatch(
    P patch,
    const typename P::value_type& actual,
    const T1& expected1,
    const T2& expected2) {
  { // Applying twice produces the expected results.
    auto actual1 = actual;
    patch.apply(actual1);
    EXPECT_EQ(actual1, expected1);
    patch.apply(actual1);
    EXPECT_EQ(actual1, expected2);
    if (actual != expected1) {
      // The value changes, so this cannot be an empty patch.
      EXPECT_FALSE(patch.empty());
    }
  }
  { // Merging with self, is the same as applying twice.
    patch.merge(patch);
    auto actual2 = actual;
    patch.apply(actual2);
    EXPECT_EQ(actual2, expected2);
  }
  { // Reset should be a noop patch.
    patch.reset();
    EXPECT_TRUE(patch.empty());
    auto actual3 = actual;
    patch.apply(actual3);
    EXPECT_EQ(actual3, actual);
  }
}
template <typename P, typename T = typename P::value_type>
void expectPatch(
    P patch, const typename P::value_type& actual, const T& expected) {
  expectPatch(std::move(patch), actual, expected, expected);
}

} // namespace apache::thrift::test
