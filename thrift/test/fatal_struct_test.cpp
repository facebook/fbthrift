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

FATAL_S(struct1s, "struct1");
FATAL_S(field0s, "field0");
FATAL_S(field1s, "field1");
FATAL_S(field2s, "field2");
FATAL_S(field3s, "field3");
FATAL_S(field4s, "field4");
FATAL_S(field5s, "field5");
FATAL_S(fieldAs, "fieldA");
FATAL_S(fieldBs, "fieldB");
FATAL_S(fieldCs, "fieldC");
FATAL_S(fieldDs, "fieldD");
FATAL_S(fieldEs, "fieldE");
FATAL_S(fieldFs, "fieldF");
FATAL_S(fieldGs, "fieldG");

template <apache::thrift::field_id_t Id>
using field_id = std::integral_constant<apache::thrift::field_id_t, Id>;

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

  EXPECT_SAME<field0s, traits::member::field0::name>();
  EXPECT_SAME<field1s, traits::member::field1::name>();
  EXPECT_SAME<field2s, traits::member::field2::name>();
  EXPECT_SAME<field3s, traits::member::field3::name>();
  EXPECT_SAME<field4s, traits::member::field4::name>();
  EXPECT_SAME<field5s, traits::member::field5::name>();

  struct1 pod;
  pod.field0 = 19;
  pod.field1 = "hello";
  pod.field2 = enum1::field2;
  pod.field3 = enum2::field1_2;
  pod.field4.set_ud(5.6);
  pod.field5.set_us_2("world");

  EXPECT_EQ(
    pod.field0,
    traits::member::field0::getter::ref(pod)
  );
  EXPECT_EQ(
    pod.field1,
    traits::member::field1::getter::ref(pod)
  );
  EXPECT_EQ(
    pod.field2,
    traits::member::field2::getter::ref(pod)
  );
  EXPECT_EQ(
    pod.field3,
    traits::member::field3::getter::ref(pod)
  );
  EXPECT_EQ(
    pod.field4,
    traits::member::field4::getter::ref(pod)
  );
  EXPECT_EQ(
    pod.field5,
    traits::member::field5::getter::ref(pod)
  );

  traits::member::field0::getter::ref(pod) = 98;
  EXPECT_EQ(98, pod.field0);

  traits::member::field1::getter::ref(pod) = "test";
  EXPECT_EQ("test", pod.field1);

  traits::member::field2::getter::ref(pod) = enum1::field0;
  EXPECT_EQ(enum1::field0, pod.field2);

  traits::member::field3::getter::ref(pod) = enum2::field2_2;
  EXPECT_EQ(enum2::field2_2, pod.field3);

  traits::member::field4::getter::ref(pod).set_ui(56);
  EXPECT_EQ(union1::Type::ui, pod.field4.getType());
  EXPECT_EQ(56, pod.field4.get_ui());

  traits::member::field5::getter::ref(pod).set_ue_2(enum1::field1);
  EXPECT_EQ(union2::Type::ue_2, pod.field5.getType());
  EXPECT_EQ(enum1::field1, pod.field5.get_ue_2());

  EXPECT_SAME<
    field0s,
    fatal::get<traits::members, field0s, fatal::get_type::name>::name
  >();
  EXPECT_SAME<
    field1s,
    fatal::get<traits::members, field1s, fatal::get_type::name>::name
  >();
  EXPECT_SAME<
    field2s,
    fatal::get<traits::members, field2s, fatal::get_type::name>::name
  >();
  EXPECT_SAME<
    field3s,
    fatal::get<traits::members, field3s, fatal::get_type::name>::name
  >();
  EXPECT_SAME<
    field4s,
    fatal::get<traits::members, field4s, fatal::get_type::name>::name
  >();
  EXPECT_SAME<
    field5s,
    fatal::get<traits::members, field5s, fatal::get_type::name>::name
  >();

  EXPECT_SAME<
    std::int32_t,
    fatal::get<traits::members, field0s, fatal::get_type::name>::type
  >();
  EXPECT_SAME<
    std::string,
    fatal::get<traits::members, field1s, fatal::get_type::name>::type
  >();
  EXPECT_SAME<
    enum1,
    fatal::get<traits::members, field2s, fatal::get_type::name>::type
  >();
  EXPECT_SAME<
    enum2,
    fatal::get<traits::members, field3s, fatal::get_type::name>::type
  >();
  EXPECT_SAME<
    union1,
    fatal::get<traits::members, field4s, fatal::get_type::name>::type
  >();
  EXPECT_SAME<
    union2,
    fatal::get<traits::members, field5s, fatal::get_type::name>::type
  >();

  EXPECT_SAME<
    field_id<1>,
    fatal::get<traits::members, field0s, fatal::get_type::name>::id
  >();
  EXPECT_SAME<
    field_id<2>,
    fatal::get<traits::members, field1s, fatal::get_type::name>::id
  >();
  EXPECT_SAME<
    field_id<3>,
    fatal::get<traits::members, field2s, fatal::get_type::name>::id
  >();
  EXPECT_SAME<
    field_id<4>,
    fatal::get<traits::members, field3s, fatal::get_type::name>::id
  >();
  EXPECT_SAME<
    field_id<5>,
    fatal::get<traits::members, field4s, fatal::get_type::name>::id
  >();
  EXPECT_SAME<
    field_id<6>,
    fatal::get<traits::members, field5s, fatal::get_type::name>::id
  >();

  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    fatal::get<traits::members, field0s, fatal::get_type::name>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    fatal::get<traits::members, field1s, fatal::get_type::name>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    fatal::get<traits::members, field2s, fatal::get_type::name>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    fatal::get<traits::members, field3s, fatal::get_type::name>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    fatal::get<traits::members, field4s, fatal::get_type::name>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    fatal::get<traits::members, field5s, fatal::get_type::name>::optional
  >();

  EXPECT_EQ(
    98,
    ( fatal::get<
        traits::members,
        traits::member::field0::name,
        fatal::get_type::name
      >::getter::ref(pod))
  );
  EXPECT_EQ(
    "test",
    ( fatal::get<
        traits::members,
        traits::member::field1::name,
        fatal::get_type::name
      >::getter::ref(pod))
  );
  EXPECT_EQ(
    enum1::field0,
    ( fatal::get<
        traits::members,
        traits::member::field2::name,
        fatal::get_type::name
      >::getter::ref(pod))
  );
  EXPECT_EQ(
    enum2::field2_2,
    ( fatal::get<
        traits::members,
        traits::member::field3::name,
        fatal::get_type::name
      >::getter::ref(pod))
  );
  EXPECT_EQ(
    union1::Type::ui,
    ( fatal::get<
        traits::members,
        traits::member::field4::name,
        fatal::get_type::name
      >::getter::ref(pod)
      .getType())
  );
  EXPECT_EQ(
    56,
    ( fatal::get<
        traits::members,
        traits::member::field4::name,
        fatal::get_type::name
      >::getter::ref(pod)
      .get_ui())
  );
  EXPECT_EQ(
    union2::Type::ue_2,
    ( fatal::get<
        traits::members,
        traits::member::field5::name,
        fatal::get_type::name
      >::getter::ref(pod)
      .getType())
  );
  EXPECT_EQ(
    enum1::field1,
    ( fatal::get<
        traits::members,
        traits::member::field5::name,
        fatal::get_type::name
      >::getter::ref(pod)
      .get_ue_2())
  );

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    fatal::get<traits::members, field0s, fatal::get_type::name>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::string,
    fatal::get<traits::members, field1s, fatal::get_type::name>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    fatal::get<traits::members, field2s, fatal::get_type::name>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    fatal::get<traits::members, field3s, fatal::get_type::name>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    fatal::get<traits::members, field4s, fatal::get_type::name>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    fatal::get<traits::members, field5s, fatal::get_type::name>::type_class
  >();

  EXPECT_SAME<
    std::int32_t,
    decltype(std::declval<
      fatal::get<traits::members, field0s, fatal::get_type::name>::pod<>
    >().field0)
  >();
  EXPECT_SAME<
    std::string,
    decltype(std::declval<
      fatal::get<traits::members, field1s, fatal::get_type::name>::pod<>
    >().field1)
  >();
  EXPECT_SAME<
    enum1,
    decltype(std::declval<
      fatal::get<traits::members, field2s, fatal::get_type::name>::pod<>
    >().field2)
  >();
  EXPECT_SAME<
    enum2,
    decltype(std::declval<
      fatal::get<traits::members, field3s, fatal::get_type::name>::pod<>
    >().field3)
  >();
  EXPECT_SAME<
    union1,
    decltype(std::declval<
      fatal::get<traits::members, field4s, fatal::get_type::name>::pod<>
    >().field4)
  >();
  EXPECT_SAME<
    union2,
    decltype(std::declval<
      fatal::get<traits::members, field5s, fatal::get_type::name>::pod<>
    >().field5)
  >();

  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field0s, fatal::get_type::name>::pod<bool>
    >().field0)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field1s, fatal::get_type::name>::pod<bool>
    >().field1)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field2s, fatal::get_type::name>::pod<bool>
    >().field2)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field3s, fatal::get_type::name>::pod<bool>
    >().field3)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field4s, fatal::get_type::name>::pod<bool>
    >().field4)
  >();
  EXPECT_SAME<
    bool,
    decltype(std::declval<
      fatal::get<traits::members, field5s, fatal::get_type::name>::pod<bool>
    >().field5)
  >();

  EXPECT_SAME<
    traits::member::field0,
    fatal::get<
      traits::members, traits::member::field0::name, fatal::get_type::name
    >
  >();
  EXPECT_SAME<
    traits::member::field1,
    fatal::get<
      traits::members, traits::member::field1::name, fatal::get_type::name
    >
  >();
  EXPECT_SAME<
    traits::member::field2,
    fatal::get<
      traits::members, traits::member::field2::name, fatal::get_type::name
    >
  >();
  EXPECT_SAME<
    traits::member::field3,
    fatal::get<
      traits::members, traits::member::field3::name, fatal::get_type::name
    >
  >();
  EXPECT_SAME<
    traits::member::field4,
    fatal::get<
      traits::members, traits::member::field4::name, fatal::get_type::name
    >
  >();
  EXPECT_SAME<
    traits::member::field5,
    fatal::get<
      traits::members, traits::member::field5::name, fatal::get_type::name
    >
  >();
}

