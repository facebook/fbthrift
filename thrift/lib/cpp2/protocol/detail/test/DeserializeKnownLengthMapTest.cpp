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

#include <memory>
#include <memory_resource>
#include <vector>

#include <gtest/gtest.h>
#include <folly/container/F14Map.h>
#include <folly/container/sorted_vector_types.h>
#include <folly/container/test/TrackingTypes.h>

#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>

using namespace apache::thrift;

namespace {

using TrackedKey = folly::test::Tracked<0>;
using TrackedValue = folly::test::Tracked<1>;

using TransparentTrackedKeyHash = folly::test::TransparentTrackedHash<0>;
using TransparentTrackedKeyEqual = folly::test::TransparentTrackedEqual<0>;

class DeserializeKnownLengthMapTest : public testing::Test {
 public:
  void SetUp() override { folly::test::resetTracking(); }
};

enum TrackedValues {
  KEY = 1,
  VALUE = 2,
};

void deserializeTrackedKey(TrackedKey& key) {
  key.val_ = TrackedValues::KEY;
}

void deserializeTrackedValue(TrackedValue& value) {
  value.val_ = TrackedValues::VALUE;
}

template <typename Vec>
void deserializeVecOfTrackedValues(Vec& vec) {
  vec.resize(1);
  vec[0].val_ = TrackedValues::VALUE;
}

template <typename K, typename M, typename H, typename E, typename A>
class MinimalMap : private folly::F14FastMap<K, M, H, E, A> {
  using Super = folly::F14FastMap<K, M, H, E, A>;

 public:
  using typename Super::allocator_type;
  using typename Super::iterator;
  using typename Super::key_type;
  using typename Super::mapped_type;
  using typename Super::size_type;
  using typename Super::value_type;

  MinimalMap() = default;

  using Super::operator[];
  using Super::get_allocator;
  using Super::size;

  // `MinimalMap` has `emplace` method, but not `emplace_hint`.
  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return Super::emplace(std::forward<Args>(args)...);
  }
};

template <typename K, typename M, typename H, typename E, typename A>
class HintOnlyMap : private folly::F14FastMap<K, M, H, E, A> {
  using Super = folly::F14FastMap<K, M, H, E, A>;

 public:
  using typename Super::allocator_type;
  using typename Super::iterator;
  using typename Super::key_type;
  using typename Super::mapped_type;
  using typename Super::size_type;
  using typename Super::value_type;

  HintOnlyMap() = default;

  using Super::operator[];
  using Super::end;
  using Super::get_allocator;
  using Super::size;

  // `HintOnlyMap` has `emplace_hint` method, but not `try_emplace`.
  template <class... Args>
  iterator emplace_hint(iterator hint, Args&&... args) {
    return Super::emplace_hint(hint, std::forward<Args>(args)...);
  }
};

} // namespace

namespace folly::test {

bool operator<(const TrackedKey& a, const TrackedKey& b) {
  return a.val_ < b.val_;
}

} // namespace folly::test

