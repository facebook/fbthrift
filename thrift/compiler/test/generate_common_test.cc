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

#include <thrift/compiler/generate/common.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

TEST(GenerateCommon, SplitNamespace) {
  const std::vector<std::string> namespaces{
    "",
    "this",
    "this.is",
    "this.is.valid",
  };

  const std::vector<std::vector<std::string>> expected{
    { "" },
    {"this"},
    {"this", "is"},
    {"this", "is", "valid"},
  };

  std::vector<std::vector<std::string>> splits;
  for (const auto& ns : namespaces) {
    splits.push_back(split_namespace(ns));
  }

  EXPECT_EQ(expected, splits);
}
