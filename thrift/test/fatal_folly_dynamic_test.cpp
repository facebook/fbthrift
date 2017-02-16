/*
 * Copyright 2016-present Facebook, Inc.
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
#include <thrift/lib/cpp2/fatal/helpers.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <thrift/lib/cpp2/fatal/debug.h>
#include <thrift/lib/cpp2/fatal/pretty_print.h>
#include <thrift/test/gen-cpp2/compat_fatal_types.h>
#include <thrift/test/gen-cpp2/global_fatal_types.h>
#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

#include <folly/String.h>
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
    auto const actual = apache::thrift::to_dynamic(
      pod, apache::thrift::dynamic_format::PORTABLE
    );
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
    auto const actual = apache::thrift::from_dynamic<T>(
      json, apache::thrift::dynamic_format::PORTABLE
    );
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
    auto const from = apache::thrift::from_dynamic<T>(
      json, apache::thrift::dynamic_format::PORTABLE
    );
    auto const to = apache::thrift::to_dynamic(
      from, apache::thrift::dynamic_format::PORTABLE
    );
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
    auto const to = apache::thrift::to_dynamic(
      pod, apache::thrift::dynamic_format::PORTABLE
    );
    auto const from = apache::thrift::from_dynamic<T>(
      to, apache::thrift::dynamic_format::PORTABLE
    );
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
    auto const to = apache::thrift::to_dynamic(
      pod, apache::thrift::dynamic_format::PORTABLE
    );
    auto const from = apache::thrift::from_dynamic<T>(
      to,
      apache::thrift::dynamic_format::PORTABLE,
      apache::thrift::format_adherence::LENIENT
    );
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
    auto const to = apache::thrift::to_dynamic(
      pod, apache::thrift::dynamic_format::PORTABLE
    );
    auto const from = apache::thrift::from_dynamic<T>(
      to,
      apache::thrift::dynamic_format::JSON_1,
      apache::thrift::format_adherence::LENIENT
    );
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
    auto const to = apache::thrift::to_dynamic(
      pod, apache::thrift::dynamic_format::JSON_1
    );
    auto const from = apache::thrift::from_dynamic<T>(
      to,
      apache::thrift::dynamic_format::PORTABLE,
      apache::thrift::format_adherence::LENIENT
    );
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
    auto const to = apache::thrift::to_dynamic(
      pod,
      apache::thrift::dynamic_format::JSON_1
    );
    auto const from = apache::thrift::from_dynamic<T>(
      to,
      apache::thrift::dynamic_format::JSON_1,
      apache::thrift::format_adherence::LENIENT
    );
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
      apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::JSON_1
      )
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
      apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::JSON_1
      )
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
std::pair<Struct3, std::string> test_data_1() {
  StructA a1;
  a1.__isset.a = true;
  a1.a = 99;
  a1.__isset.b = true;
  a1.b = "abc";
  StructA a2;
  a2.__isset.a = true;
  a2.a = 1001;
  a2.__isset.b = true;
  a2.b = "foo";
  StructA a3;
  a3.__isset.a = true;
  a3.a = 654;
  a3.__isset.b = true;
  a3.b = "bar";
  StructA a4;
  a4.__isset.a = true;
  a4.a = 9791;
  a4.__isset.b = true;
  a4.b = "baz";
  StructA a5;
  a5.__isset.a = true;
  a5.a = 111;
  a5.__isset.b = true;
  a5.b = "gaz";

  StructB b1;
  b1.__isset.c = true;
  b1.c = 1.23;
  b1.__isset.d = true;
  b1.d = true;
  StructB b2;
  b2.__isset.c = true;
  b2.c = 9.8;
  b2.__isset.d = true;
  b2.d = false;
  StructB b3;
  b3.__isset.c = true;
  b3.c = 10.01;
  b3.__isset.d = true;
  b3.d = true;
  StructB b4;
  b4.__isset.c = true;
  b4.c = 159.73;
  b4.__isset.d = true;
  b4.d = false;
  StructB b5;
  b5.__isset.c = true;
  b5.c = 468.02;
  b5.__isset.d = true;
  b5.d = true;

  Struct3 pod;

  pod.__isset.fieldA = true;
  pod.fieldA = 141;
  pod.__isset.fieldB = true;
  pod.fieldB = "this is a test";
  pod.__isset.fieldC = true;
  pod.fieldC = Enum1::field0;
  pod.__isset.fieldD = true;
  pod.fieldD = Enum2::field1_2;
  pod.__isset.fieldE = true;
  pod.fieldE.set_ud(5.6);
  pod.__isset.fieldF = true;
  pod.fieldF.set_us_2("this is a variant");
  pod.__isset.fieldG = true;
  pod.fieldG.field0 = 98;
  pod.fieldG.__isset.field1 = true;
  pod.fieldG.field1 = "hello, world";
  pod.fieldG.__isset.field2 = true;
  pod.fieldG.field2 = Enum1::field2;
  pod.fieldG.field3 = Enum2::field0_2;
  pod.fieldG.__isset.field4 = true;
  pod.fieldG.field4.set_ui(19937);
  pod.fieldG.__isset.field5 = true;
  pod.fieldG.field5.set_ue_2(Enum1::field1);
  // fieldH intentionally left empty
  pod.__isset.fieldI = true;
  pod.fieldI = {3, 5, 7, 9};
  pod.__isset.fieldJ = true;
  pod.fieldJ = {"a", "b", "c", "d"};
  pod.__isset.fieldK = true;
  pod.fieldK = {};
  pod.__isset.fieldL = true;
  pod.fieldL.push_back(a1);
  pod.fieldL.push_back(a2);
  pod.fieldL.push_back(a3);
  pod.fieldL.push_back(a4);
  pod.fieldL.push_back(a5);
  pod.__isset.fieldM = true;
  pod.fieldM = {2, 4, 6, 8};
  pod.__isset.fieldN = true;
  pod.fieldN = {"w", "x", "y", "z"};
  pod.__isset.fieldO = true;
  pod.fieldO = {};
  pod.__isset.fieldP = true;
  pod.fieldP = {b1, b2, b3, b4, b5};
  pod.__isset.fieldQ = true;
  pod.fieldQ = {{"a1", a1}, {"a2", a2}, {"a3", a3}};
  pod.__isset.fieldR = true;
  pod.fieldR = {};

  auto const json = folly::stripLeftMargin(R"({
    "fieldA": 141,
    "fieldB": "this is a test",
    "fieldC": "field0",
    "fieldD": "field1_2",
    "fieldE": {
        "ud": 5.6
    },
    "fieldF": {
        "us_2": "this is a variant"
    },
    "fieldG": {
        "field0": 98,
        "field1": "hello, world",
        "field2": "field2",
        "field3": "field0_2",
        "field4": {
            "ui": 19937
        },
        "field5": {
            "ue_2": "field1"
        }
    },
    "fieldH": {},
    "fieldI": [3, 5, 7, 9],
    "fieldJ": ["a", "b", "c", "d"],
    "fieldK": [],
    "fieldL": [
      { "a": 99, "b": "abc" },
      { "a": 1001, "b": "foo" },
      { "a": 654, "b": "bar" },
      { "a": 9791, "b": "baz" },
      { "a": 111, "b": "gaz" }
    ],
    "fieldM": [2, 4, 6, 8],
    "fieldN": ["w", "x", "y", "z"],
    "fieldO": [],
    "fieldP": [
      { "c": 1.23, "d": true },
      { "c": 9.8, "d": false },
      { "c": 10.01, "d": true },
      { "c": 159.73, "d": false },
      { "c": 468.02, "d": true }
    ],
    "fieldQ": {
      "a1": { "a": 99, "b": "abc" },
      "a2": { "a": 1001, "b": "foo" },
      "a3": { "a": 654, "b": "bar" }
    },
    "fieldR": {},
    "fieldS": {}
  })");

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
    return apache::thrift::from_dynamic<test_cpp2::cpp_reflection::structB>(
      folly::parseJson(json), apache::thrift::dynamic_format::PORTABLE
    );
  };

  test_cpp2::cpp_reflection::structB expected;
  expected.c = 1.3;
  expected.d = true;

  EXPECT_EQ(expected, decode(R"({ "c": 1.3, "d": 1})"));
  EXPECT_EQ(expected, decode(R"({ "c": 1.3, "d": 100})"));
  EXPECT_EQ(expected, decode(R"({ "c": 1.3, "d": true})"));
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

TEST(fatal_folly_dynamic, to_from_dynamic_binary) {
  folly::dynamic actl = folly::dynamic::object;
  folly::dynamic expt = folly::dynamic::object;

  // to
  test_cpp2::cpp_reflection::struct_binary a;
  a.bi = "123abc";

  actl = to_dynamic(a, dynamic_format::PORTABLE);
  expt = folly::dynamic::object
      ("bi", "123abc");

  EXPECT_EQ(expt, actl);

  // from
  auto obj = from_dynamic<test_cpp2::cpp_reflection::struct_binary>(
      folly::dynamic::object("bi", "123abc"),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_EQ("123abc", obj.bi);
}

}} // apache::thrift

namespace test_cpp1 {
namespace cpp_compat {

} // namespace cpp_compat {
} // namespace test_cpp1 {

TEST(fatal_folly_dynamic, optional_string) {
  auto obj = apache::thrift::from_dynamic<global_struct1>(
      folly::dynamic::object("field1", "asdf"),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_TRUE(obj.__isset.field1);
  EXPECT_EQ("asdf", obj.field1);
}

TEST(fatal_folly_dynamic, list_from_empty_object) {
  // some dynamic languages (lua, php) conflate empty array and empty object;
  // check that we do not throw in such cases
  using type = global_structC;
  using member_name = fatal::sequence<char, 'j', '3'>;
  using member_meta =
      apache::thrift::get_struct_member_by_name<type, member_name>;
  EXPECT_SAME< // sanity check
      member_meta::type_class,
      apache::thrift::type_class::list<
        apache::thrift::type_class::structure>>();
  auto obj = apache::thrift::from_dynamic<type>(
      folly::dynamic::object(
        fatal::z_data<member_name>(), folly::dynamic::object),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_TRUE(member_meta::is_set(obj));
  EXPECT_EQ(0, member_meta::getter::ref(obj).size());
}

TEST(fatal_folly_dynamic, set_from_empty_object) {
  // some dynamic languages (lua, php) conflate empty array and empty object;
  // check that we do not throw in such cases
  using type = global_structC;
  using member_name = fatal::sequence<char, 'k', '3'>;
  using member_meta =
      apache::thrift::get_struct_member_by_name<type, member_name>;
  EXPECT_SAME< // sanity check
      member_meta::type_class,
      apache::thrift::type_class::set<
        apache::thrift::type_class::structure>>();
  auto obj = apache::thrift::from_dynamic<type>(
      folly::dynamic::object(
        fatal::z_data<member_name>(), folly::dynamic::object),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_TRUE(member_meta::is_set(obj));
  EXPECT_EQ(0, member_meta::getter::ref(obj).size());
}

TEST(fatal_folly_dynamic, map_from_empty_array) {
  // some dynamic languages (lua, php) conflate empty array and empty object;
  // check that we do not throw in such cases
  using type = global_structC;
  using member_name = fatal::sequence<char, 'l', '3'>;
  using member_meta =
      apache::thrift::get_struct_member_by_name<type, member_name>;
  EXPECT_SAME< // sanity check
      member_meta::type_class,
      apache::thrift::type_class::map<
        apache::thrift::type_class::integral,
        apache::thrift::type_class::structure>>();
  auto obj = apache::thrift::from_dynamic<type>(
      folly::dynamic::object(
        fatal::z_data<member_name>(), folly::dynamic::array),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_TRUE(member_meta::is_set(obj));
  EXPECT_EQ(0, member_meta::getter::ref(obj).size());
}

namespace {

class fatal_folly_dynamic_enum : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_SAME< // sanity check
        member_meta::type_class,
        apache::thrift::type_class::enumeration>();
  }

  using type = global_structC;
  using member_name = fatal::sequence<char, 'e'>;
  using member_meta =
      apache::thrift::get_struct_member_by_name<type, member_name>;

  std::string member_name_s = fatal::to_instance<std::string, member_name>();
};
}

TEST_F(fatal_folly_dynamic_enum, from_string_strict) {
  folly::dynamic dyn = folly::dynamic::object(member_name_s, "field0");
  auto obj = apache::thrift::from_dynamic<type>(
      dyn, apache::thrift::dynamic_format::PORTABLE);
  EXPECT_TRUE(member_meta::is_set(obj));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj));
  EXPECT_THROW(
      apache::thrift::from_dynamic<type>(
          dyn, apache::thrift::dynamic_format::JSON_1),
      folly::ConversionError);
}

TEST_F(fatal_folly_dynamic_enum, from_integer_strict) {
  folly::dynamic dyn = folly::dynamic::object(member_name_s, 0);
  auto obj = apache::thrift::from_dynamic<type>(
      dyn, apache::thrift::dynamic_format::JSON_1);
  EXPECT_TRUE(member_meta::is_set(obj));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj));
  EXPECT_THROW(
      apache::thrift::from_dynamic<type>(
          dyn, apache::thrift::dynamic_format::PORTABLE),
      std::invalid_argument);
}

TEST_F(fatal_folly_dynamic_enum, from_string_lenient) {
  folly::dynamic dyn = folly::dynamic::object(member_name_s, "field0");
  auto obj1 = apache::thrift::from_dynamic<type>(
      dyn,
      apache::thrift::dynamic_format::PORTABLE,
      apache::thrift::format_adherence::LENIENT);
  EXPECT_TRUE(member_meta::is_set(obj1));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj1));
  auto obj2 = apache::thrift::from_dynamic<type>(
      dyn,
      apache::thrift::dynamic_format::JSON_1,
      apache::thrift::format_adherence::LENIENT);
  EXPECT_TRUE(member_meta::is_set(obj2));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj2));
}

TEST_F(fatal_folly_dynamic_enum, from_integer_lenient) {
  folly::dynamic dyn = folly::dynamic::object(member_name_s, 0);
  auto obj1 = apache::thrift::from_dynamic<type>(
      dyn,
      apache::thrift::dynamic_format::PORTABLE,
      apache::thrift::format_adherence::LENIENT);
  EXPECT_TRUE(member_meta::is_set(obj1));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj1));
  auto obj2 = apache::thrift::from_dynamic<type>(
      dyn,
      apache::thrift::dynamic_format::JSON_1,
      apache::thrift::format_adherence::LENIENT);
  EXPECT_TRUE(member_meta::is_set(obj2));
  EXPECT_EQ(global_enum1::field0, member_meta::getter::ref(obj2));
}
