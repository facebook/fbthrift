/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/test/gen-cpp/test_constants.h>

#include <gtest/gtest.h>

TEST(constants, cpp) {
  EXPECT_EQ(test_cpp::enum1::field0, test_cpp::test_constants::e_1);
  EXPECT_EQ(test_cpp::enum1::field2, test_cpp::test_constants::e_2);

  EXPECT_EQ(72, test_cpp::test_constants::i_1);
  EXPECT_EQ(99, test_cpp::test_constants::i_2);

  EXPECT_EQ(std::string("hello"), test_cpp::test_constants::str_1);
  EXPECT_EQ(std::string("world"), test_cpp::test_constants::str_2);

  EXPECT_EQ(
    (std::vector<std::int32_t>{23, 42, 56}),
    test_cpp::test_constants::l_1()
  );
  EXPECT_EQ(
    (std::vector<std::string>{"foo", "bar", "baz"}),
    test_cpp::test_constants::l_2()
  );

  EXPECT_EQ(
    (std::set<std::int32_t>{23, 42, 56}),
    test_cpp::test_constants::s_1()
  );
  EXPECT_EQ(
    (std::set<std::string>{"foo", "bar", "baz"}),
    test_cpp::test_constants::s_2()
  );

  EXPECT_EQ(
    (std::map<std::int32_t, std::int32_t>{{23, 97}, {42, 37}, {56, 11}}),
    test_cpp::test_constants::m_1()
  );
  EXPECT_EQ(
    (std::map<std::string, std::string>{{"foo", "bar"}, {"baz", "gaz"}}),
    test_cpp::test_constants::m_2()
  );

  test_cpp::struct1 pod1;
  pod1.a = 10;
  pod1.b = "foo";

  EXPECT_EQ(pod1, test_cpp::test_constants::pod_1());

  test_cpp::struct2 pod2;
  pod2.c.a = 12;
  pod2.c.b = "bar";
  pod2.d = {11, 22, 33};

  EXPECT_EQ(pod2, test_cpp::test_constants::pod_2());
}
