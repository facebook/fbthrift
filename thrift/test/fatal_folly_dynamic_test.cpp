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

#include <thrift/lib/cpp2/fatal/folly_dynamic.h>

#include <thrift/test/gen-cpp2/reflection_fatal_enum.h>
#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>
#include <thrift/test/gen-cpp2/reflection_fatal_union.h>

#include <folly/json.h>

#include <gtest/gtest.h>

using output_json = std::false_type;

namespace test_cpp2 {
namespace cpp_reflection {

#define TEST_IMPL(Input, Expected) \
  do { \
    auto const actual = apache::thrift::to_dynamic(Input); \
    \
    if (output_json::value) { \
      std::cout << "output json: " << folly::toPrettyJson(actual) \
        << std::endl << "expected json: " << folly::toPrettyJson(Expected) \
        << std::endl; \
    } \
    \
    EXPECT_EQ(actual, Expected); \
  } while (false)

TEST(fatal_folly_dynamic, to_dynamic) {
  structA a1;
  a1.a = 99;
  a1.b = "abc";
  structA a2;
  a2.a = 1001;
  a2.b = "foo";
  structA a3;
  a3.a = 654;
  a3.b = "bar";
  structA a4;
  a4.a = 9791;
  a4.b = "baz";
  structA a5;
  a5.a = 111;
  a5.b = "gaz";

  structB b1;
  b1.c = 1.23;
  b1.d = true;
  structB b2;
  b2.c = 9.8;
  b2.d = false;
  structB b3;
  b3.c = 10.01;
  b3.d = true;
  structB b4;
  b4.c = 159.73;
  b4.d = false;
  structB b5;
  b5.c = 468.02;
  b5.d = true;

  struct3 pod;

  pod.fieldA = 141;
  pod.fieldB = "this is a test";
  pod.fieldC = enum1::field0;
  pod.fieldD = enum2::field1_2;
  pod.fieldE.set_ud(5.6);
  pod.fieldF.set_us_2("this is a variant");
  pod.fieldG.field0 = 98;
  pod.fieldG.field1 = "hello, world";
  pod.fieldG.field2 = enum1::field2;
  pod.fieldG.field3 = enum2::field0_2;
  pod.fieldG.field4.set_ui(19937);
  pod.fieldG.field5.set_ue_2(enum1::field1);
  // fieldH intentionally left empty
  pod.fieldI = {3, 5, 7, 9};
  pod.fieldJ = {"a", "b", "c", "d"};
  pod.fieldK = {};
  pod.fieldL.push_back(a1);
  pod.fieldL.push_back(a2);
  pod.fieldL.push_back(a3);
  pod.fieldL.push_back(a4);
  pod.fieldL.push_back(a5);
  pod.fieldM = {2, 4, 6, 8};
  pod.fieldN = {"w", "x", "y", "z"};
  pod.fieldO = {};
  pod.fieldP = {b1, b2, b3, b4, b5};
  pod.fieldQ = {{"a1", a1}, {"a2", a2}, {"a3", a3}};
  pod.fieldR = {};

  TEST_IMPL(pod, folly::parseJson("{\
    \"fieldA\": 141,\
    \"fieldB\": \"this is a test\",\
    \"fieldC\": \"field0\",\
    \"fieldD\": \"field1_2\",\
    \"fieldE\": {\
        \"value\": 5.6,\
        \"type\": \"ud\"\
    },\
    \"fieldF\": {\
        \"value\": \"this is a variant\",\
        \"type\": \"us_2\"\
    },\
    \"fieldG\": {\
        \"field0\": 98,\
        \"field1\": \"hello, world\",\
        \"field2\": \"field2\",\
        \"field3\": \"field0_2\",\
        \"field4\": {\
            \"value\": 19937,\
            \"type\": \"ui\"\
        },\
        \"field5\": {\
            \"value\": \"field1\",\
            \"type\": \"ue_2\"\
        }\
    },\
    \"fieldH\": {},\
    \"fieldI\": [3, 5, 7, 9],\
    \"fieldJ\": [\"a\", \"b\", \"c\", \"d\"],\
    \"fieldK\": [],\
    \"fieldL\": [\
      { \"a\": 99, \"b\": \"abc\" },\
      { \"a\": 1001, \"b\": \"foo\" },\
      { \"a\": 654, \"b\": \"bar\" },\
      { \"a\": 9791, \"b\": \"baz\" },\
      { \"a\": 111, \"b\": \"gaz\" }\
    ],\
    \"fieldM\": [2, 4, 6, 8],\
    \"fieldN\": [\"w\", \"x\", \"y\", \"z\"],\
    \"fieldO\": [],\
    \"fieldP\": [\
      { \"c\": 1.23, \"d\": true },\
      { \"c\": 9.8, \"d\": false },\
      { \"c\": 10.01, \"d\": true },\
      { \"c\": 159.73, \"d\": false },\
      { \"c\": 468.02, \"d\": true }\
    ],\
    \"fieldQ\": {\
      \"a1\": { \"a\": 99, \"b\": \"abc\" },\
      \"a2\": { \"a\": 1001, \"b\": \"foo\" },\
      \"a3\": { \"a\": 654, \"b\": \"bar\" }\
    },\
    \"fieldR\": {}\
  }"));
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
