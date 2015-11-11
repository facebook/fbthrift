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

#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>

#include <thrift/test/expect_same.h>

#include <gtest/gtest.h>

FATAL_STR(struct1s, "struct1");
FATAL_STR(field0s, "field0");
FATAL_STR(field1s, "field1");
FATAL_STR(field2s, "field2");
FATAL_STR(field3s, "field3");
FATAL_STR(field4s, "field4");
FATAL_STR(field5s, "field5");
FATAL_STR(fieldAs, "fieldA");
FATAL_STR(fieldBs, "fieldB");
FATAL_STR(fieldCs, "fieldC");
FATAL_STR(fieldDs, "fieldD");
FATAL_STR(fieldEs, "fieldE");
FATAL_STR(fieldFs, "fieldF");
FATAL_STR(fieldGs, "fieldG");

namespace test_cpp2 {
namespace cpp_reflection {

TEST(fatal_struct, struct1_sanity_check) {
  using traits = apache::thrift::reflect_struct<struct1>;

  EXPECT_SAME<struct1, traits::type>();
  EXPECT_SAME<struct1s, traits::name>();
  EXPECT_SAME<reflection_tags::metadata, traits::module>();

  EXPECT_SAME<traits, apache::thrift::try_reflect_struct<struct1, void>>();
  EXPECT_SAME<void, apache::thrift::try_reflect_struct<int, void>>();

  EXPECT_SAME<field0s, traits::names::field0>();
  EXPECT_SAME<field1s, traits::names::field1>();
  EXPECT_SAME<field2s, traits::names::field2>();
  EXPECT_SAME<field3s, traits::names::field3>();
  EXPECT_SAME<field4s, traits::names::field4>();
  EXPECT_SAME<field5s, traits::names::field5>();

  EXPECT_SAME<
    fatal::build_type_map<
      field0s, std::int32_t,
      field1s, std::string,
      field2s, enum1,
      field3s, enum2,
      field4s, union1,
      field5s, union2
    >,
    traits::types
  >();

  EXPECT_SAME<
    fatal::build_type_map<
      field0s, std::integral_constant<apache::thrift::field_id_t, 1>,
      field1s, std::integral_constant<apache::thrift::field_id_t, 2>,
      field2s, std::integral_constant<apache::thrift::field_id_t, 3>,
      field3s, std::integral_constant<apache::thrift::field_id_t, 4>,
      field4s, std::integral_constant<apache::thrift::field_id_t, 5>,
      field5s, std::integral_constant<apache::thrift::field_id_t, 6>
    >,
    traits::ids
  >();

  EXPECT_SAME<
    fatal::type_list<
      field0s,
      field1s,
      field2s,
      field3s,
      field4s,
      field5s
    >,
    traits::getters::keys
  >();

  struct1 pod;
  pod.field0 = 19;
  pod.field1 = "hello";
  pod.field2 = enum1::field2;
  pod.field3 = enum2::field1_2;
  pod.field4.set_ud(5.6);
  pod.field5.set_us_2("world");

  EXPECT_EQ(pod.field0, traits::getters::get<traits::names::field0>::ref(pod));
  EXPECT_EQ(pod.field1, traits::getters::get<traits::names::field1>::ref(pod));
  EXPECT_EQ(pod.field2, traits::getters::get<traits::names::field2>::ref(pod));
  EXPECT_EQ(pod.field3, traits::getters::get<traits::names::field3>::ref(pod));
  EXPECT_EQ(pod.field4, traits::getters::get<traits::names::field4>::ref(pod));
  EXPECT_EQ(pod.field5, traits::getters::get<traits::names::field5>::ref(pod));

  traits::getters::get<traits::names::field0>::ref(pod) = 98;
  EXPECT_EQ(98, pod.field0);

  traits::getters::get<traits::names::field1>::ref(pod) = "test";
  EXPECT_EQ("test", pod.field1);

  traits::getters::get<traits::names::field2>::ref(pod) = enum1::field0;
  EXPECT_EQ(enum1::field0, pod.field2);

  traits::getters::get<traits::names::field3>::ref(pod) = enum2::field2_2;
  EXPECT_EQ(enum2::field2_2, pod.field3);

  traits::getters::get<traits::names::field4>::ref(pod).set_ui(56);
  EXPECT_EQ(union1::Type::ui, pod.field4.getType());
  EXPECT_EQ(56, pod.field4.get_ui());

  traits::getters::get<traits::names::field5>::ref(pod).set_ue_2(enum1::field1);
  EXPECT_EQ(union2::Type::ue_2, pod.field5.getType());
  EXPECT_EQ(enum1::field1, pod.field5.get_ue_2());

  EXPECT_SAME<field0s, traits::members::get<field0s>::name>();
  EXPECT_SAME<field1s, traits::members::get<field1s>::name>();
  EXPECT_SAME<field2s, traits::members::get<field2s>::name>();
  EXPECT_SAME<field3s, traits::members::get<field3s>::name>();
  EXPECT_SAME<field4s, traits::members::get<field4s>::name>();
  EXPECT_SAME<field5s, traits::members::get<field5s>::name>();

  EXPECT_SAME<std::int32_t, traits::members::get<field0s>::type>();
  EXPECT_SAME<std::string, traits::members::get<field1s>::type>();
  EXPECT_SAME<enum1, traits::members::get<field2s>::type>();
  EXPECT_SAME<enum2, traits::members::get<field3s>::type>();
  EXPECT_SAME<union1, traits::members::get<field4s>::type>();
  EXPECT_SAME<union2, traits::members::get<field5s>::type>();

  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 1>,
    traits::members::get<field0s>::id
  >();
  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 2>,
    traits::members::get<field1s>::id
  >();
  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 3>,
    traits::members::get<field2s>::id
  >();
  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 4>,
    traits::members::get<field3s>::id
  >();
  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 5>,
    traits::members::get<field4s>::id
  >();
  EXPECT_SAME<
    std::integral_constant<apache::thrift::field_id_t, 6>,
    traits::members::get<field5s>::id
  >();

  EXPECT_EQ(
    98,
    traits::members::get<traits::names::field0>::getter::ref(pod)
  );
  EXPECT_EQ(
    "test",
    traits::members::get<traits::names::field1>::getter::ref(pod)
  );
  EXPECT_EQ(
    enum1::field0,
    traits::members::get<traits::names::field2>::getter::ref(pod)
  );
  EXPECT_EQ(
    enum2::field2_2,
    traits::members::get<traits::names::field3>::getter::ref(pod)
  );
  EXPECT_EQ(
    union1::Type::ui,
    traits::members::get<traits::names::field4>::getter::ref(pod).getType()
  );
  EXPECT_EQ(
    56,
    traits::members::get<traits::names::field4>::getter::ref(pod).get_ui()
  );
  EXPECT_EQ(
    union2::Type::ue_2,
    traits::members::get<traits::names::field5>::getter::ref(pod).getType()
  );
  EXPECT_EQ(
    enum1::field1,
    traits::members::get<traits::names::field5>::getter::ref(pod).get_ue_2()
  );

  EXPECT_EQ(
    apache::thrift::thrift_category::integral,
    traits::members::get<field0s>::category::value
  );
  EXPECT_EQ(
    apache::thrift::thrift_category::string,
    traits::members::get<field1s>::category::value
  );
  EXPECT_EQ(
    apache::thrift::thrift_category::enumeration,
    traits::members::get<field2s>::category::value
  );
  EXPECT_EQ(
    apache::thrift::thrift_category::enumeration,
    traits::members::get<field3s>::category::value
  );
  EXPECT_EQ(
    apache::thrift::thrift_category::variant,
    traits::members::get<field4s>::category::value
  );
  EXPECT_EQ(
    apache::thrift::thrift_category::variant,
    traits::members::get<field5s>::category::value
  );

  EXPECT_SAME<
    std::int32_t,
    decltype(std::declval<traits::members::get<field0s>::pod<>>().field0)
  >();
  EXPECT_SAME<
    std::string,
    decltype(std::declval<traits::members::get<field1s>::pod<>>().field1)
  >();
  EXPECT_SAME<
    enum1,
    decltype(std::declval<traits::members::get<field2s>::pod<>>().field2)
  >();
  EXPECT_SAME<
    enum2,
    decltype(std::declval<traits::members::get<field3s>::pod<>>().field3)
  >();
  EXPECT_SAME<
    union1,
    decltype(std::declval<traits::members::get<field4s>::pod<>>().field4)
  >();
  EXPECT_SAME<
    union2,
    decltype(std::declval<traits::members::get<field5s>::pod<>>().field5)
  >();

  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field0s>::pod<bool>>().field0)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field1s>::pod<bool>>().field1)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field2s>::pod<bool>>().field2)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field3s>::pod<bool>>().field3)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field4s>::pod<bool>>().field4)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<traits::members::get<field5s>::pod<bool>>().field5)
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
