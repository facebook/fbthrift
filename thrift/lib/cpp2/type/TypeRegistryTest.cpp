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

#include <thrift/lib/cpp2/type/TypeRegistry.h>

#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/type/UniversalName.h>

namespace apache::thrift::type {
namespace {

TEST(TypeRegistry, NotImplemented) {
  EXPECT_THROW(TypeRegistry::generated().store({}, {}), std::out_of_range);
  EXPECT_THROW(
      TypeRegistry::generated().store<StandardProtocol::Compact>({}),
      std::out_of_range);
  EXPECT_THROW(TypeRegistry::generated().load({}, {}), std::out_of_range);
  EXPECT_THROW(TypeRegistry::generated().load({}), std::out_of_range);
}

} // namespace
} // namespace apache::thrift::type
