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

#include <thrift/compiler/ast/source_range.h>

#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/t_program.h>

namespace apache::thrift::compiler {
namespace {

// Simulates parsing a thrift file, only adding the offsets to the program.
void addOffsets(t_program& program, const std::string& content) {
  size_t offset = 0;
  for (const auto& c : content) {
    offset++;
    if (c == '\n') {
      program.add_line_offset(offset);
    }
  }
}

template <typename T>
void testOrdering(int expected_cmp, const T& lhs, const T& rhs) {
  EXPECT_EQ(lhs == rhs, expected_cmp == 0);
  EXPECT_EQ(lhs != rhs, expected_cmp != 0);
  EXPECT_EQ(lhs < rhs, expected_cmp < 0);
  EXPECT_EQ(lhs <= rhs, expected_cmp <= 0);
  EXPECT_EQ(lhs > rhs, expected_cmp > 0);
  EXPECT_EQ(lhs >= rhs, expected_cmp >= 0);

  EXPECT_EQ(rhs == lhs, 0 == expected_cmp);
  EXPECT_EQ(rhs != lhs, 0 != expected_cmp);
  EXPECT_EQ(rhs < lhs, 0 < expected_cmp);
  EXPECT_EQ(rhs <= lhs, 0 <= expected_cmp);
  EXPECT_EQ(rhs > lhs, 0 > expected_cmp);
  EXPECT_EQ(rhs >= lhs, 0 >= expected_cmp);
}

template <typename T>
std::string toString(const T& rhs) {
  std::ostringstream os;
  os << rhs;
  return os.str();
}

class SourceLocTest : public ::testing::Test {
 protected:
  void testLocOrdering(
      int expected_cmp, const source_loc& lhs, const source_loc& rhs) {
    EXPECT_EQ(lhs.compare(rhs), expected_cmp);
    testOrdering(expected_cmp, lhs, rhs);
  }
};

class SourceRangeTest : public ::testing::Test {
 protected:
  void testRangeOrdering(
      int expected_cmp, const source_range& lhs, const source_range& rhs) {
    // Should match comparing begin() than end()
    int actual_cmp = lhs.begin().compare(rhs.begin());
    if (actual_cmp == 0) {
      actual_cmp = lhs.end().compare(rhs.end());
    }
    EXPECT_EQ(actual_cmp, expected_cmp);

    testOrdering(expected_cmp, lhs, rhs);
  }
};

TEST_F(SourceLocTest, Offset) {
  t_program program("");

  addOffsets(
      program,
      "struct A {\n"
      "  1: optional A a (cpp.ref);\n"
      "}\n");

  EXPECT_EQ(source_loc().offset(), t_program::noffset); // unknown loc.
  EXPECT_EQ(
      source_loc(program, 0, 7).offset(),
      t_program::noffset); // unknown line, ignored col.
  EXPECT_EQ(source_loc(program, 1, 0).offset(), 0); // first line, unknown col.

  EXPECT_EQ(source_loc(program, 1, 1).offset(), 0); // struct begin
  EXPECT_EQ(source_loc(program, 3, 2).offset(), 41); // struct end

  EXPECT_EQ(source_loc(program, 2, 3).offset(), 13); // field begin
  EXPECT_EQ(source_loc(program, 2, 29).offset(), 39); // field end

  EXPECT_EQ(source_loc(program, 100, 1).offset(), t_program::noffset);
}

TEST_F(SourceLocTest, Order) {
  t_program prog1("prog/1/src.thrift");
  t_program prog2("prog/2/src.thrift");
  testLocOrdering(0, source_loc(), source_loc());
  testLocOrdering(-1, source_loc(), source_loc(prog1, 1, 1));
  testLocOrdering(0, source_loc(prog1, 7, 13), source_loc(prog1, 7, 13));
  testLocOrdering(-1, source_loc(prog1, 7, 12), source_loc(prog1, 7, 13));
  testLocOrdering(1, source_loc(prog1, 7, 12), source_loc(prog1, 6, 13));
  testLocOrdering(-1, source_loc(prog1, 7, 12), source_loc(prog2, 6, 13));

  // For stability and consistency with operator==, Pointer address is a tie
  // breaker between same paths.
  t_program prog1b("prog/1/src.thrift");
  testLocOrdering(
      &prog1 > &prog1b ? 1 : -1,
      source_loc(prog1, 1, 1),
      source_loc(prog1b, 1, 1));
}

TEST_F(SourceLocTest, Print) {
  t_program prog1("prog/1/src.thrift");
  EXPECT_EQ(toString(source_loc()), "");
  EXPECT_EQ(toString(source_loc(prog1, 0, 0)), "prog/1/src.thrift");
  EXPECT_EQ(toString(source_loc(prog1, 7, 0)), "prog/1/src.thrift:7");
  EXPECT_EQ(toString(source_loc(prog1, 7, 13)), "prog/1/src.thrift:7:13");
}

TEST_F(SourceRangeTest, ProgramMismatch) {
  t_program program1("");
  t_program program2("");

  source_loc loc1(program1, 1, 1);
  source_loc loc2(program2, 2, 1);

  EXPECT_THROW(source_range(loc1, loc2), std::invalid_argument);
}

TEST_F(SourceRangeTest, Order) {
  t_program prog1("prog/1/src.thrift");
  t_program prog2("prog/2/src.thrift");
  testRangeOrdering(0, source_range(), source_range());
  testRangeOrdering(-1, source_range(), source_range(prog1, 1, 1, 1, 2));
  testRangeOrdering(
      0, source_range(prog1, 7, 13, 7, 14), source_range(prog1, 7, 13, 7, 14));
  testRangeOrdering(
      -1, source_range(prog1, 7, 13, 7, 14), source_range(prog1, 7, 13, 7, 15));
  testRangeOrdering(
      1, source_range(prog1, 7, 13, 8, 14), source_range(prog1, 7, 13, 7, 15));
  testRangeOrdering(
      -1, source_range(prog1, 7, 13, 8, 14), source_range(prog1, 7, 14, 7, 15));
  testRangeOrdering(
      1, source_range(prog1, 8, 13, 8, 14), source_range(prog1, 7, 14, 7, 15));
  testRangeOrdering(
      -1, source_range(prog1, 8, 13, 8, 14), source_range(prog2, 7, 14, 7, 15));

  // For stability and consistency with operator==, Pointer address is a tie
  // breaker between same paths.
  t_program prog1b("prog/1/src.thrift");
  testRangeOrdering(
      &prog1 > &prog1b ? 1 : -1,
      source_range(prog1, 1, 1, 1, 1),
      source_range(prog1b, 1, 1, 1, 1));
}

} // namespace
} // namespace apache::thrift::compiler
