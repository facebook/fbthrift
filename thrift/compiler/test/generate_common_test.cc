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
#include <vector>

#include <folly/portability/GTest.h>

#include <thrift/compiler/generate/common.h>

namespace apache {
namespace thrift {
namespace compiler {

TEST(GenerateCommon, SplitNamespace) {
  const std::vector<std::string> namespaces{
      "",
      "this",
      "this.is",
      "this.is.valid",
  };

  const std::vector<std::vector<std::string>> expected{
      {},
      {"this"},
      {"this", "is"},
      {"this", "is", "valid"},
  };

  std::vector<std::vector<std::string>> splits;
  splits.reserve(namespaces.size());
  for (const auto& ns : namespaces) {
    splits.push_back(split_namespace(ns));
  }

  EXPECT_EQ(expected, splits);
}

TEST(GenerateCommon, StripComments) {
  std::vector<std::string> validCases{
      {},
      {"no comments"},
      {"/*foo*/"},
      {"one/*foo*/comment"},
      {"this/*foo*/has/*bar*/three/*baz*/comments"},
  };

  const std::vector<std::string> expected{
      {},
      {"no comments"},
      {""},
      {"onecomment"},
      {"thishasthreecomments"},
  };

  for (auto& s : validCases) {
    strip_comments(s);
  }

  EXPECT_EQ(expected, validCases);

  std::string unpaired{"unpaired/*foo*/comments/*baz* /"};
  EXPECT_THROW(strip_comments(unpaired), std::runtime_error);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
