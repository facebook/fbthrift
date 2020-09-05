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

#include <thrift/lib/cpp2/reflection/diff.h>

#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

#include <folly/String.h>
#include <folly/portability/GTest.h>

#include <sstream>
#include <string>
#include <vector>

using namespace test_cpp2::cpp_reflection;

static struct3 test_data() {
  structA a1;
  *a1.a_ref() = 99;
  *a1.b_ref() = "abc";
  structA a2;
  *a2.a_ref() = 1001;
  *a2.b_ref() = "foo";
  structA a3;
  *a3.a_ref() = 654;
  *a3.b_ref() = "bar";
  structA a4;
  *a4.a_ref() = 9791;
  *a4.b_ref() = "baz";
  structA a5;
  *a5.a_ref() = 111;
  *a5.b_ref() = "gaz";

  structB b1;
  *b1.c_ref() = 1.23;
  *b1.d_ref() = true;
  structB b2;
  *b2.c_ref() = 9.8;
  *b2.d_ref() = false;
  structB b3;
  *b3.c_ref() = 10.01;
  *b3.d_ref() = true;
  structB b4;
  *b4.c_ref() = 159.73;
  *b4.d_ref() = false;
  structB b5;
  *b5.c_ref() = 468.02;
  *b5.d_ref() = true;

  struct3 pod;

  *pod.fieldA_ref() = 141;
  *pod.fieldB_ref() = "this is a test";
  *pod.fieldC_ref() = enum1::field0;
  *pod.fieldD_ref() = enum2::field1_2;
  pod.fieldE_ref()->set_ud(5.6);
  pod.fieldF_ref()->set_us_2("this is a variant");
  pod.fieldG_ref()->field0 = 98;
  pod.fieldG_ref()->field1_ref() = "hello, world";
  *pod.fieldG_ref()->field2_ref() = enum1::field2;
  pod.fieldG_ref()->field3 = enum2::field0_2;
  pod.fieldG_ref()->field4_ref() = {};
  pod.fieldG_ref()->field4_ref()->set_ui(19937);
  pod.fieldG_ref()->field5_ref()->set_ue_2(enum1::field1);
  // fieldH intentionally left empty
  *pod.fieldI_ref() = {3, 5, 7, 9};
  *pod.fieldJ_ref() = {"a", "b", "c", "d"};
  *pod.fieldK_ref() = {};
  pod.fieldL_ref()->push_back(a1);
  pod.fieldL_ref()->push_back(a2);
  pod.fieldL_ref()->push_back(a3);
  pod.fieldL_ref()->push_back(a4);
  pod.fieldL_ref()->push_back(a5);
  *pod.fieldM_ref() = {2, 4, 6, 8};
  *pod.fieldN_ref() = {"w", "x", "y", "z"};
  *pod.fieldO_ref() = {};
  *pod.fieldP_ref() = {b1, b2, b3, b4, b5};
  *pod.fieldQ_ref() = {{"a1", a1}, {"a2", a2}, {"a3", a3}};
  *pod.fieldR_ref() = {};
  *pod.fieldS_ref() = {{"123", "456"}, {"abc", "ABC"}, {"def", "DEF"}};

  return pod;
}

#define TEST_IMPL(LHS, RHS, EXPECTED)                                \
  do {                                                               \
    using namespace apache::thrift;                                  \
    auto const& expected = folly::stripLeftMargin(EXPECTED);         \
    std::ostringstream actualStream;                                 \
    debug_equals(LHS, RHS, make_diff_output_callback(actualStream)); \
    EXPECT_EQ(expected, actualStream.str());                         \
  } while (false)

TEST(fatal_diff, equal) {
  TEST_IMPL(test_data(), test_data(), "");
}

TEST(fatal_diff, Failure) {
  auto pod = test_data();
  struct3 pod1, pod2;
  *pod1.fieldR_ref()["a"].c_ref() = 1;
  *pod1.fieldR_ref()["b"].c_ref() = 2;
  *pod1.fieldR_ref()["c"].c_ref() = 3;
  *pod1.fieldR_ref()["d"].c_ref() = 4;
  *pod2.fieldR_ref()["d"].c_ref() = 4;
  *pod2.fieldR_ref()["c"].c_ref() = 3;
  *pod2.fieldR_ref()["b"].c_ref() = 2;
  *pod2.fieldR_ref()["a"].c_ref() = 1;
  TEST_IMPL(pod1, pod2, "");
}

