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

#include <thrift/test/gen-cpp2/reflection_fatal_union.h>

#include <thrift/test/expect_same.h>

#include <gtest/gtest.h>

#include <fatal/type/transform.h>

namespace test_cpp2 {
namespace cpp_reflection {

FATAL_STR(uis, "ui");
FATAL_STR(uds, "ud");
FATAL_STR(uss, "us");
FATAL_STR(ues, "ue");

using uii = std::integral_constant<union1::Type, union1::Type::ui>;
using udi = std::integral_constant<union1::Type, union1::Type::ud>;
using usi = std::integral_constant<union1::Type, union1::Type::us>;
using uei = std::integral_constant<union1::Type, union1::Type::ue>;

TEST(fatal_union, variants) {
  using traits = fatal::variant_traits<union1>;

  EXPECT_SAME<union1, traits::type>();
  EXPECT_SAME<union1::Type, traits::id>();

  EXPECT_SAME<uis, traits::names::ui>();
  EXPECT_SAME<uds, traits::names::ud>();
  EXPECT_SAME<uss, traits::names::us>();
  EXPECT_SAME<ues, traits::names::ue>();

  EXPECT_SAME<uii, traits::ids::ui>();
  EXPECT_SAME<udi, traits::ids::ud>();
  EXPECT_SAME<usi, traits::ids::us>();
  EXPECT_SAME<uei, traits::ids::ue>();

  EXPECT_SAME<
    fatal::type_list<uis, uds, uss, ues>,
    traits::descriptors::transform<fatal::get_member_type::name>
  >();

  EXPECT_SAME<
    fatal::type_list<uii, udi, usi, uei>,
    traits::descriptors::transform<fatal::get_member_type::id>
  >();

  EXPECT_SAME<
    fatal::type_list<std::int32_t, double, std::string, enum1>,
    traits::descriptors::transform<fatal::get_member_type::type>
  >();
}

TEST(fatal_union, by_name) {
  using traits = fatal::variant_traits<union1>::by_name;

  EXPECT_SAME<fatal::type_list<uis, uds, uss, ues>, traits::tags>();

  EXPECT_SAME<uis, traits::name<uis>>();
  EXPECT_SAME<uds, traits::name<uds>>();
  EXPECT_SAME<uss, traits::name<uss>>();
  EXPECT_SAME<ues, traits::name<ues>>();

  EXPECT_SAME<uii, traits::id<uis>>();
  EXPECT_SAME<udi, traits::id<uds>>();
  EXPECT_SAME<usi, traits::id<uss>>();
  EXPECT_SAME<uei, traits::id<ues>>();

  EXPECT_SAME<std::int32_t, traits::type<uis>>();
  EXPECT_SAME<double, traits::type<uds>>();
  EXPECT_SAME<std::string, traits::type<uss>>();
  EXPECT_SAME<enum1, traits::type<ues>>();

  union1 u;
  union1 const &uc = u;
  union1 &ul = u;
  union1 &&ur = std::move(u);

  traits::set<uis>(u, 10);
  EXPECT_EQ(union1::Type::ui, u.getType());
  EXPECT_EQ(10, u.get_ui());
  EXPECT_EQ(10, traits::get<uis>(ul));
  EXPECT_EQ(10, traits::get<uis>(uc));
  EXPECT_EQ(10, traits::get<uis>(ur));

  traits::set<uds>(u, 5.6);
  EXPECT_EQ(union1::Type::ud, u.getType());
  EXPECT_EQ(5.6, u.get_ud());
  EXPECT_EQ(5.6, traits::get<uds>(ul));
  EXPECT_EQ(5.6, traits::get<uds>(uc));
  EXPECT_EQ(5.6, traits::get<uds>(ur));

  traits::set<uss>(u, "hello");
  EXPECT_EQ(union1::Type::us, u.getType());
  EXPECT_EQ("hello", u.get_us());
  EXPECT_EQ("hello", traits::get<uss>(ul));
  EXPECT_EQ("hello", traits::get<uss>(uc));
  EXPECT_EQ("hello", traits::get<uss>(ur));

  traits::set<ues>(u, enum1::field1);
  EXPECT_EQ(union1::Type::ue, u.getType());
  EXPECT_EQ(enum1::field1, u.get_ue());
  EXPECT_EQ(enum1::field1, traits::get<ues>(ul));
  EXPECT_EQ(enum1::field1, traits::get<ues>(uc));
  EXPECT_EQ(enum1::field1, traits::get<ues>(ur));
}

TEST(fatal_union, by_id) {
  using traits = fatal::variant_traits<union1>::by_id;

  EXPECT_SAME<fatal::type_list<uii, udi, usi, uei>, traits::tags>();

  EXPECT_SAME<uis, traits::name<uii>>();
  EXPECT_SAME<uds, traits::name<udi>>();
  EXPECT_SAME<uss, traits::name<usi>>();
  EXPECT_SAME<ues, traits::name<uei>>();

  EXPECT_SAME<uii, traits::id<uii>>();
  EXPECT_SAME<udi, traits::id<udi>>();
  EXPECT_SAME<usi, traits::id<usi>>();
  EXPECT_SAME<uei, traits::id<uei>>();

  EXPECT_SAME<std::int32_t, traits::type<uii>>();
  EXPECT_SAME<double, traits::type<udi>>();
  EXPECT_SAME<std::string, traits::type<usi>>();
  EXPECT_SAME<enum1, traits::type<uei>>();

  union1 u;
  union1 const &uc = u;
  union1 &ul = u;
  union1 &&ur = std::move(u);

  traits::set<uii>(u, 10);
  EXPECT_EQ(union1::Type::ui, u.getType());
  EXPECT_EQ(10, u.get_ui());
  EXPECT_EQ(10, traits::get<uii>(ul));
  EXPECT_EQ(10, traits::get<uii>(uc));
  EXPECT_EQ(10, traits::get<uii>(ur));

  traits::set<udi>(u, 5.6);
  EXPECT_EQ(union1::Type::ud, u.getType());
  EXPECT_EQ(5.6, u.get_ud());
  EXPECT_EQ(5.6, traits::get<udi>(ul));
  EXPECT_EQ(5.6, traits::get<udi>(uc));
  EXPECT_EQ(5.6, traits::get<udi>(ur));

  traits::set<usi>(u, "hello");
  EXPECT_EQ(union1::Type::us, u.getType());
  EXPECT_EQ("hello", u.get_us());
  EXPECT_EQ("hello", traits::get<usi>(ul));
  EXPECT_EQ("hello", traits::get<usi>(uc));
  EXPECT_EQ("hello", traits::get<usi>(ur));

  traits::set<uei>(u, enum1::field1);
  EXPECT_EQ(union1::Type::ue, u.getType());
  EXPECT_EQ(enum1::field1, u.get_ue());
  EXPECT_EQ(enum1::field1, traits::get<uei>(ul));
  EXPECT_EQ(enum1::field1, traits::get<uei>(uc));
  EXPECT_EQ(enum1::field1, traits::get<uei>(ur));
}

TEST(fatal_union, by_type) {
  using traits = fatal::variant_traits<union1>::by_type;

  EXPECT_SAME<
    fatal::type_list<std::int32_t, double, std::string, enum1>,
    traits::tags
  >();

  EXPECT_SAME<uis, traits::name<std::int32_t>>();
  EXPECT_SAME<uds, traits::name<double>>();
  EXPECT_SAME<uss, traits::name<std::string>>();
  EXPECT_SAME<ues, traits::name<enum1>>();

  EXPECT_SAME<uii, traits::id<std::int32_t>>();
  EXPECT_SAME<udi, traits::id<double>>();
  EXPECT_SAME<usi, traits::id<std::string>>();
  EXPECT_SAME<uei, traits::id<enum1>>();

  EXPECT_SAME<std::int32_t, traits::type<std::int32_t>>();
  EXPECT_SAME<double, traits::type<double>>();
  EXPECT_SAME<std::string, traits::type<std::string>>();
  EXPECT_SAME<enum1, traits::type<enum1>>();

  union1 u;
  union1 const &uc = u;
  union1 &ul = u;
  union1 &&ur = std::move(u);

  traits::set<std::int32_t>(u, 10);
  EXPECT_EQ(union1::Type::ui, u.getType());
  EXPECT_EQ(10, u.get_ui());
  EXPECT_EQ(10, traits::get<std::int32_t>(ul));
  EXPECT_EQ(10, traits::get<std::int32_t>(uc));
  EXPECT_EQ(10, traits::get<std::int32_t>(ur));

  traits::set<double>(u, 5.6);
  EXPECT_EQ(union1::Type::ud, u.getType());
  EXPECT_EQ(5.6, u.get_ud());
  EXPECT_EQ(5.6, traits::get<double>(ul));
  EXPECT_EQ(5.6, traits::get<double>(uc));
  EXPECT_EQ(5.6, traits::get<double>(ur));

  traits::set<std::string>(u, "hello");
  EXPECT_EQ(union1::Type::us, u.getType());
  EXPECT_EQ("hello", u.get_us());
  EXPECT_EQ("hello", traits::get<std::string>(ul));
  EXPECT_EQ("hello", traits::get<std::string>(uc));
  EXPECT_EQ("hello", traits::get<std::string>(ur));

  traits::set<enum1>(u, enum1::field1);
  EXPECT_EQ(union1::Type::ue, u.getType());
  EXPECT_EQ(enum1::field1, u.get_ue());
  EXPECT_EQ(enum1::field1, traits::get<enum1>(ul));
  EXPECT_EQ(enum1::field1, traits::get<enum1>(uc));
  EXPECT_EQ(enum1::field1, traits::get<enum1>(ur));
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
