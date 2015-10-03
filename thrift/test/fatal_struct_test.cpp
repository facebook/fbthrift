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

  EXPECT_SAME<traits, apache::thrift::try_reflect_struct<struct1, void>>();
  EXPECT_SAME<void, apache::thrift::try_reflect_struct<int, void>>();

  EXPECT_SAME<field0s, traits::names::field0>();
  EXPECT_SAME<field1s, traits::names::field1>();
  EXPECT_SAME<field2s, traits::names::field2>();
  EXPECT_SAME<field3s, traits::names::field3>();
  EXPECT_SAME<field4s, traits::names::field4>();
  EXPECT_SAME<field5s, traits::names::field5>();

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
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
