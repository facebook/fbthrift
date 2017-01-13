/*
 * Copyright 2017 Facebook, Inc.
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

#include "thrift/test/gen-cpp2/fatal_reflection_indirection_fatal.h"

#include <gtest/gtest.h>

#include <folly/Utility.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

namespace {

class FatalReflectionIndirectionTest : public testing::Test {};
}

using type = reflection_indirection::struct_with_indirections;
using info = apache::thrift::reflect_struct<type>;

TEST_F(FatalReflectionIndirectionTest, sanity_check_no_indirection) {
  type obj;
  obj.real = 12;
  EXPECT_EQ(12, obj.real);
}

TEST_F(FatalReflectionIndirectionTest, simple_alias_no_indirection) {
  type obj;
  obj.fake = 15;
  EXPECT_EQ(15, obj.fake);
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_single_member_field) {
  using getter = info::member::number::getter;
  EXPECT_SAME<
    std::int32_t &,
    decltype(getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(getter::ref(std::declval<type const&>()))
  >();

  type obj;
  obj.number.number = -43;
  EXPECT_EQ(-43, getter::ref(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_chained_member_funcs) {
  using getter = info::member::result::getter;
  EXPECT_SAME<
    std::int32_t &,
    decltype(getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(getter::ref(std::declval<type const&>()))
  >();

  type obj;
  obj.result.foo().result() = -2;
  EXPECT_EQ(-2, getter::ref(obj));
}
