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

#include <thrift/conformance/cpp2/UniversalType.h>

#include <regex>

#include <fmt/core.h>
#include <folly/portability/GTest.h>

namespace apache::thrift::conformance {
namespace {

TEST(UniversalTypeTest, ValidateUniversalType) {
  std::regex pattern(fmt::format(
      "{0}(\\.{0})+\\/{1}(\\/{1})*(\\/{2})",
      "[a-z0-9-]+",
      "[a-z0-9_-]+",
      "[a-zA-Z0-9_-]+"));
  auto goods = {
      "foo.com/my/type",
      "foo.com/my/Type",
      "foo.com/my/other-type",
      "foo.com/my/other_type",
      "foo.com/m_y/type",
      "foo.com/m-y/type",
      "foo-bar.com/my/type",
      "foo.com/my/type/type",
      "1.2/3/4",
  };
  for (const auto& good : goods) {
    SCOPED_TRACE(good);
    EXPECT_TRUE(std::regex_match(good, pattern));
    validateUniversalType(good);
  }
  auto bads = {
      "my",
      "my/type",
      "foo/my/type",
      "foo.com/my",
      "Foo.com/my/type",
      "foo.Com/my/type",
      "foo.com/My/type",
      "foo.com:42/my/type",
      "foo%20.com/my/type",
      "foo.com/m%20y/type",
      "foo.com/my/ty%20pe",
      "@foo.com/my/type",
      ":@foo.com/my/type",
      ":foo.com/my/type",
      "user@foo.com/my/type",
      "user:pass@foo.com/my/type",
      "fbthrift://foo.com/my/type",
      ".com/my/type",
      "foo./my/type",
      "./my/type",
      "/my/type/type",
      "foo.com//type",
      "foo.com/my/",
      "foo.com/my//type",
      "foo.com/my/type?",
      "foo.com/my/type?a=",
      "foo.com/my/type?a=b",
      "foo.com/my/type#",
      "foo.com/my/type#1",
      "foo.com/m#y/type",
      "foo.com/my/type@",
      "foo.com/my/type@1",
      "foo.com/m@y/type",
      "foo.com/my/ty@pe/type",
      "foo_bar.com/my/type",
  };
  for (const auto& bad : bads) {
    SCOPED_TRACE(bad);
    EXPECT_FALSE(std::regex_match(bad, pattern));
    EXPECT_THROW(validateUniversalType(bad), std::invalid_argument);
  }
}

} // namespace
} // namespace apache::thrift::conformance
