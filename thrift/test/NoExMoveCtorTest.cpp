/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/test/gen-cpp/NoExMoveCtorTest_types.h>
#include <thrift/test/gen-cpp2/NoExMoveCtorTest_types.h>

#include <type_traits>

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace thrift::test::noexcept_move_ctor;

TEST(TNoExceptMoveCtorTest, simple) {
  // Check the customized move ctor (by marking cpp.noexcept_move_ctor)
  // actually "move" data members.
  // NOTE: gcc may not do real "move" if std::string is too small
  size_t textSize = 2000;
  Simple s1;
  s1.d = "some text here";
  s1.d.resize(textSize);
  s1.__isset.i = true;
  Simple s2{std::move(s1)};
  EXPECT_EQ(s1.d.size(), 0);
  EXPECT_EQ(s2.d.size(), textSize);
  EXPECT_TRUE(s2.__isset.i);

  cpp2::Simple c2s1;
  c2s1.d = "some text here";
  c2s1.d.resize(textSize);
  c2s1.__isset.i = true;
  cpp2::Simple c2s2{std::move(c2s1)};
  EXPECT_EQ(c2s1.d.size(), 0);
  EXPECT_EQ(c2s2.d.size(), textSize);
  EXPECT_TRUE(c2s2.__isset.i);

  // Check thrift struct default move ctor not "noexcept" if a STL data
  // member is not "noexcept move ctor".
  bool nxMoveCtor = std::is_nothrow_constructible<mapx, mapx&&>::value;
  EXPECT_EQ(nxMoveCtor, false);
  nxMoveCtor = std::is_nothrow_constructible<Complex, Complex&&>::value;
  EXPECT_EQ(nxMoveCtor, false);
  nxMoveCtor = std::is_nothrow_constructible<cpp2::Complex,
      cpp2::Complex&&>::value;
  EXPECT_EQ(nxMoveCtor, false);

  // Check cpp.noexcept_move_ctor works.
  nxMoveCtor = std::is_nothrow_constructible<ComplexEx, ComplexEx&&>::value;
  EXPECT_EQ(nxMoveCtor, true);
  nxMoveCtor = std::is_nothrow_constructible<cpp2::ComplexEx,
      cpp2::ComplexEx&&>::value;
  EXPECT_EQ(nxMoveCtor, true);

  // Check thrift struct default move ctor not "noexcept" if a user defined
  // type data member is not "noexcept move ctor".
  nxMoveCtor = std::is_nothrow_constructible<TThrowCtorType,
      TThrowCtorType&&>::value;
  EXPECT_EQ(nxMoveCtor, false);
  nxMoveCtor = std::is_nothrow_constructible<cpp2::TThrowCtorType,
      cpp2::TThrowCtorType&&>::value;
  EXPECT_EQ(nxMoveCtor, false);
  nxMoveCtor = std::is_nothrow_constructible<MayThrowInDefMoveCtorStruct,
      MayThrowInDefMoveCtorStruct&&>::value;
  EXPECT_EQ(nxMoveCtor, false);
  nxMoveCtor = std::is_nothrow_constructible<cpp2::MayThrowInDefMoveCtorStruct,
      cpp2::MayThrowInDefMoveCtorStruct&&>::value;
  EXPECT_EQ(nxMoveCtor, false);

  // Check cpp.noexcept_move_ctor works.
  nxMoveCtor = std::is_nothrow_constructible<MayThrowInDefMoveCtorStructEx,
      MayThrowInDefMoveCtorStructEx&&>::value;
  EXPECT_EQ(nxMoveCtor, true);
  nxMoveCtor =
      std::is_nothrow_constructible<cpp2::MayThrowInDefMoveCtorStructEx,
          cpp2::MayThrowInDefMoveCtorStructEx&&>::value;
  EXPECT_EQ(nxMoveCtor, true);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
