/*
 * Copyright 2015-present Facebook, Inc.
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
#include <thrift/test/gen-cpp2/test_constants.h>
#include <thrift/test/gen-cpp2/test_types.tcc>

#include <gtest/gtest.h>

TEST(constants, cpp) {
  EXPECT_EQ(test_cpp::enum1::field0, test_cpp::test_constants::e_1_);
  EXPECT_EQ(test_cpp::enum1::field0, test_cpp::test_constants::e_1());
  EXPECT_EQ(test_cpp::enum1::field2, test_cpp::test_constants::e_2_);
  EXPECT_EQ(test_cpp::enum1::field2, test_cpp::test_constants::e_2());

  EXPECT_EQ(72, test_cpp::test_constants::i_1_);
  EXPECT_EQ(72, test_cpp::test_constants::i_1());
  EXPECT_EQ(99, test_cpp::test_constants::i_2_);
  EXPECT_EQ(99, test_cpp::test_constants::i_2());

  EXPECT_EQ(std::string(), test_cpp::test_constants::str_e_);
  EXPECT_EQ(std::string(), test_cpp::test_constants::str_e());
  EXPECT_EQ(std::string("hello"), test_cpp::test_constants::str_1_);
  EXPECT_EQ(std::string("hello"), test_cpp::test_constants::str_1());
  EXPECT_EQ(std::string("world"), test_cpp::test_constants::str_2_);
  EXPECT_EQ(std::string("world"), test_cpp::test_constants::str_2());
  EXPECT_EQ(std::string("'"), test_cpp::test_constants::str_3());
  EXPECT_EQ(std::string("\"foo\""), test_cpp::test_constants::str_4());

  EXPECT_TRUE(test_cpp::test_constants::l_e().empty());
  EXPECT_EQ(
    (std::vector<std::int32_t>{23, 42, 56}),
    test_cpp::test_constants::l_1()
  );
  EXPECT_EQ(
    (std::vector<std::string>{"foo", "bar", "baz"}),
    test_cpp::test_constants::l_2()
  );

  EXPECT_TRUE(test_cpp::test_constants::s_e().empty());
  EXPECT_EQ(
    (std::set<std::int32_t>{23, 42, 56}),
    test_cpp::test_constants::s_1()
  );
  EXPECT_EQ(
    (std::set<std::string>{"foo", "bar", "baz"}),
    test_cpp::test_constants::s_2()
  );

  EXPECT_TRUE(test_cpp::test_constants::m_e().empty());
  EXPECT_EQ(
    (std::map<std::int32_t, std::int32_t>{{23, 97}, {42, 37}, {56, 11}}),
    test_cpp::test_constants::m_1()
  );
  EXPECT_EQ(
    (std::map<std::string, std::string>{{"foo", "bar"}, {"baz", "gaz"}}),
    test_cpp::test_constants::m_2()
  );
  EXPECT_EQ(
    (std::map<std::string, std::int32_t>{
     {"'", 39}, {"\"", 34}, {"\\", 92}, {"a", 97}}),
    test_cpp::test_constants::m_3()
  );

  EXPECT_EQ(test_cpp::struct1(), test_cpp::test_constants::pod_0());

  test_cpp::struct1 pod1;
  pod1.a = 10;
  pod1.b = "foo";

  EXPECT_TRUE(test_cpp::test_constants::pod_1().__isset.a);
  EXPECT_TRUE(test_cpp::test_constants::pod_1().__isset.b);
  EXPECT_EQ(pod1, test_cpp::test_constants::pod_1());

  test_cpp::struct2 pod2;
  pod2.a = 98;
  pod2.b = "gaz";
  pod2.c.a = 12;
  pod2.c.b = "bar";
  pod2.d = {11, 22, 33};

  EXPECT_TRUE(test_cpp::test_constants::pod_2().__isset.a);
  EXPECT_TRUE(test_cpp::test_constants::pod_2().__isset.b);
  EXPECT_TRUE(test_cpp::test_constants::pod_2().__isset.c);
  EXPECT_TRUE(test_cpp::test_constants::pod_2().c.__isset.a);
  EXPECT_TRUE(test_cpp::test_constants::pod_2().c.__isset.b);
  EXPECT_TRUE(test_cpp::test_constants::pod_2().__isset.d);
  EXPECT_EQ(pod2, test_cpp::test_constants::pod_2());

  EXPECT_EQ(
    test_cpp::union1::Type::i,
    test_cpp::test_constants::u_1_1().getType()
  );
  EXPECT_EQ(97, test_cpp::test_constants::u_1_1().get_i());

  EXPECT_EQ(
    test_cpp::union1::Type::d,
    test_cpp::test_constants::u_1_2().getType()
  );
  EXPECT_EQ(5.6, test_cpp::test_constants::u_1_2().get_d());

  EXPECT_EQ(
    test_cpp::union1::Type::__EMPTY__,
    test_cpp::test_constants::u_1_3().getType()
  );

  EXPECT_EQ(
    test_cpp::union2::Type::i,
    test_cpp::test_constants::u_2_1().getType()
  );
  EXPECT_EQ(51, test_cpp::test_constants::u_2_1().get_i());

  EXPECT_EQ(
    test_cpp::union2::Type::d,
    test_cpp::test_constants::u_2_2().getType()
  );
  EXPECT_EQ(6.7, test_cpp::test_constants::u_2_2().get_d());

  EXPECT_EQ(
    test_cpp::union2::Type::s,
    test_cpp::test_constants::u_2_3().getType()
  );
  EXPECT_EQ(8, test_cpp::test_constants::u_2_3().get_s().a);
  EXPECT_TRUE(test_cpp::test_constants::u_2_3().get_s().__isset.a);
  EXPECT_EQ("abacabb", test_cpp::test_constants::u_2_3().get_s().b);
  EXPECT_TRUE(test_cpp::test_constants::u_2_3().get_s().__isset.b);

  EXPECT_EQ(
    test_cpp::union2::Type::u,
    test_cpp::test_constants::u_2_4().getType()
  );
  EXPECT_EQ(
    test_cpp::union1::Type::i,
    test_cpp::test_constants::u_2_4().get_u().getType()
  );
  EXPECT_EQ(43, test_cpp::test_constants::u_2_4().get_u().get_i());

  EXPECT_EQ(
    test_cpp::union2::Type::u,
    test_cpp::test_constants::u_2_5().getType()
  );
  EXPECT_EQ(
    test_cpp::union1::Type::d,
    test_cpp::test_constants::u_2_5().get_u().getType()
  );
  EXPECT_EQ(9.8, test_cpp::test_constants::u_2_5().get_u().get_d());

  EXPECT_EQ(
    test_cpp::union2::Type::u,
    test_cpp::test_constants::u_2_6().getType()
  );
  EXPECT_EQ(
    test_cpp::union1::Type::__EMPTY__,
    test_cpp::test_constants::u_2_6().get_u().getType()
  );
}

TEST(constants, cpp2) {
  EXPECT_EQ(test_cpp2::enum1::field0, test_cpp2::test_constants::e_1_);
  EXPECT_EQ(test_cpp2::enum1::field0, test_cpp2::test_constants::e_1());
  EXPECT_EQ(test_cpp2::enum1::field2, test_cpp2::test_constants::e_2_);
  EXPECT_EQ(test_cpp2::enum1::field2, test_cpp2::test_constants::e_2());

  EXPECT_EQ(72, test_cpp2::test_constants::i_1_);
  EXPECT_EQ(72, test_cpp2::test_constants::i_1());
  EXPECT_EQ(99, test_cpp2::test_constants::i_2_);
  EXPECT_EQ(99, test_cpp2::test_constants::i_2());

  EXPECT_EQ(std::string(), test_cpp2::test_constants::str_e_);
  EXPECT_EQ(std::string(), test_cpp2::test_constants::str_e());
  EXPECT_EQ(std::string("hello"), test_cpp2::test_constants::str_1_);
  EXPECT_EQ(std::string("hello"), test_cpp2::test_constants::str_1());
  EXPECT_EQ(std::string("world"), test_cpp2::test_constants::str_2_);
  EXPECT_EQ(std::string("world"), test_cpp2::test_constants::str_2());
  EXPECT_EQ(std::string("'"), test_cpp2::test_constants::str_3());
  EXPECT_EQ(std::string("\"foo\""), test_cpp2::test_constants::str_4());

  EXPECT_TRUE(test_cpp2::test_constants::l_e().empty());
  EXPECT_EQ(
    (std::vector<std::int32_t>{23, 42, 56}),
    test_cpp2::test_constants::l_1()
  );
  EXPECT_EQ(
    (std::vector<std::string>{"foo", "bar", "baz"}),
    test_cpp2::test_constants::l_2()
  );

  EXPECT_TRUE(test_cpp2::test_constants::s_e().empty());
  EXPECT_EQ(
    (std::set<std::int32_t>{23, 42, 56}),
    test_cpp2::test_constants::s_1()
  );
  EXPECT_EQ(
    (std::set<std::string>{"foo", "bar", "baz"}),
    test_cpp2::test_constants::s_2()
  );

  EXPECT_TRUE(test_cpp2::test_constants::m_e().empty());
  EXPECT_EQ(
    (std::map<std::int32_t, std::int32_t>{{23, 97}, {42, 37}, {56, 11}}),
    test_cpp2::test_constants::m_1()
  );
  EXPECT_EQ(
    (std::map<std::string, std::string>{{"foo", "bar"}, {"baz", "gaz"}}),
    test_cpp2::test_constants::m_2()
  );
  EXPECT_EQ(
    (std::map<std::string, std::int32_t>{
     {"'", 39}, {"\"", 34}, {"\\", 92}, {"a", 97}}),
    test_cpp2::test_constants::m_3()
  );

  EXPECT_EQ(test_cpp2::struct1(), test_cpp2::test_constants::pod_0());

  test_cpp2::struct1 pod1;
  pod1.set_a(10);
  pod1.set_b("foo");

  auto const& pod_1 = test_cpp2::test_constants::pod_1();
  EXPECT_TRUE(pod_1.__isset.a);
  EXPECT_TRUE(pod_1.__isset.b);
  EXPECT_EQ(pod1, pod_1);

  test_cpp2::struct2 pod2;
  pod2.set_a(98);
  pod2.set_b("gaz");
  auto& pod2_c = pod2.set_c(test_cpp2::struct1());
  pod2_c.set_a(12);
  pod2_c.set_b("bar");
  pod2.set_d(std::vector<std::int32_t>{11, 22, 33});

  auto const& pod_2 = test_cpp2::test_constants::pod_2();
  EXPECT_TRUE(pod_2.__isset.a);
  EXPECT_TRUE(pod_2.__isset.b);
  EXPECT_TRUE(pod_2.__isset.c);
  EXPECT_TRUE(pod_2.__isset.d);
  EXPECT_TRUE(pod_2.c.__isset.a);
  EXPECT_TRUE(pod_2.c.__isset.b);
  EXPECT_EQ(pod2, pod_2);

  auto const& pod_3 = test_cpp2::test_constants::pod_3();
  EXPECT_TRUE(pod_3.__isset.a);
  EXPECT_TRUE(pod_3.__isset.b);
  EXPECT_TRUE(pod_3.__isset.c);
  EXPECT_TRUE(pod_3.c.__isset.a);
  EXPECT_FALSE(pod_3.c.__isset.b);
  EXPECT_TRUE(pod_3.c.__isset.c);
  EXPECT_FALSE(pod_3.c.__isset.d);
  EXPECT_FALSE(pod_3.c.c.__isset.a);
  EXPECT_TRUE(pod_3.c.c.__isset.b);

  EXPECT_EQ(
    test_cpp2::union1::Type::i,
    test_cpp2::test_constants::u_1_1().getType()
  );
  EXPECT_EQ(97, test_cpp2::test_constants::u_1_1().get_i());

  EXPECT_EQ(
    test_cpp2::union1::Type::d,
    test_cpp2::test_constants::u_1_2().getType()
  );
  EXPECT_EQ(5.6, test_cpp2::test_constants::u_1_2().get_d());

  EXPECT_EQ(
    test_cpp2::union1::Type::__EMPTY__,
    test_cpp2::test_constants::u_1_3().getType()
  );

  EXPECT_EQ(
    test_cpp2::union2::Type::i,
    test_cpp2::test_constants::u_2_1().getType()
  );
  EXPECT_EQ(51, test_cpp2::test_constants::u_2_1().get_i());

  EXPECT_EQ(
    test_cpp2::union2::Type::d,
    test_cpp2::test_constants::u_2_2().getType()
  );
  EXPECT_EQ(6.7, test_cpp2::test_constants::u_2_2().get_d());

  EXPECT_EQ(
    test_cpp2::union2::Type::s,
    test_cpp2::test_constants::u_2_3().getType()
  );
  EXPECT_EQ(8, test_cpp2::test_constants::u_2_3().get_s().get_a());
  EXPECT_EQ("abacabb", test_cpp2::test_constants::u_2_3().get_s().get_b());

  EXPECT_EQ(
    test_cpp2::union2::Type::u,
    test_cpp2::test_constants::u_2_4().getType()
  );
  EXPECT_EQ(
    test_cpp2::union1::Type::i,
    test_cpp2::test_constants::u_2_4().get_u().getType()
  );
  EXPECT_EQ(43, test_cpp2::test_constants::u_2_4().get_u().get_i());

  EXPECT_EQ(
    test_cpp2::union2::Type::u,
    test_cpp2::test_constants::u_2_5().getType()
  );
  EXPECT_EQ(
    test_cpp2::union1::Type::d,
    test_cpp2::test_constants::u_2_5().get_u().getType()
  );
  EXPECT_EQ(9.8, test_cpp2::test_constants::u_2_5().get_u().get_d());

  EXPECT_EQ(
    test_cpp2::union2::Type::u,
    test_cpp2::test_constants::u_2_6().getType()
  );
  EXPECT_EQ(
    test_cpp2::union1::Type::__EMPTY__,
    test_cpp2::test_constants::u_2_6().get_u().getType()
  );
}
