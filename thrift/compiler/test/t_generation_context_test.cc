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

#include <string>

#include <folly/portability/GTest.h>

#include <thrift/compiler/generate/t_generator.h>

using namespace apache::thrift::compiler;

TEST(TProgram, SetOutPath) {
  auto context = t_generation_context{};

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
  auto context_1 = t_generation_context{out_dir_1, non_absolute_path};
  EXPECT_EQ(expect_1, context_1.get_out_path());

  const std::string expect_2 = "./";
  auto context_2a = t_generation_context{out_dir_2, non_absolute_path};
  EXPECT_EQ(expect_2, context_2a.get_out_path());
  auto context_2b = t_generation_context{out_dir_3, non_absolute_path};
  EXPECT_EQ(expect_2, context_2b.get_out_path());

  const std::string expect_3 = "./dir/";
  auto context_3a = t_generation_context{out_dir_4, non_absolute_path};
  EXPECT_EQ(expect_3, context_3a.get_out_path());
  auto context_3b = t_generation_context{out_dir_5, non_absolute_path};
  EXPECT_EQ(expect_3, context_3b.get_out_path());

  const std::string expect_4 = "/this/is/a/dir/";
  auto context_4a = t_generation_context{out_dir_6, absolute_path};
  EXPECT_EQ(expect_4, context_4a.get_out_path());
  auto context_4b = t_generation_context{out_dir_7, absolute_path};
  EXPECT_EQ(expect_4, context_4b.get_out_path());
}
