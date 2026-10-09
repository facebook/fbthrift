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

#include <thrift/test/CppAllocatorTest.h>

#include <memory_resource>
#include <string>
#include <type_traits>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/op/Encode.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/gen-cpp2/CppAllocatorUnionTest_types.h>

#include <gtest/gtest.h>

namespace apache::thrift::test {

static_assert(
    std::uses_allocator_v<UnionPmr, std::pmr::polymorphic_allocator<UnionPmr>>);
static_assert(std::is_nothrow_move_constructible_v<UnionPmr>);
// FIXME(ytj): Should be constructible: std::uses_allocator promises
// allocator-extended constructors, so std::pmr::vector<UnionPmr> and a
// cpp.use_allocator UnionPmr field of a cpp.allocator struct fail to compile.
static_assert(!std::is_constructible_v<UnionPmr, const PmrByteAlloc&>);
static_assert(
    !std::is_constructible_v<UnionPmr, const UnionPmr&, const PmrByteAlloc&>);
static_assert(
    !std::is_constructible_v<UnionPmr, UnionPmr&&, const PmrByteAlloc&>);

namespace {

const char* const kTooLong =
    "This is too long for the small string optimization";

// With null_memory_resource(), anything that allocates from the default
// resource instead of a union's allocator throws.
class DefaultResourceGuard {
 public:
  explicit DefaultResourceGuard(std::pmr::memory_resource* r)
      : prev_(std::pmr::set_default_resource(r)) {}
  DefaultResourceGuard(const DefaultResourceGuard&) = delete;
  DefaultResourceGuard& operator=(const DefaultResourceGuard&) = delete;
  ~DefaultResourceGuard() { std::pmr::set_default_resource(prev_); }

 private:
  std::pmr::memory_resource* prev_;
};

// A default-constructed pmr allocator takes the default resource, so this
// makes a union whose allocator is on `r` without an allocator constructor.
template <class T>
T makeOn(std::pmr::memory_resource* r) {
  DefaultResourceGuard guard(r);
  return T();
}

template <class T>
std::pmr::memory_resource* resourceOf(const T& t) {
  return t.get_allocator().resource();
}

std::pmr::monotonic_buffer_resource makeResource() {
  return std::pmr::monotonic_buffer_resource(std::pmr::new_delete_resource());
}

} // namespace

// A default-constructed counting allocator has its own counter, so it compares
// unequal to every other one.
TEST(CppAllocatorUnionTest, CopyCtorUsesSelectOnContainerCopyConstruction) {
  const CountingUnion src;
  const CountingUnion copy(src);
  EXPECT_EQ(copy.get_allocator(), src.get_allocator());
}

TEST(CppAllocatorUnionTest, MoveCtorKeepsSourceAllocator) {
  auto res = makeResource();
  auto src = makeOn<UnionPmr>(&res);
  src.aa_string_ref() = UPmrString(kTooLong, &res);

  const UnionPmr moved(std::move(src));
  EXPECT_EQ(resourceOf(moved), &res);
  EXPECT_EQ(resourceOf(*moved.aa_string_ref()), &res);

  CountingUnion counting;
  const auto alloc = counting.get_allocator();
  const CountingUnion movedCounting(std::move(counting));
  EXPECT_EQ(movedCounting.get_allocator(), alloc);
}

TEST(CppAllocatorUnionTest, MovesTakeOverRefMember) {
  RefChildUnionPmr u;
  const auto* s = &u.ref_s_ref().emplace(kTooLong);

  const RefChildUnionPmr moved(std::move(u));
  // FIXME(ytj): Should hand the pointer over instead of rebuilding the
  // pointee.
  EXPECT_NE(&*moved.ref_s_ref(), s);
}

TEST(CppAllocatorUnionTest, SettersUseUnionAllocator) {
  auto res = makeResource();
  auto res2 = makeResource();
  auto u = makeOn<UnionPmr>(&res);
  const UPmrString onRes2(kTooLong, &res2);

  u.set_aa_string(kTooLong);
  // FIXME(ytj): Should be &res, the union's allocator; likewise below.
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), std::pmr::get_default_resource());
  EXPECT_EQ(u.set_aa_string(onRes2), onRes2);
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), std::pmr::get_default_resource());
  u.set_aa_string(UPmrString(kTooLong, &res2));
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), &res2);
}

TEST(CppAllocatorUnionTest, CopyAssignKeepsTargetAllocator) {
  auto res = makeResource();
  auto res2 = makeResource();
  auto src = makeOn<UnionPmr>(&res);
  src.aa_string_ref() = UPmrString(kTooLong, &res);
  auto dst = makeOn<UnionPmr>(&res2);

  dst = src;
  EXPECT_EQ(dst, src);
  EXPECT_EQ(resourceOf(dst), &res2);
  // FIXME(ytj): Should be &res2, the target's allocator.
  EXPECT_EQ(resourceOf(*dst.aa_string_ref()), std::pmr::get_default_resource());
}

