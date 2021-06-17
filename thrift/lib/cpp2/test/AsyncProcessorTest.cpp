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

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/test/gen-cpp2/Child.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Parent.h>

namespace apache::thrift::test {

namespace {
class Child : public ChildSvIf {
  MOCK_METHOD(std::unique_ptr<InteractionIf>, createInteraction, ());
};

const AsyncProcessorFactory::MethodMetadataMap& expectMethodMetadataMap(
    const AsyncProcessorFactory::CreateMethodMetadataResult&
        createMethodMetadataResult) {
  if (auto map = std::get_if<AsyncProcessorFactory::MethodMetadataMap>(
          &createMethodMetadataResult)) {
    return *map;
  }
  throw std::logic_error{"Expected createMethodMetadata to return a map"};
}

} // namespace

TEST(AsyncProcessorMetadataTest, ParentMetadata) {
  ParentSvIf service;
  auto createMethodMetadataResult = service.createMethodMetadata();
  auto& metadataMap = expectMethodMetadataMap(createMethodMetadataResult);

  EXPECT_EQ(metadataMap.size(), 2);
  EXPECT_NE(metadataMap.find("parentMethod1"), metadataMap.end());
  EXPECT_NE(metadataMap.find("parentMethod2"), metadataMap.end());
}

TEST(AsyncProcessorMetadataTest, ChildMetadata) {
  Child service;
  auto createMethodMetadataResult = service.createMethodMetadata();
  auto& metadataMap = expectMethodMetadataMap(createMethodMetadataResult);

  EXPECT_EQ(metadataMap.size(), 5);
  EXPECT_NE(metadataMap.find("parentMethod1"), metadataMap.end());
  EXPECT_NE(metadataMap.find("parentMethod2"), metadataMap.end());
  EXPECT_NE(metadataMap.find("childMethod1"), metadataMap.end());
  EXPECT_NE(metadataMap.find("childMethod2"), metadataMap.end());
  EXPECT_NE(
      metadataMap.find("Interaction.interactionMethod"), metadataMap.end());
}

} // namespace apache::thrift::test
