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

#include <folly/String.h>
#include <folly/experimental/TestUtil.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/codemod/file_manager.h>
#include <thrift/compiler/compiler.h>

namespace apache::thrift::compiler {

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

void write_file(const std::string& path, const std::string& content) {
  EXPECT_TRUE(folly::writeFile(content, path.c_str()));
}

std::string read_file(const std::string& path) {
  std::string content;
  EXPECT_TRUE(folly::readFile(path.c_str(), content));
  return content;
}

// Testing overloading of < operator in replacement struct.
TEST(FileManagerTest, replacement_less_than) {
  codemod::replacement a{2, 4, ""};
  codemod::replacement b{2, 5, ""};
  codemod::replacement c{3, 5, ""};
  codemod::replacement d{5, 7, ""};

  EXPECT_TRUE(a < b); // Same begin, different end
  EXPECT_TRUE(b < c); // Same end, different begin
  EXPECT_TRUE(a < c); // Overlapping
  EXPECT_TRUE(a < d); // Non-overlapping
}

// Basic test of apply_replacements functionality, without traversing AST.
TEST(FileManagerTest, apply_replacements_test) {
  const folly::test::TemporaryFile tempFile(
      "FileManagerTest_apply_replacements_test");
  const std::string path = tempFile.path().string();
  const std::string initial_content = folly::stripLeftMargin(R"(
      struct A {
        1: optional A a (cpp.ref);
      } (cpp.noexcept_move)
      )");

  write_file(path, initial_content);

  t_program program(path);
  add_offsets(program, initial_content);

  codemod::file_manager fm(program);

  fm.add(
      {program.get_byte_offset(2, 2),
       program.get_byte_offset(2, 28),
       "@cpp.Ref{cpp.RefType.Unique}\n  1: optional string a;"});
  fm.add({program.get_byte_offset(3, 1), program.get_byte_offset(3, 21), ""});

  fm.apply_replacements();

  EXPECT_EQ(read_file(path), folly::stripLeftMargin(R"(
      struct A {
        @cpp.Ref{cpp.RefType.Unique}
        1: optional string a;
      }
      )"));
}

// Testing correct line and column after including another thrift file.
TEST(FileManagerTest, correct_location_after_include_test) {
  const folly::test::TemporaryFile tempFile1(
      "FileManagerTest_correct_location_after_include_test");
  const std::string path1 = tempFile1.path().string();
  const std::string initial_content1 = folly::stripLeftMargin(R"(
      struct A {
        1: string a;
      }
      )");

  const folly::test::TemporaryFile tempFile2(
      "FileManagerTest_correct_location_after_include_test2");
  const std::string path2 = tempFile2.path().string();
  const std::string initial_content2 = folly::stripLeftMargin(R"(
      include ")" + path1 + R"("
      struct B {
        1: string b;
      }
      )");

  write_file(path1, initial_content1);
  write_file(path2, initial_content2);

  const auto program_bundle =
      parse_and_get_program({"thrift1", "--gen", "mstch_cpp2", path2});

  EXPECT_TRUE(program_bundle);

  const auto program = program_bundle->root_program();
  const auto struct_src_range = program->structs()[0]->src_range();
  const auto field_src_range = program->structs()[0]->fields()[0].src_range();

  EXPECT_EQ(struct_src_range.begin().line(), 2);
  EXPECT_EQ(struct_src_range.begin().column(), 1);
  EXPECT_EQ(struct_src_range.end().line(), 4);
  EXPECT_EQ(struct_src_range.end().column(), 2);

  EXPECT_EQ(field_src_range.begin().line(), 3);
  EXPECT_EQ(field_src_range.begin().column(), 3);
  EXPECT_EQ(field_src_range.end().line(), 3);
  EXPECT_EQ(field_src_range.end().column(), 15);
}

} // namespace apache::thrift::compiler
