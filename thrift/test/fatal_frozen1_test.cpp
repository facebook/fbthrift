/*
 * Copyright 2016-present Facebook, Inc.
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

#include <thrift/lib/cpp2/fatal/frozen1.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <thrift/test/gen-cpp2/frozen1_fatal_types.h>

#include <gtest/gtest.h>

namespace apache {
namespace thrift {

TEST(frozen1, align_up) {
  EXPECT_EQ(0, frzn_dtl::align_up<std::size_t>(0, 1));
  EXPECT_EQ(1, frzn_dtl::align_up<std::size_t>(1, 1));
  EXPECT_EQ(2, frzn_dtl::align_up<std::size_t>(2, 1));
  EXPECT_EQ(3, frzn_dtl::align_up<std::size_t>(3, 1));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(4, 1));
  EXPECT_EQ(5, frzn_dtl::align_up<std::size_t>(5, 1));
  EXPECT_EQ(6, frzn_dtl::align_up<std::size_t>(6, 1));
  EXPECT_EQ(7, frzn_dtl::align_up<std::size_t>(7, 1));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(8, 1));
  EXPECT_EQ(25, frzn_dtl::align_up<std::size_t>(25, 1));

  EXPECT_EQ(0, frzn_dtl::align_up<std::size_t>(0, 2));
  EXPECT_EQ(2, frzn_dtl::align_up<std::size_t>(1, 2));
  EXPECT_EQ(2, frzn_dtl::align_up<std::size_t>(2, 2));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(3, 2));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(4, 2));
  EXPECT_EQ(6, frzn_dtl::align_up<std::size_t>(5, 2));
  EXPECT_EQ(6, frzn_dtl::align_up<std::size_t>(6, 2));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(7, 2));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(8, 2));

  EXPECT_EQ(0, frzn_dtl::align_up<std::size_t>(0, 4));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(1, 4));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(2, 4));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(3, 4));
  EXPECT_EQ(4, frzn_dtl::align_up<std::size_t>(4, 4));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(5, 4));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(6, 4));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(7, 4));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(8, 4));
  EXPECT_EQ(12, frzn_dtl::align_up<std::size_t>(9, 4));
  EXPECT_EQ(12, frzn_dtl::align_up<std::size_t>(10, 4));
  EXPECT_EQ(12, frzn_dtl::align_up<std::size_t>(11, 4));
  EXPECT_EQ(12, frzn_dtl::align_up<std::size_t>(12, 4));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(13, 4));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(14, 4));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(15, 4));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(16, 4));
  EXPECT_EQ(20, frzn_dtl::align_up<std::size_t>(17, 4));
  EXPECT_EQ(20, frzn_dtl::align_up<std::size_t>(18, 4));


  EXPECT_EQ(0, frzn_dtl::align_up<std::size_t>(0, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(1, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(2, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(3, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(4, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(5, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(6, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(7, 8));
  EXPECT_EQ(8, frzn_dtl::align_up<std::size_t>(8, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(9, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(10, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(11, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(12, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(13, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(14, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(15, 8));
  EXPECT_EQ(16, frzn_dtl::align_up<std::size_t>(16, 8));
  EXPECT_EQ(24, frzn_dtl::align_up<std::size_t>(17, 8));
  EXPECT_EQ(24, frzn_dtl::align_up<std::size_t>(18, 8));
}

} // namespace thrift {
} // namespace apache {

namespace test_cpp2 {
namespace cpp_frozen1 {

#include "thrift/test/generated_frozen1_test.h"

} // namespace cpp_frozen1 {
} // namespace test_cpp2 {
