/*
 * Copyright 2016 Facebook, Inc.
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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <thrift/compiler/parse/t_program.h>

/**
 * t_program functions are protected so we need
 * an inheritance to access the functions
 */
class t_program_fake : public t_program {
 public:
  using t_program::compute_name_from_file_path;
  using t_program::set_include_prefix;
  using t_program::t_program;
};

TEST(TProgram, GetNamespace) {
  auto program = t_program_fake("");

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

TEST(TProgram, SetOutPath) {
  auto program = t_program_fake("");

  const bool absolute_path = true;
  const bool non_absolute_path = false;
  const std::string out_dir_1 = "";
  const std::string out_dir_2 = ".";
  const std::string out_dir_3 = "./";
  const std::string out_dir_4 = "./dir";
  const std::string out_dir_5 = "./dir/";
  const std::string out_dir_6 = "/this/is/a/dir";
  const std::string out_dir_7 = "/this/is/a/dir/";

  const std::string expect_1 = "";
  program.set_out_path(out_dir_1, non_absolute_path);
  EXPECT_EQ(expect_1, program.get_out_path());

  const std::string expect_2 = "./";
  program.set_out_path(out_dir_2, non_absolute_path);
  EXPECT_EQ(expect_2, program.get_out_path());
  program.set_out_path(out_dir_3, non_absolute_path);
  EXPECT_EQ(expect_2, program.get_out_path());

  const std::string expect_3 = "./dir/";
  program.set_out_path(out_dir_4, non_absolute_path);
  EXPECT_EQ(expect_3, program.get_out_path());
  program.set_out_path(out_dir_5, non_absolute_path);
  EXPECT_EQ(expect_3, program.get_out_path());

  const std::string expect_4 = "/this/is/a/dir/";
  program.set_out_path(out_dir_6, absolute_path);
  EXPECT_EQ(expect_4, program.get_out_path());
  program.set_out_path(out_dir_7, absolute_path);
  EXPECT_EQ(expect_4, program.get_out_path());
}

TEST(TProgram, AddInclude) {
  auto program = t_program_fake("");

  const std::string expect_1 = "tprogramtest1";
  const std::string rel_file_path_1 = "./" + expect_1 + ".thrift";
  const std::string full_file_path_1 = "/this/is/a/dir/" + expect_1 + ".thrift";
  const std::string expect_2 = "tprogramtest2";
  const std::string full_file_path_2 = "/this/is/a/dir/" + expect_2 + ".thrift";
  const auto expect = std::vector<std::string>{expect_1, expect_2};

  program.add_include(full_file_path_1, rel_file_path_1);
  program.add_include(full_file_path_2, full_file_path_2);
  const auto& includes = program.get_includes();

  auto included_names = std::vector<std::string>();
  for (auto include : includes) {
    included_names.push_back(include->get_name());
  }
  EXPECT_EQ(expect, included_names);
}

TEST(TProgram, SetIncludePrefix) {
  auto program = t_program_fake("");

  const std::string dir_path_1 = "/this/is/a/dir";
  const std::string dir_path_2 = "/this/is/a/dir/";

  const std::string expect = "/this/is/a/dir/";

  program.set_include_prefix(dir_path_1);
  EXPECT_EQ(expect, program.get_include_prefix());
  program.set_include_prefix(dir_path_2);
  EXPECT_EQ(expect, program.get_include_prefix());
}

TEST(TProgram, ComputeNameFromFilePath) {
  auto program = t_program_fake("");

  const std::string expect = "tprogramtest";
  const std::string file_path_1 = expect;
  const std::string file_path_2 = expect + ".thrift";
  const std::string file_path_3 = "/this/is/a/path/" + expect + ".thrift";

  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_1));
  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_2));
  EXPECT_EQ(expect, program.compute_name_from_file_path(file_path_3));
}
