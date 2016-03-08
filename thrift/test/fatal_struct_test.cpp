/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

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

template <apache::thrift::field_id_t Id>
using field_id = std::integral_constant<apache::thrift::field_id_t, Id>;

template <apache::thrift::thrift_category Category>
using category = std::integral_constant<
  apache::thrift::thrift_category,
  Category
>;

template <apache::thrift::optionality Optionality>
using required = std::integral_constant<
  apache::thrift::optionality,
  Optionality
>;

namespace test_cpp2 {
namespace cpp_reflection {

TEST(fatal_struct, struct1_sanity_check) {
  using traits = apache::thrift::reflect_struct<struct1>;

  EXPECT_SAME<struct1, traits::type>();
  EXPECT_SAME<struct1s, traits::name>();
  EXPECT_SAME<reflection_tags::module, traits::module>();

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
      field0s, field_id<1>,
      field1s, field_id<2>,
      field2s, field_id<3>,
      field3s, field_id<4>,
      field4s, field_id<5>,
      field5s, field_id<6>
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

  EXPECT_SAME<field_id<1>, traits::members::get<field0s>::id>();
  EXPECT_SAME<field_id<2>, traits::members::get<field1s>::id>();
  EXPECT_SAME<field_id<3>, traits::members::get<field2s>::id>();
  EXPECT_SAME<field_id<4>, traits::members::get<field3s>::id>();
  EXPECT_SAME<field_id<5>, traits::members::get<field4s>::id>();
  EXPECT_SAME<field_id<6>, traits::members::get<field5s>::id>();

  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    traits::members::get<field0s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    traits::members::get<field1s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    traits::members::get<field2s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    traits::members::get<field3s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    traits::members::get<field4s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    traits::members::get<field5s>::optional
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

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    traits::members::get<field0s>::category
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::string>,
    traits::members::get<field1s>::category
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::enumeration>,
    traits::members::get<field2s>::category
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::enumeration>,
    traits::members::get<field3s>::category
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::variant>,
    traits::members::get<field4s>::category
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::variant>,
    traits::members::get<field5s>::category
  >();

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

FATAL_STR(structB_annotation1k, "some.annotation");
FATAL_STR(structB_annotation1v, "this is its value");
FATAL_STR(structB_annotation2k, "some.other.annotation");
FATAL_STR(structB_annotation2v, "this is its other value");

TEST(fatal_struct, annotations) {
  EXPECT_SAME<
    fatal::type_map<>,
    apache::thrift::reflect_struct<struct1>::annotations::map
  >();

  EXPECT_SAME<
    fatal::type_map<>,
    apache::thrift::reflect_struct<struct2>::annotations::map
  >();

  EXPECT_SAME<
    fatal::type_map<>,
    apache::thrift::reflect_struct<struct3>::annotations::map
  >();

  EXPECT_SAME<
    fatal::type_map<>,
    apache::thrift::reflect_struct<structA>::annotations::map
  >();

  using structB_annotations = apache::thrift::reflect_struct<structB>
    ::annotations;

  EXPECT_SAME<
    structB_annotation1k,
    structB_annotations::keys::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation1v,
    structB_annotations::values::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation2k,
    structB_annotations::keys::some_other_annotation
  >();
  EXPECT_SAME<
    structB_annotation2v,
    structB_annotations::values::some_other_annotation
  >();
  EXPECT_SAME<
    fatal::build_type_map<
      structB_annotation1k, structB_annotation1v,
      structB_annotation2k, structB_annotation2v
    >,
    structB_annotations::map
  >();

  EXPECT_SAME<
    fatal::type_map<>,
    apache::thrift::reflect_struct<structC>::annotations::map
  >();
}

FATAL_STR(structBd_annotation1k, "another.annotation");
FATAL_STR(structBd_annotation1v, "another value");
FATAL_STR(structBd_annotation2k, "some.annotation");
FATAL_STR(structBd_annotation2v, "some value");

TEST(fatal_struct, member_annotations) {
  using info = apache::thrift::reflect_struct<structB>;

  EXPECT_SAME<fatal::build_type_map<>, info::members_annotations::c::map>();
  EXPECT_SAME<
    fatal::build_type_map<>,
    info::members::get<info::names::c>::annotations::map
  >();

  using annotations_d = info::members::get<info::names::d>::annotations;
  using expected_d_map = fatal::build_type_map<
    structBd_annotation1k, structBd_annotation1v,
    structBd_annotation2k, structBd_annotation2v
  >;

  EXPECT_SAME<expected_d_map, info::members_annotations::d::map>();
  EXPECT_SAME<expected_d_map, annotations_d::map>();

  EXPECT_SAME<
    structBd_annotation1k,
    info::members_annotations::d::keys::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation1v,
    info::members_annotations::d::values::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2k,
    info::members_annotations::d::keys::some_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2v,
    info::members_annotations::d::values::some_annotation
  >();

  EXPECT_SAME<
    structBd_annotation1k,
    annotations_d::keys::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation1v,
    annotations_d::values::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2k,
    annotations_d::keys::some_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2v,
    annotations_d::values::some_annotation
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
