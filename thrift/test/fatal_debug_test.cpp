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

#include <thrift/lib/cpp2/fatal/debug.h>
#include <thrift/lib/cpp2/fatal/testing.h>

#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>


namespace test_cpp2 {
namespace cpp_reflection {

struct3 test_data() {
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
  pod.fieldS = {{"123", "456"}, {"abc", "ABC"}, {"def", "DEF"}};

  return pod;
}

struct test_callback {
  explicit test_callback(std::vector<std::string> &out): out_(out) {}

  template <typename  T>
  void operator ()(
    T const &,
    T const &,
    folly::StringPiece path,
    folly::StringPiece
  ) const {
    out_.emplace_back(path.data(), path.size());
  }

private:
  std::vector<std::string> &out_;
};

#define TEST_IMPL(LHS, RHS, ...) \
  do { \
    std::vector<std::string> const expected{__VA_ARGS__}; \
    \
    std::vector<std::string> actual; \
    actual.reserve(expected.size()); \
    \
    EXPECT_EQ( \
      expected.empty(), \
      (apache::thrift::debug_equals(LHS, RHS, test_callback(actual))) \
    ); \
    \
    EXPECT_EQ(expected, actual); \
  } while (false)

TEST(fatal_debug, equal) {
  TEST_IMPL(test_data(), test_data());
}

TEST(Equal, Failure) {
  auto pod = test_data();
  struct3 pod1, pod2;
  pod1.fieldR["a"].c = 1;
  pod1.fieldR["b"].c = 2;
  pod1.fieldR["c"].c = 3;
  pod1.fieldR["d"].c = 4;
  pod2.fieldR["d"].c = 4;
  pod2.fieldR["c"].c = 3;
  pod2.fieldR["b"].c = 2;
  pod2.fieldR["a"].c = 1;
  EXPECT_THRIFT_EQ(pod1, pod2);
}

TEST(fatal_debug, fieldA) {
  auto pod = test_data();
  pod.fieldA = 90;
  TEST_IMPL(pod, test_data(), "$.fieldA");
  pod.fieldA = 141;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldB) {
  auto pod = test_data();
  pod.fieldB = "should mismatch";
  TEST_IMPL(pod, test_data(), "$.fieldB");
  pod.fieldB = "this is a test";
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldC) {
  auto pod = test_data();
  pod.fieldC = enum1::field2;
  TEST_IMPL(pod, test_data(), "$.fieldC");
  pod.fieldC = enum1::field0;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldE) {
  auto pod = test_data();
  pod.fieldE.set_ui(5);
  TEST_IMPL(pod, test_data(), "$.fieldE");
  pod.fieldE.__clear();
  TEST_IMPL(pod, test_data(), "$.fieldE");
  pod.fieldE.set_ud(4);
  TEST_IMPL(pod, test_data(), "$.fieldE.ud");
  pod.fieldE.set_ud(5.6);
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldH) {
  auto pod = test_data();
  pod.fieldH.set_ui_2(3);
  TEST_IMPL(pod, test_data(), "$.fieldH");
  pod.fieldH.__clear();
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldI) {
  auto pod = test_data();
  pod.fieldI[0] = 4;
  TEST_IMPL(pod, test_data(), "$.fieldI[0]");
  pod.fieldI[0] = 3;
  TEST_IMPL(pod, test_data());
  pod.fieldI[2] = 10;
  TEST_IMPL(pod, test_data(), "$.fieldI[2]");
  pod.fieldI.push_back(11);
  TEST_IMPL(pod, test_data(), "$.fieldI");
  pod.fieldI.clear();
  TEST_IMPL(pod, test_data(), "$.fieldI");
}

TEST(fatal_debug, fieldM) {
  auto pod = test_data();
  pod.fieldM.clear();
  TEST_IMPL(
      pod,
      test_data(),
      "$.fieldM" /* size-mismatch */,
      "$.fieldM[2]" /* extra */,
      "$.fieldM[4]" /* extra */,
      "$.fieldM[6]" /* extra */,
      "$.fieldM[8]" /* extra */);
  pod.fieldM.insert(11);
  pod.fieldM.insert(12);
  pod.fieldM.insert(13);
  pod.fieldM.insert(14);
  TEST_IMPL(
      pod,
      test_data(),
      "$.fieldM[11]" /* missing */,
      "$.fieldM[12]" /* missing */,
      "$.fieldM[13]" /* missing */,
      "$.fieldM[14]" /* missing */,
      "$.fieldM[2]" /* extra */,
      "$.fieldM[4]" /* extra */,
      "$.fieldM[6]" /* extra */,
      "$.fieldM[8]" /* extra */);
  pod.fieldM = test_data().fieldM;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldQ) {
  auto pod = test_data();
  pod.fieldQ.clear();
  TEST_IMPL(
      pod,
      test_data(),
      "$.fieldQ" /* size-mismatch */,
      "$.fieldQ[\"a1\"]" /* extra */,
      "$.fieldQ[\"a2\"]" /* extra */,
      "$.fieldQ[\"a3\"]" /* extra */);
  structA a1;
  a1.a = 1;
  a1.b = "1";
  structA a2;
  a2.a = 2;
  a2.b = "2";
  structA a3;
  a3.a = 3;
  a3.b = "3";
  pod.fieldQ["A1"] = a1;
  pod.fieldQ["A2"] = a2;
  pod.fieldQ["A3"] = a3;
  TEST_IMPL(
      pod,
      test_data(),
      "$.fieldQ[\"A1\"]" /* missing */,
      "$.fieldQ[\"A2\"]" /* missing */,
      "$.fieldQ[\"A3\"]" /* missing */,
      "$.fieldQ[\"a1\"]" /* extra */,
      "$.fieldQ[\"a2\"]" /* extra */,
      "$.fieldQ[\"a3\"]" /* extra */);
  pod.fieldQ = test_data().fieldQ;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldG_field0) {
  auto pod = test_data();
  pod.fieldG.field0 = 12;
  TEST_IMPL(pod, test_data(), "$.fieldG.field0");
  pod.fieldG.field0 = 98;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldG_field1) {
  auto pod = test_data();
  pod.fieldG.field1 = "should mismatch";
  TEST_IMPL(pod, test_data(), "$.fieldG.field1");
  pod.fieldG.field1 = "hello, world";
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldG_field2) {
  auto pod = test_data();
  pod.fieldG.field2 = enum1::field1;
  TEST_IMPL(pod, test_data(), "$.fieldG.field2");
  pod.fieldG.field2 = enum1::field2;
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldG_field5) {
  auto pod = test_data();
  pod.fieldG.field5.set_ui_2(5);
  TEST_IMPL(pod, test_data(), "$.fieldG.field5");
  pod.fieldG.field5.__clear();
  TEST_IMPL(pod, test_data(), "$.fieldG.field5");
  pod.fieldG.field5.set_ue_2(enum1::field0);
  TEST_IMPL(pod, test_data(), "$.fieldG.field5.ue_2");
  pod.fieldG.field5.set_ue_2(enum1::field1);
  TEST_IMPL(pod, test_data());
}

TEST(fatal_debug, fieldS) {
  auto pod = test_data();
  pod.fieldS = {{"123", "456"}, {"abc", "ABC"}, {"ghi", "GHI"}};
  TEST_IMPL(
    pod,
    test_data(),
    "$.fieldS[\"0x676869\"]" /* missing */,
    "$.fieldS[\"0x646566\"]" /* extra  */);
}

TEST(fatal_debug, struct_binary) {
  struct_binary lhs;
  lhs.bi = "hello";
  struct_binary rhs;
  rhs.bi = "world";

  TEST_IMPL(lhs, rhs, "$.bi");
}

namespace {
struct UniqueHelper {
  template <typename T, typename... Args>
  static std::unique_ptr<T> build(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }
};

struct SharedHelper {
  template <typename T, typename... Args>
  static std::shared_ptr<T> build(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }
};

struct SharedConstHelper {
  template <typename T, typename... Args>
  static std::shared_ptr<T const> build(Args&&... args) {
    return std::make_shared<T const>(std::forward<Args>(args)...);
  }
};
} // namespace

template <typename Structure, typename Helper>
void ref_test() {
  Structure allNull;
  allNull.aStruct = Helper::template build<structA>();
  allNull.aList = Helper::template build<std::deque<std::string>>();
  allNull.aSet = Helper::template build<std::unordered_set<std::string>>();
  allNull.aMap =
      Helper::template build<std::unordered_map<std::string, std::string>>();
  allNull.aUnion = Helper::template build<unionA>();
  allNull.anOptionalStruct = nullptr;
  allNull.anOptionalList = nullptr;
  allNull.anOptionalSet = nullptr;
  allNull.anOptionalMap = nullptr;
  allNull.anOptionalUnion = nullptr;

  Structure allDefault;
  allDefault.aStruct = Helper::template build<structA>();
  allDefault.anOptionalStruct = Helper::template build<structA>();
  allDefault.aList = Helper::template build<std::deque<std::string>>();
  allDefault.anOptionalList = Helper::template build<std::deque<std::string>>();
  allDefault.aSet = Helper::template build<std::unordered_set<std::string>>();
  allDefault.anOptionalSet =
      Helper::template build<std::unordered_set<std::string>>();
  allDefault.aMap =
      Helper::template build<std::unordered_map<std::string, std::string>>();
  allDefault.anOptionalMap =
      Helper::template build<std::unordered_map<std::string, std::string>>();
  allDefault.aUnion = Helper::template build<unionA>();
  allDefault.anOptionalUnion = Helper::template build<unionA>();
  TEST_IMPL(
      allNull,
      allDefault,
      "$.anOptionalStruct" /* extra */,
      "$.anOptionalList" /* extra */,
      "$.anOptionalSet" /* extra */,
      "$.anOptionalMap" /* extra */,
      "$.anOptionalUnion" /* extra */);
  TEST_IMPL(
      allDefault,
      allNull,
      "$.anOptionalStruct" /* missing */,
      "$.anOptionalList" /* missing */,
      "$.anOptionalSet" /* missing */,
      "$.anOptionalMap" /* missing */,
      "$.anOptionalUnion" /* missing */);
}

TEST(fatal_debug, ref_unique) {
  ref_test<hasRefUnique, UniqueHelper>();
}

TEST(fatal_debug, ref_shared) {
  ref_test<hasRefShared, SharedHelper>();
}

TEST(fatal_debug, ref_shared_const) {
  ref_test<hasRefSharedConst, SharedConstHelper>();
}

#undef TEST_IMPL

} // namespace cpp_reflection
} // namespace test_cpp2
