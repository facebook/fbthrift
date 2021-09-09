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

#include <stdexcept>
#include <string>
#include <vector>

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

TEST(TProgram, GetNamespace) {
  t_program program("");

  const std::string expect_1 = "this.namespace";
  program.set_namespace("java", expect_1);
  program.set_namespace("java.swift", expect_1);

  const std::string expect_2 = "other.namespace";
  program.set_namespace("cpp", expect_2);
  program.set_namespace("py", expect_2);

  const std::string expect_3 = "";

  EXPECT_EQ(expect_1, program.get_namespace("java"));
  EXPECT_EQ(expect_1, program.get_namespace("java.swift"));
  EXPECT_EQ(expect_2, program.get_namespace("cpp"));
  EXPECT_EQ(expect_2, program.get_namespace("py"));
  EXPECT_EQ(expect_3, program.get_namespace("Non existent"));
}

TEST(TProgram, AddInclude) {
  t_program program("");

  const std::string expect_1 = "tprogramtest1";
  const std::string rel_file_path_1 = "./" + expect_1 + ".thrift";
  const std::string full_file_path_1 = "/this/is/a/dir/" + expect_1 + ".thrift";
  const std::string expect_2 = "tprogramtest2";
  const std::string full_file_path_2 = "/this/is/a/dir/" + expect_2 + ".thrift";
  const auto expect = std::vector<std::string>{expect_1, expect_2};

  auto program_1 = program.add_include(full_file_path_1, rel_file_path_1, 0);
  auto program_2 = program.add_include(full_file_path_2, full_file_path_2, 0);
  const auto& includes = program.get_included_programs();

  auto included_names = std::vector<std::string>();
  for (auto include : includes) {
    included_names.push_back(include->name());
  }
  EXPECT_EQ(expect, included_names);
}

TEST(TProgram, SetIncludePrefix) {
  t_program program("");

  const std::string dir_path_1 = "/this/is/a/dir";
  const std::string dir_path_2 = "/this/is/a/dir/";

  const std::string expect = "/this/is/a/dir/";

  program.set_include_prefix(dir_path_1);
  EXPECT_EQ(expect, program.include_prefix());
  program.set_include_prefix(dir_path_2);
  EXPECT_EQ(expect, program.include_prefix());
}

TEST(TProgram, ComputeNameFromFilePath) {
  t_program program("");

  const std::string expect = "tprogramtest";
  const std::string file_path_1 = expect;
  const std::string file_path_2 = expect + ".thrift";
  const std::string file_path_3 = "/this/is/a/path/" + expect + ".thrift";

  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_1));
  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_2));
  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_3));
}

TEST(TProgram, GetByteOffset) {
  t_program program("");

  addOffsets(
      program,
      "struct A {\n"
      "  1: optional A a (cpp.ref);\n"
      "}\n");

  EXPECT_EQ(program.get_byte_offset(0),
            t_program::noffset); // unknown line.
  EXPECT_EQ(
      program.get_byte_offset(0, 7),
      t_program::noffset); // unknown line, ignored col.
  EXPECT_EQ(program.get_byte_offset(1), 0); // first line.
  EXPECT_EQ(program.get_byte_offset(1, 0), 0); // first line, no extra offset.

  EXPECT_EQ(program.get_byte_offset(1, 0), 0); // struct begin
  EXPECT_EQ(program.get_byte_offset(3, 1), 41); // struct end

  EXPECT_EQ(program.get_byte_offset(2, 2), 13); // field begin
  EXPECT_EQ(program.get_byte_offset(2, 28), 39); // field end

  // We can't compute the offset for a line past the end of the file.
  EXPECT_EQ(program.get_byte_offset(100, 0), t_program::noffset);
  // We can compute the offset if the line_offset is past the end of the line.
  EXPECT_EQ(program.get_byte_offset(1, 14), 14);
  // We currently can compute the line_offset when past the end of the line and
  // file, though this is techically UB.
  // TODO(afuller): Consider returning noffset in this case.
  EXPECT_EQ(program.get_byte_offset(1, 100), 100);
}

} // namespace
} // namespace apache::thrift::compiler