TEST_F(DeserializeKnownLengthMapTest, SortedUniqueConstuctCount) {
  using Map = folly::sorted_vector_map<TrackedKey, TrackedValue>;

  static_assert(detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeTrackedValue);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(DeserializeKnownLengthMapTest, EmplaceHintConstuctCount) {
  using Map = folly::F14FastMap<TrackedKey, TrackedValue>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeTrackedValue);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(DeserializeKnownLengthMapTest, EmplaceHintConstuctCountVec) {
  using Vec = std::vector<TrackedValue>;
  using Map = folly::F14FastMap<TrackedKey, Vec>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeVecOfTrackedValues<Vec>);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(
    DeserializeKnownLengthMapTest, EmplaceHintWithoutTryEmplaceConstuctCount) {
  using Allocator = std::allocator<std::pair<const TrackedKey, TrackedValue>>;
  using Map = HintOnlyMap<
      TrackedKey,
      TrackedValue,
      TransparentTrackedKeyHash,
      TransparentTrackedKeyEqual,
      Allocator>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(!detail::pm::map_try_emplace_is_invocable_v<Map>);
  static_assert(!detail::pm::map_supports_in_place_deserialize_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeTrackedValue);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
}

TEST_F(DeserializeKnownLengthMapTest, EmplaceConstuctCount) {
  using Allocator = std::allocator<std::pair<const TrackedKey, TrackedValue>>;
  using Map = MinimalMap<
      TrackedKey,
      TrackedValue,
      TransparentTrackedKeyHash,
      TransparentTrackedKeyEqual,
      Allocator>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(!detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeTrackedValue);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(DeserializeKnownLengthMapTest, EmplaceConstuctCountVec) {
  using Vec = std::vector<TrackedValue>;
  using Allocator = std::allocator<std::pair<const TrackedKey, Vec>>;
  using Map = MinimalMap<
      TrackedKey,
      Vec,
      TransparentTrackedKeyHash,
      TransparentTrackedKeyEqual,
      Allocator>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(!detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(!detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeVecOfTrackedValues<Vec>);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

#if FOLLY_HAS_MEMORY_RESOURCE

TEST_F(DeserializeKnownLengthMapTest, EmplaceHintConstuctCountVecAlloc) {
  using Map =
      folly::pmr::F14FastMap<TrackedKey, std::pmr::vector<TrackedValue>>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map,
      1,
      deserializeTrackedKey,
      deserializeVecOfTrackedValues<std::pmr::vector<TrackedValue>>);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(DeserializeKnownLengthMapTest, SortedUniqueConstuctCountVecAlloc) {
  using Map =
      folly::pmr::sorted_vector_map<TrackedKey, std::pmr::vector<TrackedValue>>;

  static_assert(detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(detail::alloc_should_propagate_map<Map>);

  std::pmr::monotonic_buffer_resource resource;
  Map map{Map::allocator_type(&resource)};
  detail::pm::deserialize_known_length_map(
      map,
      1,
      deserializeTrackedKey,
      deserializeVecOfTrackedValues<std::pmr::vector<TrackedValue>>);
  ASSERT_EQ(map.size(), 1);
  EXPECT_EQ(map.begin()->second.get_allocator().resource(), &resource);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

TEST_F(DeserializeKnownLengthMapTest, EmplaceConstuctCountVecAlloc) {
  using Vec = std::pmr::vector<TrackedValue>;
  using Allocator =
      std::pmr::polymorphic_allocator<std::pair<const TrackedKey, Vec>>;
  using Map = MinimalMap<
      TrackedKey,
      Vec,
      TransparentTrackedKeyHash,
      TransparentTrackedKeyEqual,
      Allocator>;

  static_assert(!detail::pm::sorted_unique_constructible_v<Map>);
  static_assert(!detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(detail::alloc_should_propagate_map<Map>);

  Map map;
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, deserializeVecOfTrackedValues<Vec>);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(
      TrackedKey::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 1,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 1}));
  EXPECT_EQ(
      TrackedValue::counts(),
      (folly::test::Counts{/* copyConstruct */ 0,
                           /* moveConstruct */ 0,
                           /* copyConvert */ 0,
                           /* moveConvert */ 0,
                           /* copyAssign */ 0,
                           /* moveAssign */ 0,
                           /* defaultConstruct */ 1,
                           /* destroyed */ 0}));
}

class CountingResource : public std::pmr::memory_resource {
 public:
  int allocs = 0;

 private:
  void* do_allocate(std::size_t bytes, std::size_t align) override {
    ++allocs;
    return std::pmr::new_delete_resource()->allocate(bytes, align);
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
    std::pmr::new_delete_resource()->deallocate(p, bytes, align);
  }
  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }
};

class DefaultResourceGuard {
 public:
  explicit DefaultResourceGuard(std::pmr::memory_resource* r)
      : prev_(std::pmr::set_default_resource(r)) {}
  ~DefaultResourceGuard() { std::pmr::set_default_resource(prev_); }

 private:
  std::pmr::memory_resource* prev_;
};

// An allocator-aware mapped value that counts how it is built and destroyed.
struct AAValue {
  using allocator_type = std::pmr::polymorphic_allocator<>;

  AAValue() : AAValue(allocator_type{}) {}
  explicit AAValue(const allocator_type& alloc) : vals(alloc) { ++built; }
  AAValue(const AAValue& other) : vals(other.vals) { ++built; }
  AAValue(const AAValue& other, const allocator_type& alloc)
      : vals(other.vals, alloc) {
    ++built;
  }
  AAValue(AAValue&& other) noexcept : vals(std::move(other.vals)) { ++moved; }
  AAValue(AAValue&& other, const allocator_type& alloc)
      : vals(std::move(other.vals), alloc) {
    ++moved;
  }
  AAValue& operator=(const AAValue&) = default;
  AAValue& operator=(AAValue&&) = default;
  ~AAValue() { ++destroyed; }

  allocator_type get_allocator() const { return vals.get_allocator(); }

  std::pmr::vector<int> vals;

  static void reset() { built = moved = destroyed = 0; }
  static inline int built = 0;
  static inline int moved = 0;
  static inline int destroyed = 0;
};

TEST_F(DeserializeKnownLengthMapTest, EmplaceConstuctCountAllocAwareValue) {
  using Map = MinimalMap<
      TrackedKey,
      AAValue,
      TransparentTrackedKeyHash,
      TransparentTrackedKeyEqual,
      std::pmr::polymorphic_allocator<std::pair<const TrackedKey, AAValue>>>;

  static_assert(!detail::pm::map_emplace_hint_is_invocable_v<Map>);
  static_assert(detail::alloc_should_propagate_map<Map>);

  CountingResource arena;
  DefaultResourceGuard arenaGuard(&arena);
  Map map;
  CountingResource heap;
  DefaultResourceGuard heapGuard(&heap);
  AAValue::reset();
  detail::pm::deserialize_known_length_map(
      map, 1, deserializeTrackedKey, [](AAValue& value) {
        value.vals.assign({1, 2, 3});
      });

  EXPECT_EQ(AAValue::built, 1);
  EXPECT_EQ(AAValue::moved, 0);
  EXPECT_EQ(AAValue::destroyed, 0);
  EXPECT_EQ(heap.allocs, 0);
  ASSERT_EQ(map.size(), 1);
  TrackedKey key;
  deserializeTrackedKey(key);
  EXPECT_EQ(map[key].get_allocator().resource(), &arena);
}

#endif
