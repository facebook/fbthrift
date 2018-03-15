/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/fatal/debug.h>

namespace apache {
namespace thrift {

struct EqualHelperCallback {
  explicit EqualHelperCallback(
      const char* left,
      const char* right,
      ::testing::AssertionResult& result)
      : left_(left), right_(right), result_(result) {}

  template <typename T>
  void operator()(
      T const& a,
      T const& b,
      folly::StringPiece path,
      folly::StringPiece message) const {
    result_ << path << ": " << message;

    result_ << '\n' << left_ << ":\n";
    ::apache::thrift::pretty_print(result_, a, "  ", "    ");

    result_ << '\n' << right_ << ":\n";
    ::apache::thrift::pretty_print(result_, b, "  ", "    ");

    result_ << '\n';
  }

 private:
  const char* left_;
  const char* right_;
  ::testing::AssertionResult& result_;
};

template <class T>
::testing::AssertionResult
thriftEqualHelper(const char* left, const char* right, const T& a, const T& b) {
  ::testing::AssertionResult result(false);
  if (::apache::thrift::debug_equals(
          a, b, EqualHelperCallback(left, right, result))) {
    return ::testing::AssertionResult(true);
  } else {
    return result;
  }
}

} // namespace thrift
} // namespace apache

#define EXPECT_THRIFT_EQ(a, b) \
  EXPECT_PRED_FORMAT2(::apache::thrift::thriftEqualHelper, a, b)
