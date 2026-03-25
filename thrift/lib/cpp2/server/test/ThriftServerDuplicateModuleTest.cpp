/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

using namespace ::testing;
using namespace apache::thrift;

namespace {
class TestModule : public apache::thrift::ServerModule {
 public:
  std::string getName() const override { return "TestModule"; }
};
} // namespace

TEST(ThriftServerDuplicateModuleTest, ThrowsDuplicateModuleNameError) {
  EXPECT_THROW(
      {
        try {
          ThriftServer server;
          server.addModule(std::make_unique<TestModule>());
          server.addModule(std::make_unique<TestModule>());
        } catch (const DuplicateModuleNameError& ex) {
          EXPECT_THAT(
              ex.what(), HasSubstr("Duplicate module name: TestModule"));
          throw;
        }
      },
      DuplicateModuleNameError);
}

TEST(ThriftServerDuplicateModuleTest, CatchableAsInvalidArgument) {
  ThriftServer server;
  server.addModule(std::make_unique<TestModule>());
  EXPECT_THROW(
      server.addModule(std::make_unique<TestModule>()), std::invalid_argument);
}
