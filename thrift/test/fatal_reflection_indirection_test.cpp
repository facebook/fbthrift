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
  using member = info::member::real;

  EXPECT_SAME<
    std::int32_t &,
    decltype(member::getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(member::getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(member::getter::ref(std::declval<type const&>()))
  >();

  type obj;
  member::getter::ref(obj) = 12;
  EXPECT_EQ(12, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, simple_alias_no_indirection) {
  using member = info::member::fake;

  EXPECT_SAME<
    std::int32_t &,
    decltype(member::getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(member::getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(member::getter::ref(std::declval<type const&>()))
  >();

  type obj;
  member::getter::ref(obj) = 15;
  EXPECT_EQ(15, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_single_member_field) {
  using member = info::member::number;

  EXPECT_SAME<
    std::int32_t &,
    decltype(member::getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(member::getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(member::getter::ref(std::declval<type const&>()))
  >();

  type obj;
  member::getter::ref(obj) = -43;
  EXPECT_EQ(-43, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_chained_member_funcs) {
  using member = info::member::result;

  EXPECT_SAME<
    std::int32_t &,
    decltype(member::getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::int32_t &&,
    decltype(member::getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::int32_t const&,
    decltype(member::getter::ref(std::declval<type const&>()))
  >();

  type obj;
  member::getter::ref(obj) = -2;
  EXPECT_EQ(-2, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_string_field) {
  using member = info::member::phrase;

  EXPECT_SAME<
    std::string &,
    decltype(member::getter::ref(std::declval<type &>()))
  >();
  EXPECT_SAME<
    std::string &&,
    decltype(member::getter::ref(std::declval<type &&>()))
  >();
  EXPECT_SAME<
    std::string const&,
    decltype(member::getter::ref(std::declval<type const&>()))
  >();

  type obj;
  member::getter::ref(obj) = "aloha";
  EXPECT_EQ("aloha", member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));
}