FATAL_S(structB_annotation1k, "multi_line_annotation");
FATAL_S(structB_annotation1v, "line one\nline two");
FATAL_S(structB_annotation2k, "some.annotation");
FATAL_S(structB_annotation2v, "this is its value");
FATAL_S(structB_annotation3k, "some.other.annotation");
FATAL_S(structB_annotation3v, "this is its other value");

TEST(fatal_struct, annotations) {
  EXPECT_SAME<
    fatal::list<>,
    apache::thrift::reflect_struct<struct1>::annotations::map
  >();

  EXPECT_SAME<
    fatal::list<>,
    apache::thrift::reflect_struct<struct2>::annotations::map
  >();

  EXPECT_SAME<
    fatal::list<>,
    apache::thrift::reflect_struct<struct3>::annotations::map
  >();

  EXPECT_SAME<
    fatal::list<>,
    apache::thrift::reflect_struct<structA>::annotations::map
  >();

  using structB_annotations = apache::thrift::reflect_struct<structB>
    ::annotations;

  EXPECT_SAME<
    structB_annotation1k,
    structB_annotations::keys::multi_line_annotation
  >();
  EXPECT_SAME<
    structB_annotation1v,
    structB_annotations::values::multi_line_annotation
  >();
  EXPECT_SAME<
    structB_annotation2k,
    structB_annotations::keys::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation2v,
    structB_annotations::values::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation3k,
    structB_annotations::keys::some_other_annotation
  >();
  EXPECT_SAME<
    structB_annotation3v,
    structB_annotations::values::some_other_annotation
  >();
  EXPECT_SAME<
    fatal::list<
      fatal::pair<structB_annotation1k, structB_annotation1v>,
      fatal::pair<structB_annotation2k, structB_annotation2v>,
      fatal::pair<structB_annotation3k, structB_annotation3v>
    >,
    structB_annotations::map
  >();

  EXPECT_SAME<
    fatal::list<>,
    apache::thrift::reflect_struct<structC>::annotations::map
  >();
}

