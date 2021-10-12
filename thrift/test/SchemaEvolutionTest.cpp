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

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/gen-cpp2/schema_evolution_test_for_each_field.h>

using namespace apache::thrift;

template <class T>
struct EvolutionTest : testing::Test {
  using Serializer = T;
};

using Serializers =
    ::testing::Types<BinarySerializer, CompactSerializer, SimpleJSONSerializer>;

TYPED_TEST_CASE(EvolutionTest, Serializers);

TYPED_TEST(EvolutionTest, evolution) {
  using Serializer = typename TestFixture::Serializer;
  cpp2::Old oldObj;
  for_each_field(oldObj, [](auto& meta, auto ref) { ref = *meta.name_ref(); });

  cpp2::New newObj;
  Serializer::deserialize(
      Serializer::template serialize<std::string>(oldObj), newObj);

  EXPECT_EQ(
      newObj.unqualified_to_unqualified_ref(),
      *oldObj.unqualified_to_unqualified_ref());
  EXPECT_EQ(
      newObj.unqualified_to_optional_ref(),
      *oldObj.unqualified_to_optional_ref());
  EXPECT_EQ(
      newObj.unqualified_to_required_ref(),
      *oldObj.unqualified_to_required_ref());

  EXPECT_EQ(
      newObj.optional_to_unqualified_ref(),
      *oldObj.optional_to_unqualified_ref());
  EXPECT_EQ(
      newObj.optional_to_optional_ref(), *oldObj.optional_to_optional_ref());
  EXPECT_EQ(
      newObj.optional_to_required_ref(), *oldObj.optional_to_required_ref());

  EXPECT_EQ(
      newObj.required_to_unqualified_ref(),
      *oldObj.required_to_unqualified_ref());
  EXPECT_EQ(
      newObj.required_to_optional_ref(), *oldObj.required_to_optional_ref());
  EXPECT_EQ(
      newObj.required_to_required_ref(), *oldObj.required_to_required_ref());

  if (std::is_same_v<Serializer, SimpleJSONSerializer>) {
    EXPECT_EQ(newObj.unqualified_new_ref(), "");
    EXPECT_EQ(newObj.required_new_ref(), "");

    EXPECT_FALSE(newObj.unqualified_new_ref().is_set());
    EXPECT_FALSE(newObj.optional_new_ref().has_value());
    EXPECT_TRUE(newObj.required_new_ref().has_value());
  } else {
    EXPECT_EQ(newObj.unqualified_new_ref(), *oldObj.unqualified_old_ref());
    EXPECT_EQ(newObj.optional_new_ref(), *oldObj.optional_old_ref());
    EXPECT_EQ(newObj.required_new_ref(), *oldObj.required_old_ref());
  }

  EXPECT_EQ(newObj.unqualified_added_ref(), "");
  EXPECT_EQ(newObj.required_added_ref(), "");

  EXPECT_FALSE(newObj.unqualified_added_ref().is_set());
  EXPECT_FALSE(newObj.optional_added_ref().has_value());
  EXPECT_TRUE(newObj.required_added_ref().has_value());
}
