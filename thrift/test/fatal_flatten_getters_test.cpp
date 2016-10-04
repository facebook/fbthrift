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

#include <thrift/lib/cpp2/fatal/flatten_getters.h>

#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <gtest/gtest.h>

namespace test_cpp2 {
namespace cpp_reflection {

using namespace apache::thrift;

TEST(flatten_getters, no_terminal_filter) {
  using sa = reflect_struct<structA>::member;
  using sb = reflect_struct<structB>::member;
  using sc = reflect_struct<structC>::member;
  using s1 = reflect_struct<struct1>::member;
  using s2 = reflect_struct<struct2>::member;
  using s3 = reflect_struct<struct3>::member;
  using s4 = reflect_struct<struct4>::member;
  using s5 = reflect_struct<struct5>::member;

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, sa::a>,
      flat_getter<fatal::list<>, sa::b>
    >,
    flatten_getters<structA>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, sb::c>,
      flat_getter<fatal::list<>, sb::d>
    >,
    flatten_getters<structB>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, sc::a>,
      flat_getter<fatal::list<>, sc::b>,
      flat_getter<fatal::list<>, sc::c>,
      flat_getter<fatal::list<>, sc::d>,
      flat_getter<fatal::list<>, sc::e>,
      flat_getter<fatal::list<>, sc::f>,
      flat_getter<fatal::list<>, sc::g>,
      flat_getter<fatal::list<>, sc::h>,
      flat_getter<fatal::list<>, sc::i>,
      flat_getter<fatal::list<>, sc::j>,
      flat_getter<fatal::list<>, sc::j1>,
      flat_getter<fatal::list<>, sc::j2>,
      flat_getter<fatal::list<>, sc::j3>,
      flat_getter<fatal::list<>, sc::k>,
      flat_getter<fatal::list<>, sc::k1>,
      flat_getter<fatal::list<>, sc::k2>,
      flat_getter<fatal::list<>, sc::k3>,
      flat_getter<fatal::list<>, sc::l>,
      flat_getter<fatal::list<>, sc::l1>,
      flat_getter<fatal::list<>, sc::l2>,
      flat_getter<fatal::list<>, sc::l3>,
      flat_getter<fatal::list<>, sc::m1>,
      flat_getter<fatal::list<>, sc::m2>,
      flat_getter<fatal::list<>, sc::m3>,
      flat_getter<fatal::list<>, sc::n1>,
      flat_getter<fatal::list<>, sc::n2>,
      flat_getter<fatal::list<>, sc::n3>,
      flat_getter<fatal::list<>, sc::o1>,
      flat_getter<fatal::list<>, sc::o2>,
      flat_getter<fatal::list<>, sc::o3>
    >,
    flatten_getters<structC>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s1::field0>,
      flat_getter<fatal::list<>, s1::field1>,
      flat_getter<fatal::list<>, s1::field2>,
      flat_getter<fatal::list<>, s1::field3>,
      flat_getter<fatal::list<>, s1::field4>,
      flat_getter<fatal::list<>, s1::field5>
    >,
    flatten_getters<struct1>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s2::fieldA>,
      flat_getter<fatal::list<>, s2::fieldB>,
      flat_getter<fatal::list<>, s2::fieldC>,
      flat_getter<fatal::list<>, s2::fieldD>,
      flat_getter<fatal::list<>, s2::fieldE>,
      flat_getter<fatal::list<>, s2::fieldF>,
      flat_getter<fatal::list<s2::fieldG>, s1::field0>,
      flat_getter<fatal::list<s2::fieldG>, s1::field1>,
      flat_getter<fatal::list<s2::fieldG>, s1::field2>,
      flat_getter<fatal::list<s2::fieldG>, s1::field3>,
      flat_getter<fatal::list<s2::fieldG>, s1::field4>,
      flat_getter<fatal::list<s2::fieldG>, s1::field5>
    >,
    flatten_getters<struct2>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s3::fieldA>,
      flat_getter<fatal::list<>, s3::fieldB>,
      flat_getter<fatal::list<>, s3::fieldC>,
      flat_getter<fatal::list<>, s3::fieldD>,
      flat_getter<fatal::list<>, s3::fieldE>,
      flat_getter<fatal::list<>, s3::fieldF>,
      flat_getter<fatal::list<s3::fieldG>, s1::field0>,
      flat_getter<fatal::list<s3::fieldG>, s1::field1>,
      flat_getter<fatal::list<s3::fieldG>, s1::field2>,
      flat_getter<fatal::list<s3::fieldG>, s1::field3>,
      flat_getter<fatal::list<s3::fieldG>, s1::field4>,
      flat_getter<fatal::list<s3::fieldG>, s1::field5>,
      flat_getter<fatal::list<>, s3::fieldH>,
      flat_getter<fatal::list<>, s3::fieldI>,
      flat_getter<fatal::list<>, s3::fieldJ>,
      flat_getter<fatal::list<>, s3::fieldK>,
      flat_getter<fatal::list<>, s3::fieldL>,
      flat_getter<fatal::list<>, s3::fieldM>,
      flat_getter<fatal::list<>, s3::fieldN>,
      flat_getter<fatal::list<>, s3::fieldO>,
      flat_getter<fatal::list<>, s3::fieldP>,
      flat_getter<fatal::list<>, s3::fieldQ>,
      flat_getter<fatal::list<>, s3::fieldR>
    >,
    flatten_getters<struct3>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s4::field0>,
      flat_getter<fatal::list<>, s4::field1>,
      flat_getter<fatal::list<>, s4::field2>,
      flat_getter<fatal::list<s4::field3>, sa::a>,
      flat_getter<fatal::list<s4::field3>, sa::b>
    >,
    flatten_getters<struct4>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s5::field0>,
      flat_getter<fatal::list<>, s5::field1>,
      flat_getter<fatal::list<>, s5::field2>,
      flat_getter<fatal::list<s5::field3>, sa::a>,
      flat_getter<fatal::list<s5::field3>, sa::b>,
      flat_getter<fatal::list<s5::field4>, sb::c>,
      flat_getter<fatal::list<s5::field4>, sb::d>
    >,
    flatten_getters<struct5>
  >();
}

struct annotated_terminals_only_filter {
  template <typename Member>
  using apply = fatal::negate<fatal::empty<typename Member::annotations::map>>;
};

TEST(flatten_getters, annotated_terminals_only) {
  using sa = reflect_struct<structA>::member;
  using sb = reflect_struct<structB>::member;
  using sc = reflect_struct<structC>::member;
  using s1 = reflect_struct<struct1>::member;
  using s2 = reflect_struct<struct2>::member;
  using s3 = reflect_struct<struct3>::member;
  using s4 = reflect_struct<struct4>::member;
  using s5 = reflect_struct<struct5>::member;

  EXPECT_SAME<
    fatal::list<>,
    flatten_getters<structA, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, sb::d>
    >,
    flatten_getters<structB, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<>,
    flatten_getters<structC, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<>,
    flatten_getters<struct1, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<>,
    flatten_getters<struct2, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<>,
    flatten_getters<struct3, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s4::field3>
    >,
    flatten_getters<struct4, annotated_terminals_only_filter>
  >();

  EXPECT_SAME<
    fatal::list<
      flat_getter<fatal::list<>, s5::field3>,
      flat_getter<fatal::list<s5::field4>, sb::d>
    >,
    flatten_getters<struct5, annotated_terminals_only_filter>
  >();
}

struct runtime_checker {
  template <typename Member, std::size_t Index, typename T, typename Expected>
  void operator ()(
    fatal::indexed<Member, Index>,
    T const &actual,
    Expected const &expected
  ) const {
    EXPECT_EQ(std::get<Index>(expected), Member::getter::ref(actual));
  }
};

TEST(flatten_getters, runtime) {
  struct5 x;
  x.set_field0(10);
  x.set_field1("hello");
  x.set_field2(enum1::field2);
  x.field3.set_a(56);
  x.field3.set_b("world");
  x.field4.set_c(7.2);
  x.field4.set_d(true);

  fatal::foreach<flatten_getters<struct5>>(
    runtime_checker(), x,
    std::make_tuple(10, "hello", enum1::field2, 56, "world", 7.2, true)
  );
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
