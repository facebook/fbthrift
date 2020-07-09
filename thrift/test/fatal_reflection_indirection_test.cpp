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

#include <folly/Traits.h>
#include <folly/Utility.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/reflection/internal/test_helpers.h>
#include <thrift/test/gen-cpp2/fatal_reflection_indirection_fatal.h>

namespace {

class FatalReflectionIndirectionTest : public testing::Test {};
} // namespace

using type = reflection_indirection::struct_with_indirections;
using info = apache::thrift::reflect_struct<type>;

TEST_F(FatalReflectionIndirectionTest, sanity_check_no_indirection) {
  using member = info::member::real;

  EXPECT_SAME<
      std::int32_t&,
      decltype(member::getter::ref(std::declval<type&>()))>();
  EXPECT_SAME<
      std::int32_t&&,
      decltype(member::getter::ref(std::declval<type&&>()))>();
  EXPECT_SAME<
      std::int32_t const&,
      decltype(member::getter::ref(std::declval<type const&>()))>();

  type obj;
  member::getter::ref(obj) = 12;
  EXPECT_EQ(12, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));

  member::field_ref_getter{}(obj) = 13;
  EXPECT_EQ(13, member::getter::ref(obj));
  EXPECT_TRUE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, simple_alias_no_indirection) {
  using member = info::member::fake;

  EXPECT_SAME<
      std::int32_t&,
      decltype(member::getter::ref(std::declval<type&>()))>();
  EXPECT_SAME<
      std::int32_t&&,
      decltype(member::getter::ref(std::declval<type&&>()))>();
  EXPECT_SAME<
      std::int32_t const&,
      decltype(member::getter::ref(std::declval<type const&>()))>();

  type obj;
  member::getter::ref(obj) = 15;
  EXPECT_EQ(15, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));

  member::field_ref_getter{}(obj) = 16;
  EXPECT_EQ(16, member::getter::ref(obj));
  EXPECT_TRUE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_single_member_field) {
  using member = info::member::number;

  EXPECT_SAME<
      std::int32_t&,
      decltype(member::getter::ref(std::declval<type&>()))>();
  EXPECT_SAME<
      std::int32_t&&,
      decltype(member::getter::ref(std::declval<type&&>()))>();
  EXPECT_SAME<
      std::int32_t const&,
      decltype(member::getter::ref(std::declval<type const&>()))>();

  type obj;
  member::getter::ref(obj) = -43;
  EXPECT_EQ(-43, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));

  member::field_ref_getter{}(obj) = reflection_indirection::HasANumber(-42);
  EXPECT_EQ(-42, member::getter::ref(obj));
  EXPECT_TRUE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_via_chained_member_funcs) {
  using member = info::member::result;

  EXPECT_SAME<
      std::int32_t&,
      decltype(member::getter::ref(std::declval<type&>()))>();
  EXPECT_SAME<
      std::int32_t&&,
      decltype(member::getter::ref(std::declval<type&&>()))>();
  EXPECT_SAME<
      std::int32_t const&,
      decltype(member::getter::ref(std::declval<type const&>()))>();

  type obj;
  member::getter::ref(obj) = -2;
  EXPECT_EQ(-2, member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));

  member::field_ref_getter{}(obj) = reflection_indirection::HasAResult(-1);
  EXPECT_EQ(-1, member::getter::ref(obj));
  EXPECT_TRUE(member::is_set(obj));
}

TEST_F(FatalReflectionIndirectionTest, indirection_string_field) {
  using member = info::member::phrase;

  EXPECT_SAME<
      std::string&,
      decltype(member::getter::ref(std::declval<type&>()))>();
  EXPECT_SAME<
      std::string&&,
      decltype(member::getter::ref(std::declval<type&&>()))>();
  EXPECT_SAME<
      std::string const&,
      decltype(member::getter::ref(std::declval<type const&>()))>();

  type obj;
  member::getter::ref(obj) = "hello";
  EXPECT_EQ("hello", member::getter::ref(obj));

  EXPECT_FALSE(member::is_set(obj));
  member::mark_set(obj, true);
  EXPECT_TRUE(member::is_set(obj));
  member::mark_set(obj, false);
  EXPECT_FALSE(member::is_set(obj));

  member::field_ref_getter{}(obj) = reflection_indirection::HasAPhrase("world");
  EXPECT_EQ("world", member::getter::ref(obj));
  EXPECT_TRUE(member::is_set(obj));
}
