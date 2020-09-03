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

#include <thrift/test/CppAllocatorTest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/gen-cpp2/CppAllocatorTest_types.h>

#include <folly/portability/GTest.h>

using namespace apache::thrift::test;

static const char* kTooLong =
    "This is too long for the small string optimization";

TEST(CppAllocatorTest, UsesAllocator) {
  ScopedThrowingAlloc alloc;
  UsesAllocatorParent s(alloc);

  EXPECT_THROW(s.child_ref()->aa_list_ref()->emplace_back(42), std::bad_alloc);
  EXPECT_THROW(s.child_ref()->aa_set_ref()->emplace(42), std::bad_alloc);
  EXPECT_THROW(s.child_ref()->aa_map_ref()->emplace(42, 42), std::bad_alloc);
  EXPECT_THROW(
      s.child_ref()->aa_string_ref()->assign(kTooLong), std::bad_alloc);

  EXPECT_NO_THROW(s.child_ref()->not_aa_list_ref()->emplace_back(42));
  EXPECT_NO_THROW(s.child_ref()->not_aa_set_ref()->emplace(42));
  EXPECT_NO_THROW(s.child_ref()->not_aa_map_ref()->emplace(42, 42));
  EXPECT_NO_THROW(s.child_ref()->not_aa_string_ref()->assign(kTooLong));
}

TEST(CppAllocatorTest, GetAllocator) {
  ScopedStatefulAlloc alloc(42);

  NoAllocatorVia s1(alloc);
  EXPECT_EQ(alloc, s1.get_allocator());

  YesAllocatorVia s2(alloc);
  EXPECT_EQ(alloc, s2.get_allocator());
}

TEST(CppAllocatorTest, AllocatorVia) {
  NoAllocatorVia s1;
  YesAllocatorVia s2;
  EXPECT_GT(sizeof(s1), sizeof(s2));
}

TEST(CppAllocatorTest, Deserialize) {
  using serializer = apache::thrift::CompactSerializer;

  HasContainerFields s1;
  s1.aa_list_ref() = {1, 2, 3};
  s1.aa_set_ref() = {1, 2, 3};
  s1.aa_map_ref() = {{1, 1}, {2, 2}, {3, 3}};

  auto str = serializer::serialize<std::string>(s1);

  ScopedStatefulAlloc alloc(42);
  HasContainerFields s2(alloc);
  EXPECT_EQ(s2.get_allocator(), alloc);
  EXPECT_EQ(s2.aa_list_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_set_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_ref()->get_allocator(), alloc);

  serializer::deserialize(str, s2);
  EXPECT_EQ(s2.aa_list_ref(), (StatefulAllocVector<int32_t>{1, 2, 3}));
  EXPECT_EQ(s2.aa_set_ref(), (StatefulAllocSet<int32_t>{1, 2, 3}));
  EXPECT_EQ(
      s2.aa_map_ref(),
      (StatefulAllocMap<int32_t, int32_t>{{1, 1}, {2, 2}, {3, 3}}));

  EXPECT_EQ(s2.get_allocator(), alloc);
  EXPECT_EQ(s2.aa_list_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_set_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_ref()->get_allocator(), alloc);
}

TEST(CppAllocatorTest, UsesTypedef) {
  ScopedStatefulAlloc alloc(42);
  UsesTypedef s(alloc);
  EXPECT_EQ(alloc, s.get_allocator());
}

TEST(CppAllocatorTest, DeserializeNested) {
  using serializer = apache::thrift::CompactSerializer;

  HasNestedContainerFields s1;
  s1.aa_map_of_map_ref() = {{42, {{42, 42}}}};
  s1.aa_map_of_set_ref() = {{42, {42}}};

  auto str = serializer::serialize<std::string>(s1);

  ScopedStatefulAlloc alloc(42);
  HasNestedContainerFields s2(alloc);

  EXPECT_EQ(s2.get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_of_map_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_of_set_ref()->get_allocator(), alloc);

  serializer::deserialize(str, s2);
  EXPECT_EQ(s2.aa_map_of_map_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_of_map_ref()->at(42).get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_of_set_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_of_set_ref()->at(42).get_allocator(), alloc);
}

TEST(CppAllocatorTest, DeserializeSortedUniqueConstructible) {
  using serializer = apache::thrift::CompactSerializer;

  HasSortedUniqueConstructibleFields s1;
  s1.aa_set_ref() = {1, 2, 3};
  s1.aa_map_ref() = {{1, 1}, {2, 2}, {3, 3}};

  auto str = serializer::serialize<std::string>(s1);

  ScopedStatefulAlloc alloc(42);
  HasSortedUniqueConstructibleFields s2(alloc);
  EXPECT_EQ(s2.get_allocator(), alloc);
  EXPECT_EQ(s2.aa_set_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_ref()->get_allocator(), alloc);

  serializer::deserialize(str, s2);
  EXPECT_EQ(s2.aa_set_ref(), (StatefulAllocSortedVectorSet<int32_t>{1, 2, 3}));
  EXPECT_EQ(
      s2.aa_map_ref(),
      (StatefulAllocSortedVectorMap<int32_t, int32_t>{{1, 1}, {2, 2}, {3, 3}}));

  EXPECT_EQ(s2.get_allocator(), alloc);
  EXPECT_EQ(s2.aa_set_ref()->get_allocator(), alloc);
  EXPECT_EQ(s2.aa_map_ref()->get_allocator(), alloc);
}
