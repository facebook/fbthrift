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

#include <thrift/compiler/validator.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace apache::thrift::compiler;

namespace {

class ValidatorTest : public testing::Test{};

}

TEST_F(ValidatorTest, run_validator) {
  class fake_validator : public validator {
   public:
    using visitor::visit;
    bool visit(t_program const* const program) final {
      EXPECT_TRUE(validator::visit(program));
      add_error(50, "sadface");
      return true;
    }
  };

  t_program program("/path/to/file.thrift");
  auto errors = run_validator<fake_validator>(&program);
  EXPECT_THAT(errors, testing::ElementsAre(
        "[FAILURE:/path/to/file.thrift:50] sadface"));
}
