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

#include <folly/portability/GMock.h>
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

} // namespace apache::thrift::test