TEST(CppAllocatorUnionTest, MoveAssignKeepsTargetAllocator) {
  auto res = makeResource();
  auto res2 = makeResource();
  auto src = makeOn<UnionPmr>(&res);
  src.aa_string_ref() = UPmrString(kTooLong, &res);
  const UnionPmr expected = src;
  auto dst = makeOn<UnionPmr>(&res2);

  dst = std::move(src);
  EXPECT_EQ(dst, expected);
  EXPECT_EQ(resourceOf(dst), &res2);
  // FIXME(ytj): Should be &res2: the allocators differ,
  // so the member must be rebuilt.
  EXPECT_EQ(resourceOf(*dst.aa_string_ref()), &res);
}

TEST(CppAllocatorUnionTest, SwapUnequalAllocatorsKeepsEachSidesAllocator) {
  auto res = makeResource();
  auto res2 = makeResource();
  auto a = makeOn<UnionPmr>(&res);
  a.aa_string_ref() = UPmrString(kTooLong, &res);
  auto b = makeOn<UnionPmr>(&res2);
  b.aa_string_ref() = UPmrString("b", &res2);
  const UnionPmr expectedA = b;
  const UnionPmr expectedB = a;

  swap(a, b);
  EXPECT_EQ(a, expectedA);
  EXPECT_EQ(b, expectedB);
  EXPECT_EQ(resourceOf(a), &res);
  EXPECT_EQ(resourceOf(b), &res2);
  // FIXME(ytj): Should be each union's own allocator.
  EXPECT_EQ(resourceOf(*a.aa_string_ref()), &res2);
  EXPECT_EQ(resourceOf(*b.aa_string_ref()), &res);
}

TEST(CppAllocatorUnionTest, FieldRefEnsureEmplaceAssignUseUnionAllocator) {
  auto res = makeResource();
  auto res2 = makeResource();
  const UPmrString onRes2(kTooLong, &res2);
  auto u = makeOn<UnionPmr>(&res);

  u.aa_string_ref().ensure().assign(kTooLong);
  // FIXME(ytj): Should be &res, the union's allocator; likewise below.
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), std::pmr::get_default_resource());
  EXPECT_EQ(u.aa_string_ref().emplace(onRes2), onRes2);
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), std::pmr::get_default_resource());

  u.not_a_container_ref() = 1;
  u.aa_string_ref() = onRes2; // inactive: emplace
  EXPECT_EQ(resourceOf(*u.aa_string_ref()), std::pmr::get_default_resource());
}

TEST(CppAllocatorUnionTest, OpDecodeUsesUnionAllocator) {
  auto res = makeResource();
  UnionPmr src;
  src.aa_string_ref() = kTooLong;
  const IOBufChain input{
      folly::IOBuf::copyBuffer(BinarySerializer::serialize<std::string>(src))};
  // The chain reader takes op::decode's generic UnionDecode path, which
  // creates the member through union_field_ref::ensure(); other readers call
  // the generated readNoXfer.
  BinaryProtocolChainReader reader;
  reader.setInput(&input);
  auto dst = makeOn<UnionPmr>(&res);

  op::decode<type::union_t<UnionPmr>>(reader, dst);
  EXPECT_EQ(dst, src);
  // FIXME(ytj): Should be &res, the union's allocator.
  EXPECT_EQ(resourceOf(*dst.aa_string_ref()), std::pmr::get_default_resource());
}

namespace {

// SimpleJSON is size-omitting; Compact and Binary are size-prefixed.
using DeserializeSerializers = ::testing::Types<
    apache::thrift::SimpleJSONSerializer,
    apache::thrift::CompactSerializer,
    apache::thrift::BinarySerializer>;

template <typename Serializer>
class CppAllocatorUnionDeserializeTest : public ::testing::Test {};
TYPED_TEST_SUITE(CppAllocatorUnionDeserializeTest, DeserializeSerializers);

TYPED_TEST(CppAllocatorUnionDeserializeTest, MembersLandOnUnionAllocator) {
  auto res = makeResource();
  UnionPmr src;
  src.aa_string_ref() = kTooLong;
  const auto bytes = TypeParam::template serialize<std::string>(src);
  auto dst = makeOn<UnionPmr>(&res);

  TypeParam::deserialize(bytes, dst);
  EXPECT_EQ(dst, src);
  // FIXME(ytj): Should be &res, the union's allocator.
  EXPECT_EQ(resourceOf(*dst.aa_string_ref()), std::pmr::get_default_resource());
}

} // namespace

} // namespace apache::thrift::test
