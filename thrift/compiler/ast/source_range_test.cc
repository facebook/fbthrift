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
void add_offsets(t_program& program, const std::string& content) {
  size_t offset = 0;
  for (const auto& c : content) {
    offset++;
    if (c == '\n') {
      program.add_line_offset(offset);
    }
  }
}

class SourceLocTest : public ::testing::Test {};
class SourceRangeTest : public ::testing::Test {};

TEST_F(SourceLocTest, Offset) {
  t_program program("");

  add_offsets(
      program,
      "struct A {\n"
      "  1: optional A a (cpp.ref);\n"
      "}\n");

  EXPECT_EQ(source_loc().offset(), t_program::noffset); // unknown loc.
  EXPECT_EQ(
      source_loc(0, 7, program).offset(),
      t_program::noffset); // unknown line, ignored col.
  EXPECT_EQ(source_loc(1, 0, program).offset(), 0); // first line, unknown col.

  EXPECT_EQ(source_loc(1, 1, program).offset(), 0); // struct begin
  EXPECT_EQ(source_loc(3, 2, program).offset(), 41); // struct end

  EXPECT_EQ(source_loc(2, 3, program).offset(), 13); // field begin
  EXPECT_EQ(source_loc(2, 29, program).offset(), 39); // field end

  EXPECT_EQ(source_loc(100, 1, program).offset(), t_program::noffset);
}

TEST_F(SourceRangeTest, ProgramMismatch) {
  t_program program1("");
  t_program program2("");

  source_loc loc1(1, 0, program1);
  source_loc loc2(1, 0, program2);

  EXPECT_THROW(source_range(loc1, loc2), std::invalid_argument);
}

} // namespace
} // namespace apache::thrift::compiler
