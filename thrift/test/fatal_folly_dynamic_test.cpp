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

#include <thrift/lib/cpp2/fatal/debug.h>
#include <thrift/lib/cpp2/fatal/pretty_print.h>
#include <thrift/test/gen-cpp2/compat_fatal_enum.h>
#include <thrift/test/gen-cpp2/compat_fatal_struct.h>
#include <thrift/test/gen-cpp2/compat_fatal_union.h>
#include <thrift/test/gen-cpp2/global_fatal_enum.h>
#include <thrift/test/gen-cpp2/global_fatal_struct.h>
#include <thrift/test/gen-cpp2/global_fatal_union.h>
#include <thrift/test/gen-cpp2/reflection_fatal_enum.h>
#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>
#include <thrift/test/gen-cpp2/reflection_fatal_union.h>

#include <folly/json.h>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <sstream>
#include <utility>

namespace apache { namespace thrift {

template <typename T>
void test_to_from(T const &pod, folly::dynamic const &json) {
  std::ostringstream log;
  try {
    log.str("to_dynamic(PORTABLE):\n");
    auto const actual = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::PORTABLE
    >(pod);
    if (actual != json) {
      log << "actual: " << folly::toPrettyJson(actual)
        << std::endl << "expected: " << folly::toPrettyJson(json);
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(actual, json);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("from_dynamic(PORTABLE):\n");
    auto const actual = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE, T
    >(json);
    if (actual != pod) {
      apache::thrift::pretty_print(log << "actual: ", actual);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(actual, pod);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("from_dynamic(PORTABLE)/to_dynamic(PORTABLE):\n");
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE, T
    >(json);
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::PORTABLE
    >(from);
    if (json != to) {
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl << "to: " << folly::toPrettyJson(to)
        << std::endl << "expected: " << folly::toPrettyJson(json);
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(json, to);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(PORTABLE):\n");
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::PORTABLE
    >(pod);
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE, T
    >(to);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(PORTABLE,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::PORTABLE
    >(pod);
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE, T, false
    >(to);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(JSON_1,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::PORTABLE
    >(pod);
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::JSON_1, T, false
    >(to);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(JSON_1)/from_dynamic(PORTABLE,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::JSON_1
    >(pod);
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE, T, false
    >(to);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(JSON_1)/from_dynamic(JSON_1,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic<
      apache::thrift::dynamic_format::JSON_1
    >(pod);
    auto const from = apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::JSON_1, T, false
    >(to);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
}

template <typename T>
void test_compat(T const &pod, folly::dynamic const &json) {
  std::ostringstream log;
  try {
    log.str("to_dynamic(JSON_1)/readFromJson:\n");
    T decoded;
    auto const prettyJson = folly::toPrettyJson(
      apache::thrift::to_dynamic<
        apache::thrift::dynamic_format::JSON_1
      >(pod)
    );
    decoded.readFromJson(prettyJson.data(), prettyJson.size());
    if (pod != decoded) {
      log << "to: " << prettyJson << std::endl;
      apache::thrift::pretty_print(log << "readFromJson: ", decoded);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_TRUE(
      debug_equals(
        pod,
        decoded,
        apache::thrift::make_debug_output_callback(LOG(ERROR))
      )
    );
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(JSON_1,LENIENT)/readFromJson:\n");
    T decoded;
    auto const prettyJson = folly::toPrettyJson(
      apache::thrift::to_dynamic<
        apache::thrift::dynamic_format::JSON_1
      >(pod)
    );
    decoded.readFromJson(prettyJson.data(), prettyJson.size());
    if (pod != decoded) {
      log << "to: " << prettyJson << std::endl;
      apache::thrift::pretty_print(log << "readFromJson: ", decoded);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_TRUE(
      debug_equals(
        pod,
        decoded,
        apache::thrift::make_debug_output_callback(LOG(ERROR))
      )
    );
  } catch (std::exception const &e) {
    LOG(ERROR) << log.str();
    throw;
  }
}

template <
  typename Struct3,
  typename StructA,
  typename StructB,
  typename Enum1,
  typename Enum2
>
std::pair<Struct3, char const *> test_data_1() {
  StructA a1;
  a1.a = 99;
  a1.b = "abc";
  StructA a2;
  a2.a = 1001;
  a2.b = "foo";
  StructA a3;
  a3.a = 654;
  a3.b = "bar";
  StructA a4;
  a4.a = 9791;
  a4.b = "baz";
  StructA a5;
  a5.a = 111;
  a5.b = "gaz";

  StructB b1;
  b1.c = 1.23;
  b1.d = true;
  StructB b2;
  b2.c = 9.8;
  b2.d = false;
  StructB b3;
  b3.c = 10.01;
  b3.d = true;
  StructB b4;
  b4.c = 159.73;
  b4.d = false;
  StructB b5;
  b5.c = 468.02;
  b5.d = true;

  Struct3 pod;

  pod.fieldA = 141;
  pod.fieldB = "this is a test";
  pod.fieldC = Enum1::field0;
  pod.fieldD = Enum2::field1_2;
  pod.fieldE.set_ud(5.6);
  pod.fieldF.set_us_2("this is a variant");
  pod.fieldG.field0 = 98;
  pod.fieldG.field1 = "hello, world";
  pod.fieldG.field2 = Enum1::field2;
  pod.fieldG.field3 = Enum2::field0_2;
  pod.fieldG.field4.set_ui(19937);
  pod.fieldG.field5.set_ue_2(Enum1::field1);
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

  auto const json = "{\
    \"fieldA\": 141,\
    \"fieldB\": \"this is a test\",\
    \"fieldC\": \"field0\",\
    \"fieldD\": \"field1_2\",\
    \"fieldE\": {\
        \"ud\": 5.6\
    },\
    \"fieldF\": {\
        \"us_2\": \"this is a variant\"\
    },\
    \"fieldG\": {\
        \"field0\": 98,\
        \"field1\": \"hello, world\",\
        \"field2\": \"field2\",\
        \"field3\": \"field0_2\",\
        \"field4\": {\
            \"ui\": 19937\
        },\
        \"field5\": {\
            \"ue_2\": \"field1\"\
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
  }";

  return std::make_pair(pod, json);
}

TEST(fatal_folly_dynamic, to_from_dynamic) {
  auto const data = test_data_1<
    test_cpp2::cpp_reflection::struct3,
    test_cpp2::cpp_reflection::structA,
    test_cpp2::cpp_reflection::structB,
    test_cpp2::cpp_reflection::enum1,
    test_cpp2::cpp_reflection::enum2
  >();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
}

TEST(fatal_folly_dynamic, booleans) {
  auto const decode = [](char const *json) {
    return apache::thrift::from_dynamic<
      apache::thrift::dynamic_format::PORTABLE,
      test_cpp2::cpp_reflection::structB
    >(folly::parseJson(json));
  };

  test_cpp2::cpp_reflection::structB expected;
  expected.c = 1.3;
  expected.d = true;

  EXPECT_EQ(expected, decode("{ \"c\": 1.3, \"d\": 1}"));
  EXPECT_EQ(expected, decode("{ \"c\": 1.3, \"d\": 100}"));
  EXPECT_EQ(expected, decode("{ \"c\": 1.3, \"d\": true}"));
}

TEST(fatal_folly_dynamic, to_from_dynamic_compat) {
  auto const data = test_data_1<
    test_cpp2::cpp_compat::compat_struct3,
    test_cpp2::cpp_compat::compat_structA,
    test_cpp2::cpp_compat::compat_structB,
    test_cpp2::cpp_compat::compat_enum1,
    test_cpp2::cpp_compat::compat_enum2
  >();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
  test_compat(pod, json);
}

TEST(fatal_folly_dynamic, to_from_dynamic_global) {
  auto const data = test_data_1<
    ::global_struct3,
    ::global_structA,
    ::global_structB,
    ::global_enum1,
    ::global_enum2
  >();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
  test_compat(pod, json);
}

}} // apache::thrift

namespace test_cpp1 {
namespace cpp_compat {

bool compat_structA::operator <(compat_structA const &rhs) const {
  return a < rhs.a || (a == rhs.a && b < rhs.b);
}

bool compat_structB::operator <(compat_structB const &rhs) const {
  return c < rhs.c || (c == rhs.c && d < rhs.d);
}

} // namespace cpp_compat {
} // namespace test_cpp1 {

bool global_structA::operator <(global_structA const &rhs) const {
  return a < rhs.a || (a == rhs.a && b < rhs.b);
}

bool global_structB::operator <(global_structB const &rhs) const {
  return c < rhs.c || (c == rhs.c && d < rhs.d);
}