FATAL_S(structBd_annotation1k, "another.annotation");
FATAL_S(structBd_annotation1v, "another value");
FATAL_S(structBd_annotation2k, "some.annotation");
FATAL_S(structBd_annotation2v, "some value");

TEST(fatal_struct, member_annotations) {
  using info = apache::thrift::reflect_struct<structB>;

  EXPECT_SAME<fatal::list<>, info::members_annotations::c::map>();
  EXPECT_SAME<
    fatal::list<>,
    fatal::get<
      info::members, info::member::c::name, fatal::get_type::name
    >::annotations::map
  >();

  using annotations_d = fatal::get<
    info::members, info::member::d::name, fatal::get_type::name
  >::annotations;
  using expected_d_map = fatal::list<
    fatal::pair<structBd_annotation1k, structBd_annotation1v>,
    fatal::pair<structBd_annotation2k, structBd_annotation2v>
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

TEST(fatal_struct, set_methods) {
  using info = apache::thrift::reflect_struct<struct4>;
  using req_field = fatal::get<
    info::members, info::member::field0::name, fatal::get_type::name
  >;
  using opt_field = fatal::get<
    info::members, info::member::field1::name, fatal::get_type::name
  >;
  using def_field = fatal::get<
    info::members, info::member::field2::name, fatal::get_type::name
  >;

  using ref_field = fatal::get<
    info::members, info::member::field3::name, fatal::get_type::name
  >;

  struct4 a;
  EXPECT_EQ(true, req_field::is_set(a));
  req_field::mark_set(a);
  EXPECT_EQ(true, req_field::is_set(a));
  // can't test that req_field::unmark_set doesn't compile,
  // but trust me, it shoudln't
  // req_field::unmark_set(a);

  EXPECT_EQ(false, opt_field::is_set(a));
  opt_field::mark_set(a);
  EXPECT_EQ(true, opt_field::is_set(a));
  opt_field::unmark_set(a);
  EXPECT_EQ(false, opt_field::is_set(a));

  EXPECT_EQ(false, def_field::is_set(a));
  def_field::mark_set(a);
  EXPECT_EQ(true, def_field::is_set(a));
  def_field::unmark_set(a);
  EXPECT_EQ(false, def_field::is_set(a));

  EXPECT_EQ(true, ref_field::is_set(a));
  ref_field::mark_set(a);
  EXPECT_EQ(true, req_field::is_set(a));
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
