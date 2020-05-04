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

#include <folly/portability/GTest.h>

#include <thrift/test/gen-cpp2/fatal_optionals_fatal.h>
#include <thrift/test/gen-cpp2/fatal_optionals_types.h>

class FatalOptionalsTest : public testing::Test {};

TEST_F(FatalOptionalsTest, isset) {
  using type = apache::thrift::test::has_fields;
  using meta = apache::thrift::reflect_struct<type>;

  {
    using member = meta::member::reqd;

    type obj;
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_TRUE(member::is_set(obj));
    static_assert(
        std::is_same<decltype(member::getter::ref(obj)), int32_t&>::value, "");
  }

  {
    using member = meta::member::norm;

    type obj;
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_TRUE(member::is_set(obj));
    static_assert(
        std::is_same<decltype(member::getter::ref(obj)), int32_t&>::value, "");
  }

  {
    using member = meta::member::optl;

    type obj;
    EXPECT_FALSE(meta::member::optl::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_FALSE(member::is_set(obj));
  }

  {
    using member = meta::member::optl;

    type obj;
    obj.optl_ref() = 3;
    EXPECT_TRUE(meta::member::optl::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_FALSE(member::is_set(obj));

    static_assert(
        std::is_same<
            decltype(member::getter::ref(obj)),
            apache::thrift::DeprecatedOptionalField<int32_t>&>::value,
        "");
  }

  {
    meta::member::optl::field_ref_getter f;
    type obj;
    f(obj) = 3;
    EXPECT_EQ(*obj.optl, 3);
    obj.optl = 2;
    EXPECT_EQ(*f(obj), 2);
    static_assert(
        std::is_same<
            apache::thrift::optional_field_ref<int32_t&>,
            decltype(f(obj))>::value,
        "");
  }
}