TEST(fatal_diff, fieldA) {
  auto pod = test_data();
  *pod.fieldA_ref() = 90;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldA:
    - 90
    + 141
    )");
  *pod.fieldA_ref() = 141;
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldB) {
  auto pod = test_data();
  *pod.fieldB_ref() = "should mismatch";
  TEST_IMPL(pod, test_data(), R"(
    $.fieldB:
    - "should mismatch"
    + "this is a test"
  )");
  *pod.fieldB_ref() = "this is a test";
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldC) {
  auto pod = test_data();
  *pod.fieldC_ref() = enum1::field2;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldC:
    - field2
    + field0
  )");
  *pod.fieldC_ref() = enum1::field0;
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldE) {
  auto pod = test_data();
  pod.fieldE_ref()->__clear();
  TEST_IMPL(pod, test_data(), R"(
    $.fieldE.ud:
    + 5.6
  )");
  TEST_IMPL(test_data(), pod, R"(
    $.fieldE.ud:
    - 5.6
  )");
  pod.fieldE_ref()->set_ui(5);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldE.ui:
    - 5
    $.fieldE.ud:
    + 5.6
  )");
  pod.fieldE_ref()->__clear();
  TEST_IMPL(pod, test_data(), R"(
    $.fieldE.ud:
    + 5.6
  )");
  pod.fieldE_ref()->set_ud(4);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldE.ud:
    - 4
    + 5.6
  )");
  pod.fieldE_ref()->set_ud(5.6);
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldH) {
  auto pod = test_data();
  pod.fieldH_ref()->set_ui_2(3);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldH.ui_2:
    - 3
  )");
  pod.fieldH_ref()->__clear();
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldI) {
  auto pod = test_data();
  pod.fieldI_ref()[0] = 4;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldI[0]:
    - 4
    + 3
  )");
  pod.fieldI_ref()[0] = 3;
  TEST_IMPL(pod, test_data(), "");
  pod.fieldI_ref()[2] = 10;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldI[2]:
    - 10
    + 7
  )");
  pod.fieldI_ref()->push_back(11);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldI[2]:
    - 10
    + 7
    $.fieldI[4]:
    - 11
  )");
  pod.fieldI_ref()->clear();
  TEST_IMPL(pod, test_data(), R"(
    $.fieldI[0]:
    + 3
    $.fieldI[1]:
    + 5
    $.fieldI[2]:
    + 7
    $.fieldI[3]:
    + 9
  )");
}

TEST(fatal_diff, fieldM) {
  auto pod = test_data();
  pod.fieldM_ref()->clear();
  TEST_IMPL(
      pod,
      test_data(),
      R"(
        $.fieldM[2]:
        + 2
        $.fieldM[4]:
        + 4
        $.fieldM[6]:
        + 6
        $.fieldM[8]:
        + 8
      )");
  pod.fieldM_ref()->insert(11);
  pod.fieldM_ref()->insert(12);
  pod.fieldM_ref()->insert(13);
  pod.fieldM_ref()->insert(14);
  TEST_IMPL(
      pod,
      test_data(),
      R"(
        $.fieldM[11]:
        - 11
        $.fieldM[12]:
        - 12
        $.fieldM[13]:
        - 13
        $.fieldM[14]:
        - 14
        $.fieldM[2]:
        + 2
        $.fieldM[4]:
        + 4
        $.fieldM[6]:
        + 6
        $.fieldM[8]:
        + 8
      )");
  *pod.fieldM_ref() = *test_data().fieldM_ref();
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldQ) {
  auto pod = test_data();
  pod.fieldQ_ref()->clear();
  TEST_IMPL(
      pod,
      test_data(),
      R"(
        $.fieldQ["a1"]:
        + <struct>{
        +   a: 99,
        +   b: "abc"
        + }
        $.fieldQ["a2"]:
        + <struct>{
        +   a: 1001,
        +   b: "foo"
        + }
        $.fieldQ["a3"]:
        + <struct>{
        +   a: 654,
        +   b: "bar"
        + }
      )");
  structA a1;
  *a1.a_ref() = 1;
  *a1.b_ref() = "1";
  structA a2;
  *a2.a_ref() = 2;
  *a2.b_ref() = "2";
  structA a3;
  *a3.a_ref() = 3;
  *a3.b_ref() = "3";
  pod.fieldQ_ref()["A1"] = a1;
  pod.fieldQ_ref()["A2"] = a2;
  pod.fieldQ_ref()["A3"] = a3;
  TEST_IMPL(
      pod,
      test_data(),
      R"(
        $.fieldQ["A1"]:
        - <struct>{
        -   a: 1,
        -   b: "1"
        - }
        $.fieldQ["A2"]:
        - <struct>{
        -   a: 2,
        -   b: "2"
        - }
        $.fieldQ["A3"]:
        - <struct>{
        -   a: 3,
        -   b: "3"
        - }
        $.fieldQ["a1"]:
        + <struct>{
        +   a: 99,
        +   b: "abc"
        + }
        $.fieldQ["a2"]:
        + <struct>{
        +   a: 1001,
        +   b: "foo"
        + }
        $.fieldQ["a3"]:
        + <struct>{
        +   a: 654,
        +   b: "bar"
        + }
      )");
  *pod.fieldQ_ref() = *test_data().fieldQ_ref();
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldG_field0) {
  auto pod = test_data();
  pod.fieldG_ref()->field0 = 12;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field0:
    - 12
    + 98
  )");
  pod.fieldG_ref()->field0 = 98;
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldG_field1) {
  auto pod = test_data();
  pod.fieldG_ref()->field1_ref() = "should mismatch";
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field1:
    - "should mismatch"
    + "hello, world"
  )");
  pod.fieldG_ref()->field1_ref() = "hello, world";
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldG_field2) {
  auto pod = test_data();
  *pod.fieldG_ref()->field2_ref() = enum1::field1;
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field2:
    - field1
    + field2
  )");
  *pod.fieldG_ref()->field2_ref() = enum1::field2;
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldG_field5) {
  auto pod = test_data();
  pod.fieldG_ref()->field5_ref()->set_ui_2(5);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field5.ui_2:
    - 5
    $.fieldG.field5.ue_2:
    + field1
  )");
  pod.fieldG_ref()->field5_ref()->__clear();
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field5.ue_2:
    + field1
  )");
  pod.fieldG_ref()->field5_ref()->set_ue_2(enum1::field0);
  TEST_IMPL(pod, test_data(), R"(
    $.fieldG.field5.ue_2:
    - field0
    + field1
  )");
  pod.fieldG_ref()->field5_ref()->set_ue_2(enum1::field1);
  TEST_IMPL(pod, test_data(), "");
}

