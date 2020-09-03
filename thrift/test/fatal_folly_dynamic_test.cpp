/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sstream>
#include <utility>

#include <glog/logging.h>

#include <folly/String.h>
#include <folly/json.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/debug.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>
#include <thrift/lib/cpp2/reflection/helpers.h>
#include <thrift/lib/cpp2/reflection/internal/test_helpers.h>
#include <thrift/lib/cpp2/reflection/pretty_print.h>
#include <thrift/test/gen-cpp2/compat_fatal_types.h>
#include <thrift/test/gen-cpp2/global_fatal_types.h>
#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

using namespace cpp2;

namespace apache {
namespace thrift {

template <typename T>
void test_to_from(T const& pod, folly::dynamic const& json) {
  std::ostringstream log;
  try {
    log.str("to_dynamic(PORTABLE):\n");
    auto const actual = apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::PORTABLE);
    if (actual != json) {
      log << "actual: " << folly::toPrettyJson(actual) << std::endl
          << "expected: " << folly::toPrettyJson(json);
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(actual, json);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("from_dynamic(PORTABLE):\n");
    auto const actual = apache::thrift::from_dynamic<T>(
        json, apache::thrift::dynamic_format::PORTABLE);
    if (actual != pod) {
      apache::thrift::pretty_print(log << "actual: ", actual);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(actual, pod);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("from_dynamic(PORTABLE)/to_dynamic(PORTABLE):\n");
    auto const from = apache::thrift::from_dynamic<T>(
        json, apache::thrift::dynamic_format::PORTABLE);
    auto const to = apache::thrift::to_dynamic(
        from, apache::thrift::dynamic_format::PORTABLE);
    if (json != to) {
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl
          << "to: " << folly::toPrettyJson(to) << std::endl
          << "expected: " << folly::toPrettyJson(json);
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(json, to);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(PORTABLE):\n");
    auto const to = apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::PORTABLE);
    auto const from = apache::thrift::from_dynamic<T>(
        to, apache::thrift::dynamic_format::PORTABLE);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(PORTABLE,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::PORTABLE);
    auto const from = apache::thrift::from_dynamic<T>(
        to,
        apache::thrift::dynamic_format::PORTABLE,
        apache::thrift::format_adherence::LENIENT);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(PORTABLE)/from_dynamic(JSON_1,LENIENT):\n");
    auto const to = apache::thrift::to_dynamic(
        pod, apache::thrift::dynamic_format::PORTABLE);
    auto const from = apache::thrift::from_dynamic<T>(
        to,
        apache::thrift::dynamic_format::JSON_1,
        apache::thrift::format_adherence::LENIENT);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(JSON_1)/from_dynamic(PORTABLE,LENIENT):\n");
    auto const to =
        apache::thrift::to_dynamic(pod, apache::thrift::dynamic_format::JSON_1);
    auto const from = apache::thrift::from_dynamic<T>(
        to,
        apache::thrift::dynamic_format::PORTABLE,
        apache::thrift::format_adherence::LENIENT);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
  try {
    log.str("to_dynamic(JSON_1)/from_dynamic(JSON_1,LENIENT):\n");
    auto const to =
        apache::thrift::to_dynamic(pod, apache::thrift::dynamic_format::JSON_1);
    auto const from = apache::thrift::from_dynamic<T>(
        to,
        apache::thrift::dynamic_format::JSON_1,
        apache::thrift::format_adherence::LENIENT);
    if (pod != from) {
      log << "to: " << folly::toPrettyJson(to) << std::endl;
      apache::thrift::pretty_print(log << "from: ", from);
      log << std::endl;
      apache::thrift::pretty_print(log << "expected: ", pod);
      log << std::endl;
      LOG(ERROR) << log.str();
    }
    EXPECT_EQ(pod, from);
  } catch (std::exception const&) {
    LOG(ERROR) << log.str();
    throw;
  }
}

template <
    typename Struct3,
    typename StructA,
    typename StructB,
    typename Enum1,
    typename Enum2>
std::pair<Struct3, std::string> test_data_1() {
  StructA a1;
  a1.__isset.a = true;
  *a1.a_ref() = 99;
  a1.__isset.b = true;
  *a1.b_ref() = "abc";
  StructA a2;
  a2.__isset.a = true;
  *a2.a_ref() = 1001;
  a2.__isset.b = true;
  *a2.b_ref() = "foo";
  StructA a3;
  a3.__isset.a = true;
  *a3.a_ref() = 654;
  a3.__isset.b = true;
  *a3.b_ref() = "bar";
  StructA a4;
  a4.__isset.a = true;
  *a4.a_ref() = 9791;
  a4.__isset.b = true;
  *a4.b_ref() = "baz";
  StructA a5;
  a5.__isset.a = true;
  *a5.a_ref() = 111;
  a5.__isset.b = true;
  *a5.b_ref() = "gaz";

  StructB b1;
  b1.__isset.c = true;
  *b1.c_ref() = 1.23;
  b1.__isset.d = true;
  *b1.d_ref() = true;
  StructB b2;
  b2.__isset.c = true;
  *b2.c_ref() = 9.8;
  b2.__isset.d = true;
  *b2.d_ref() = false;
  StructB b3;
  b3.__isset.c = true;
  *b3.c_ref() = 10.01;
  b3.__isset.d = true;
  *b3.d_ref() = true;
  StructB b4;
  b4.__isset.c = true;
  *b4.c_ref() = 159.73;
  b4.__isset.d = true;
  *b4.d_ref() = false;
  StructB b5;
  b5.__isset.c = true;
  *b5.c_ref() = 468.02;
  b5.__isset.d = true;
  *b5.d_ref() = true;

  Struct3 pod;

  pod.__isset.fieldA = true;
  *pod.fieldA_ref() = 141;
  pod.__isset.fieldB = true;
  *pod.fieldB_ref() = "this is a test";
  pod.__isset.fieldC = true;
  *pod.fieldC_ref() = Enum1::field0;
  pod.__isset.fieldD = true;
  *pod.fieldD_ref() = Enum2::field1_2;
  pod.__isset.fieldE = true;
  pod.fieldE_ref()->set_ud(5.6);
  pod.__isset.fieldF = true;
  pod.fieldF_ref()->set_us_2("this is a variant");
  pod.__isset.fieldG = true;
  pod.fieldG_ref()->field0 = 98;
  pod.fieldG_ref()->field1_ref() = "hello, world";
  pod.fieldG_ref()->__isset.field2 = true;
  *pod.fieldG_ref()->field2_ref() = Enum1::field2;
  pod.fieldG_ref()->field3 = Enum2::field0_2;
  pod.fieldG_ref()->field4_ref() = {};
  pod.fieldG_ref()->field4_ref()->set_ui(19937);
  pod.fieldG_ref()->__isset.field5 = true;
  pod.fieldG_ref()->field5_ref()->set_ue_2(Enum1::field1);
  // fieldH intentionally left empty
  pod.__isset.fieldI = true;
  *pod.fieldI_ref() = {3, 5, 7, 9};
  pod.__isset.fieldJ = true;
  *pod.fieldJ_ref() = {"a", "b", "c", "d"};
  pod.__isset.fieldK = true;
  *pod.fieldK_ref() = {};
  pod.__isset.fieldL = true;
  pod.fieldL_ref()->push_back(a1);
  pod.fieldL_ref()->push_back(a2);
  pod.fieldL_ref()->push_back(a3);
  pod.fieldL_ref()->push_back(a4);
  pod.fieldL_ref()->push_back(a5);
  pod.__isset.fieldM = true;
  *pod.fieldM_ref() = {2, 4, 6, 8};
  pod.__isset.fieldN = true;
  *pod.fieldN_ref() = {"w", "x", "y", "z"};
  pod.__isset.fieldO = true;
  *pod.fieldO_ref() = {};
  pod.__isset.fieldP = true;
  *pod.fieldP_ref() = {b1, b2, b3, b4, b5};
  pod.__isset.fieldQ = true;
  *pod.fieldQ_ref() = {{"a1", a1}, {"a2", a2}, {"a3", a3}};
  pod.__isset.fieldR = true;
  *pod.fieldR_ref() = {};

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
      test_cpp2::cpp_reflection::enum2>();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
}

TEST(fatal_folly_dynamic, booleans) {
  auto const decode = [](char const* json) {
    return apache::thrift::from_dynamic<test_cpp2::cpp_reflection::structB>(
        folly::parseJson(json), apache::thrift::dynamic_format::PORTABLE);
  };

  test_cpp2::cpp_reflection::structB expected;
  *expected.c_ref() = 1.3;
  *expected.d_ref() = true;

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
      test_cpp2::cpp_compat::compat_enum2>();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
}

TEST(fatal_folly_dynamic, to_from_dynamic_global) {
  auto const data = test_data_1<
      ::global_struct3,
      ::global_structA,
      ::global_structB,
      ::global_enum1,
      ::global_enum2>();
  auto const pod = data.first;
  auto const json = folly::parseJson(data.second);

  test_to_from(pod, json);
}

TEST(fatal_folly_dynamic, to_from_dynamic_binary) {
  folly::dynamic actl = folly::dynamic::object;
  folly::dynamic expt = folly::dynamic::object;

  // to
  test_cpp2::cpp_reflection::struct_binary a;
  *a.bi_ref() = "123abc";

  actl = to_dynamic(a, dynamic_format::PORTABLE);
  expt = folly::dynamic::object("bi", "123abc");

  EXPECT_EQ(expt, actl);

  // from
  auto obj = from_dynamic<test_cpp2::cpp_reflection::struct_binary>(
      folly::dynamic::object("bi", "123abc"),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_EQ("123abc", *obj.bi_ref());
}

} // namespace thrift
} // namespace apache

namespace test_cpp1 {
namespace cpp_compat {} // namespace cpp_compat
} // namespace test_cpp1

TEST(fatal_folly_dynamic, optional_string) {
  auto obj = apache::thrift::from_dynamic<global_struct1>(
      folly::dynamic::object("field1", "asdf"),
      apache::thrift::dynamic_format::PORTABLE);
  EXPECT_EQ("asdf", *obj.field1_ref());
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
      apache::thrift::type_class::set<apache::thrift::type_class::structure>>();
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

  std::string member_name_s{fatal::to_instance<std::string, member_name>()};
};
} // namespace

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
