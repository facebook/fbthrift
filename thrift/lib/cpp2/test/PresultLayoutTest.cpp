/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/frozen/FrozenUtil.h>

#include <thrift/lib/cpp2/test/gen-cpp2/presult_layout_test_layouts.h>
#include <thrift/lib/cpp2/test/gen-cpp2/presult_layout_test_types.h>

using namespace ::presult::test;
using namespace ::apache::thrift;

using Matched = ThriftPresult<
    false,
    FieldData<1, protocol::T_I32, int32_t*>,
    FieldData<5, protocol::T_STRING, std::string*>>;
using Unmatched = ThriftPresult<
    false,
    FieldData<1, protocol::T_I32, int32_t*>,
    FieldData<2, protocol::T_STRING, std::string*>,
    FieldData<3, protocol::T_I64, int64_t*>,
    FieldData<5, protocol::T_STRING, std::string*>>;
using ResultExcA = ThriftPresult<
    true,
    FieldData<0, protocol::T_STRING, std::string*>,
    FieldData<5, protocol::T_STRUCT, ExceptionA>>;

using ResultExcB = ThriftPresult<
    true,
    FieldData<0, protocol::T_STRING, std::string*>,
    FieldData<1, protocol::T_STRUCT, StructA*>,
    FieldData<5, protocol::T_STRUCT, ExceptionA>>;

template <typename From, typename To>
void freezeAndThaw(From& a, To& b) {
  std::string out;
  frozen::freezeToString(a, out);

  frozen::Layout<To> layout;
  folly::ByteRange range((folly::StringPiece(out)));
  frozen::deserializeRootLayout(range, layout);
  layout.thaw({range.begin(), 0}, b);
}

TEST(PresultLayout, MatchedArgVersion) {
  Matched a, b;
  int32_t i32A = 8;
  std::string strA = "a";
  int32_t i32B;
  std::string strB;

  a.get<0>().value = &i32A;
  a.get<1>().value = &strA;
  b.get<0>().value = &i32B;
  b.get<1>().value = &strB;

  freezeAndThaw(a, b);

  ASSERT_EQ(i32A, i32B);
  ASSERT_EQ(strA, strB);
}

TEST(PresultLayout, UnmatchedArgVersion) {
  Matched a;
  Unmatched b;
  int32_t i32A = 8;
  std::string strA = "a";
  int32_t i32B;
  std::string strB;
  std::string strB3;
  int64_t i64B;

  a.get<0>().value = &i32A;
  a.get<1>().value = &strA;
  b.get<0>().value = &i32B;
  b.get<1>().value = &strB;
  b.get<2>().value = &i64B;
  b.get<3>().value = &strB3;

  freezeAndThaw(a, b);

  ASSERT_EQ(i32A, i32B);
  ASSERT_EQ(strA, strB3);
}

TEST(PresultLayout, ResultUnmatchedWithoutException) {
  ResultExcA a;
  ResultExcB b;
  std::string strA = "Message";
  std::string strB;
  StructA structB;

  a.get<0>().value = &strA;
  a.setIsSet(0);
  b.get<0>().value = &strB;
  b.get<1>().value = &structB;

  freezeAndThaw(a, b);

  ASSERT_EQ(strB, "Message");
  ASSERT_TRUE(b.getIsSet(0));
  ASSERT_FALSE(b.getIsSet(1));
  ASSERT_FALSE(b.getIsSet(2));
}

TEST(PresultLayout, ResultUnmatchedWithException) {
  ResultExcA a;
  ResultExcB b;
  ExceptionA excA;
  std::string strB;
  StructA structB;
  excA.code = -1;

  a.get<1>().ref() = excA;
  a.setIsSet(1);
  b.get<0>().value = &strB;
  b.get<1>().value = &structB;
  freezeAndThaw(a, b);

  ASSERT_EQ(b.get<2>().ref().code, -1);
  ASSERT_FALSE(b.getIsSet(0));
  ASSERT_FALSE(b.getIsSet(1));
  ASSERT_TRUE(b.getIsSet(2));
}

TEST(PreusltLayout, GetView) {
  Matched a;
  int32_t i32A = 8;
  std::string strA = "a";
  a.get<0>().value = &i32A;
  a.get<1>().value = &strA;

  frozen::Layout<Matched> layout;
  std::string out;
  frozen::freezeToString(a, out);
  folly::ByteRange range((folly::StringPiece(out)));
  frozen::deserializeRootLayout(range, layout);

  auto view = layout.view({range.begin(), 0});
  auto i32Layout = view.get<0>();
  auto strLayout = view.get<1>();
  ASSERT_EQ(i32Layout, 8);
  ASSERT_EQ(strLayout[0], 'a');
}