TEST(fatal_diff, fieldS) {
  auto pod = test_data();
  *pod.fieldS_ref() = {{"123", "456"}, {"abc", "ABC"}, {"ghi", "GHI"}};
  TEST_IMPL(pod, test_data(), R"(
    $.fieldS["0x676869"]:
    - "GHI"
    $.fieldS["0x646566"]:
    + "DEF"
  )");
}

TEST(fatal_diff, struct_binary) {
  struct_binary lhs;
  *lhs.bi_ref() = "hello";
  struct_binary rhs;
  *rhs.bi_ref() = "world";

  TEST_IMPL(lhs, rhs, R"(
    $.bi:
    - "hello"
    + "world"
  )");
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
  TEST_IMPL(allNull, allDefault, R"(
    $.anOptionalStruct:
    + <struct>{
    +   a: 0,
    +   b: ""
    + }
    $.anOptionalList:
    + <list>[]
    $.anOptionalSet:
    + <set>{}
    $.anOptionalMap:
    + <map>{}
    $.anOptionalUnion:
    + <variant>{}
  )");
  TEST_IMPL(allDefault, allNull, R"(
    $.anOptionalStruct:
    - <struct>{
    -   a: 0,
    -   b: ""
    - }
    $.anOptionalList:
    - <list>[]
    $.anOptionalSet:
    - <set>{}
    $.anOptionalMap:
    - <map>{}
    $.anOptionalUnion:
    - <variant>{}
  )");
}

TEST(fatal_diff, struct_ref_unique) {
  ref_test<hasRefUnique, UniqueHelper>();
}

TEST(fatal_diff, ref_shared) {
  ref_test<hasRefShared, SharedHelper>();
}

TEST(fatal_diff, ref_shared_const) {
  ref_test<hasRefSharedConst, SharedConstHelper>();
}

TEST(fatal_diff, optional_members) {
  struct1 field1Set;
  field1Set.field1_ref() = "1";
  struct1 field1Unset;
  struct1 field1SetButNotIsset;
  field1SetButNotIsset.field1_ref() = "2";
  field1SetButNotIsset.__isset.field1 = false;
  struct1 field1SetDefault;
  field1SetDefault.__isset.field1 = true;
  TEST_IMPL(field1Set, field1Unset, R"(
    $.field1:
    - "1"
  )");
  TEST_IMPL(field1Unset, field1Set, R"(
    $.field1:
    + "1"
  )");
  TEST_IMPL(field1Set, field1SetButNotIsset, R"(
    $.field1:
    - "1"
  )");
  TEST_IMPL(field1SetButNotIsset, field1Set, R"(
    $.field1:
    + "1"
  )");
  TEST_IMPL(field1Unset, field1SetButNotIsset, "");
  TEST_IMPL(field1SetButNotIsset, field1Unset, "");
  TEST_IMPL(field1Set, field1SetDefault, R"(
    $.field1:
    - "1"
    + ""
  )");
  TEST_IMPL(field1SetDefault, field1Set, R"(
    $.field1:
    - ""
    + "1"
  )");
  TEST_IMPL(field1SetDefault, field1SetButNotIsset, R"(
    $.field1:
    - ""
  )");
  TEST_IMPL(field1SetButNotIsset, field1SetDefault, R"(
    $.field1:
    + ""
  )");
  TEST_IMPL(field1Unset, field1SetDefault, R"(
    $.field1:
    + ""
  )");
  TEST_IMPL(field1SetDefault, field1Unset, R"(
    $.field1:
    - ""
  )");
}

TEST(fatal_diff, variant_ref_unique) {
  variantHasRefUnique allNull;
  allNull.set_aStruct() = nullptr;

  variantHasRefUnique allDefault;
  allDefault.set_aStruct();
  TEST_IMPL(allNull, allDefault, R"(
    $.aStruct:
    + <struct>{
    +   a: 0,
    +   b: ""
    + }
  )");
  TEST_IMPL(allDefault, allNull, R"(
    $.aStruct:
    - <struct>{
    -   a: 0,
    -   b: ""
    - }
  )");
}

#undef TEST_IMPL
