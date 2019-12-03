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

#include <thrift/test/gen-cpp2/field_ref_codegen_types.h>

#include <folly/portability/GTest.h>

using namespace apache::thrift::test;

TEST(field_ref_codegen_test, optional_getter) {
  test_struct s;
  apache::thrift::optional_field_ref<int64_t&> ref = s.foo_ref();
  EXPECT_FALSE(ref.has_value());
  ref = 42;
  EXPECT_TRUE(ref.has_value());
  EXPECT_EQ(*ref, 42);
}

TEST(field_ref_codegen_test, getter) {
  test_struct s;
  apache::thrift::field_ref<int64_t&> ref = s.bar_ref();
  EXPECT_FALSE(ref.is_set());
  ref = 42;
  EXPECT_TRUE(ref.is_set());
  EXPECT_EQ(*ref, 42);
}

TEST(field_ref_codegen_test, comparison) {
  {
    test_struct s1;
    test_struct s2;
    EXPECT_EQ(s1.foo_ref(), s2.foo_ref());
  }
  {
    test_struct s1;
    s1.foo_ref() = 1;
    test_struct s2;
    EXPECT_NE(s1.foo_ref(), s2.foo_ref());
  }
  {
    test_struct s1;
    test_struct s2;
    s2.foo_ref() = 1;
    EXPECT_NE(s1.foo_ref(), s2.foo_ref());
  }
  {
    test_struct s1;
    s1.foo_ref() = 1;
    test_struct s2;
    s2.foo_ref() = 2;
    EXPECT_NE(s1.foo_ref(), s2.foo_ref());
  }
  {
    test_struct s1;
    s1.foo_ref() = 1;
    test_struct s2;
    s2.foo_ref() = 1;
    EXPECT_EQ(s1.foo_ref(), s2.foo_ref());
  }

  // const version

  {
    test_struct s1;
    s1.foo_ref() = 1;
    const auto& const_s1 = s1;

    test_struct s2;
    s2.foo_ref() = 1;
    EXPECT_EQ(const_s1.foo_ref(), s2.foo_ref());
  }

  {
    test_struct s1;
    s1.foo_ref() = 1;
    const auto& const_s1 = s1;

    test_struct s2;
    s2.foo_ref() = 1;
    const auto& const_s2 = s2;
    EXPECT_EQ(const_s1.foo_ref(), const_s2.foo_ref());
  }

  {
    test_struct s1;
    s1.foo_ref() = 1;
    const auto& const_s1 = s1;

    test_struct s2;
    const auto& const_s2 = s2;
    EXPECT_NE(const_s1.foo_ref(), const_s2.foo_ref());
  }

  {
    test_struct s1;
    const auto& const_s1 = s1;

    test_struct s2;
    const auto& const_s2 = s2;
    EXPECT_EQ(const_s1.foo_ref(), const_s2.foo_ref());
  }

  {
    test_struct s1;
    const auto& const_s1 = s1;

    test_struct s2;
    s2.foo_ref() = 1;
    const auto& const_s2 = s2;
    EXPECT_NE(const_s1.foo_ref(), const_s2.foo_ref());
  }
}
