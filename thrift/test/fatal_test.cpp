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

#include <thrift/test/gen-cpp2/reflection_fatal.h>

#include <thrift/test/expect_same.h>

#include <gtest/gtest.h>

namespace test_cpp2 {
namespace cpp_reflection {

FATAL_STR(cpp2s, "cpp2");
FATAL_STR(cpp2ns, "test_cpp2::cpp_reflection");

FATAL_STR(enum1s, "enum1");
FATAL_STR(enum2s, "enum2");
FATAL_STR(enum3s, "enum3");

FATAL_STR(union1s, "union1");
FATAL_STR(union2s, "union2");
FATAL_STR(union3s, "union3");
FATAL_STR(unionAs, "unionA");

FATAL_STR(structAs, "structA");
FATAL_STR(structBs, "structB");
FATAL_STR(structCs, "structC");
FATAL_STR(struct1s, "struct1");
FATAL_STR(struct2s, "struct2");
FATAL_STR(struct3s, "struct3");

FATAL_STR(constant1s, "constant1");
FATAL_STR(constant2s, "constant2");
FATAL_STR(constant3s, "constant3");

FATAL_STR(service1s, "service1");
FATAL_STR(service2s, "service2");
FATAL_STR(service3s, "service3");

TEST(fatal, tags) {
  EXPECT_SAME<cpp2s, reflection_tags::languages::cpp2>();

  EXPECT_SAME<enum1s, reflection_tags::enums::enum1>();
  EXPECT_SAME<enum2s, reflection_tags::enums::enum2>();
  EXPECT_SAME<enum3s, reflection_tags::enums::enum3>();

  EXPECT_SAME<union1s, reflection_tags::unions::union1>();
  EXPECT_SAME<union2s, reflection_tags::unions::union2>();
  EXPECT_SAME<union3s, reflection_tags::unions::union3>();

  EXPECT_SAME<structAs, reflection_tags::structs::structA>();
  EXPECT_SAME<structBs, reflection_tags::structs::structB>();
  EXPECT_SAME<struct1s, reflection_tags::structs::struct1>();
  EXPECT_SAME<struct2s, reflection_tags::structs::struct2>();
  EXPECT_SAME<struct3s, reflection_tags::structs::struct3>();

  EXPECT_SAME<constant1s, reflection_tags::constants::constant1>();
  EXPECT_SAME<constant2s, reflection_tags::constants::constant2>();
  EXPECT_SAME<constant3s, reflection_tags::constants::constant3>();

  EXPECT_SAME<service1s, reflection_tags::services::service1>();
  EXPECT_SAME<service2s, reflection_tags::services::service2>();
  EXPECT_SAME<service3s, reflection_tags::services::service3>();
}

TEST(fatal, metadata) {
  using info = apache::thrift::reflect_module<reflection_tags::metadata>;

  EXPECT_SAME<
    info,
    apache::thrift::try_reflect_module<reflection_tags::metadata, void>
  >();
  EXPECT_SAME<void, apache::thrift::try_reflect_module<int, void>>();

  EXPECT_SAME<
    fatal::build_type_map<
      cpp2s, cpp2ns
    >,
    info::namespaces
  >();

  EXPECT_SAME<
    fatal::build_type_map<
      enum1, enum1s,
      enum2, enum2s,
      enum3, enum3s
    >,
    info::enums
  >();

  EXPECT_SAME<
    fatal::build_type_map<
      union1, union1s,
      union2, union2s,
      union3, union3s,
      unionA, unionAs
    >,
    info::unions
  >();

  EXPECT_SAME<
    fatal::build_type_map<
      structA, structAs,
      structB, structBs,
      structC, structCs,
      struct1, struct1s,
      struct2, struct2s,
      struct3, struct3s
    >,
    info::structs
  >();

  EXPECT_SAME<
    fatal::type_list<
      service1s,
      service2s,
      service3s
    >,
    info::services
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {

#include <thrift/test/gen-cpp2/reflection_fatal_constant.h>
#include <thrift/test/gen-cpp2/reflection_fatal_enum.h>
#include <thrift/test/gen-cpp2/reflection_fatal_service.h>
#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>
#include <thrift/test/gen-cpp2/reflection_fatal_union.h>

namespace test_cpp2 {
namespace cpp_reflection {

TEST(fatal, reflect_module_tag) {
  using tag = reflection_tags::metadata;

  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<enum1>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<enum2>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<enum3>>();

  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<union1>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<union2>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<union3>>();

  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<structA>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<structB>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<struct1>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<struct2>>();
  EXPECT_SAME<tag, apache::thrift::reflect_module_tag<struct3>>();

  // TODO: ADD SERVICES
}

TEST(fatal, try_reflect_module_tag) {
  using tag = reflection_tags::metadata;

  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<enum1, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<enum2, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<enum3, void>>();

  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<union1, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<union2, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<union3, void>>();

  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<structA, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<structB, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<struct1, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<struct2, void>>();
  EXPECT_SAME<tag, apache::thrift::try_reflect_module_tag<struct3, void>>();

  // TODO: ADD SERVICES

  EXPECT_SAME<void, apache::thrift::try_reflect_module_tag<int, void>>();
  EXPECT_SAME<void, apache::thrift::try_reflect_module_tag<void, void>>();
  EXPECT_SAME<
    void,
    apache::thrift::try_reflect_module_tag<std::string, void>
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
