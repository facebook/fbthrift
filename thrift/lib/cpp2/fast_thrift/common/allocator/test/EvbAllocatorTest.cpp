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

#include <thrift/lib/cpp2/fast_thrift/common/allocator/EvbAllocator.h>

#include <atomic>
#include <cstring>
#include <latch>
#include <vector>

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/lang/Align.h>
#include <folly/portability/GTest.h>

using namespace apache::thrift::fast_thrift::mem;

// ============================================================
// Helper
// ============================================================
struct LifecycleTracker {
  std::atomic<int>& ctorCount;
  std::atomic<int>& dtorCount;
  int value;

  explicit LifecycleTracker(std::atomic<int>& c, std::atomic<int>& d, int v = 0)
      : ctorCount(c), dtorCount(d), value(v) {
    ++ctorCount;
  }
  ~LifecycleTracker() { ++dtorCount; }
  LifecycleTracker(const LifecycleTracker&) = delete;
  LifecycleTracker& operator=(const LifecycleTracker&) = delete;
};

// ============================================================
// Group 1: Construction
// ============================================================

TEST(EvbAllocator, DefaultConstruction) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  EXPECT_EQ(alloc.bytesUsed(), 0);
  EXPECT_EQ(alloc.pageCount(), 1 + EvbAllocator::kPrewarmedFreePages);
}

TEST(EvbAllocator, FixedPageSize) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  EXPECT_EQ(alloc.pageSize(), EvbAllocator::kPageSize);
}

// ============================================================
// Group 2: Allocation
// ============================================================

TEST(EvbAllocator, SingleAllocation) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  auto* p = static_cast<int*>(alloc.allocate(sizeof(int), alignof(int)));
  *p = 0xDEADBEEF;
  EXPECT_EQ(*p, static_cast<int>(0xDEADBEEF));
}

TEST(EvbAllocator, AllAlignments) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  for (size_t a : {1, 2, 4, 8, 16, 32, 64, 128, 256}) {
    void* p = alloc.allocate(a, a);
    EXPECT_EQ(p, folly::align_floor(p, a)) << "align=" << a;
  }
}

TEST(EvbAllocator, SequentialAddresses) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  void* a = alloc.allocate(8, 8);
  void* b = alloc.allocate(8, 8);
  void* c = alloc.allocate(8, 8);
  EXPECT_LT(a, b);
  EXPECT_LT(b, c);
}

TEST(EvbAllocator, BytesUsed) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  EXPECT_EQ(alloc.bytesUsed(), 0);
  (void)alloc.allocate(64, 8);
  EXPECT_GE(alloc.bytesUsed(), 64);
}

// ============================================================
// Group 3: Page spill
// ============================================================

TEST(EvbAllocator, PageSpill) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  const size_t pageCountBefore = alloc.pageCount();
  alloc.activePage()->ptr = alloc.activePage()->end - 8;
  (void)alloc.allocate(64, 8);
  EXPECT_EQ(alloc.pageCount(), pageCountBefore);
}

TEST(EvbAllocator, LargeAllocation) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  auto* p = static_cast<char*>(alloc.allocate(512, 8));
  std::memset(p, 0xAB, 512);
  EXPECT_EQ(static_cast<unsigned char>(p[0]), 0xAB);
  EXPECT_EQ(static_cast<unsigned char>(p[511]), 0xAB);
}

TEST(EvbAllocator, OversizedAllocationDies) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  EXPECT_DEATH(
      (void)alloc.allocate(EvbAllocator::kMaxObjectSize + 1, 1),
      "Check failed");
}

TEST(EvbAllocator, OveralignedAllocationDies) {
  folly::EventBase evb;
  EvbAllocator alloc(&evb);
  EXPECT_DEATH(
      (void)alloc.allocate(1, EvbAllocator::kMaxObjectAlignment * 2),
      "Check failed");
}

TEST(EvbAllocator, DestructorFreesAllPages) {
  folly::EventBase evb;
  {
    EvbAllocator alloc(&evb);
    alloc.activePage()->ptr = alloc.activePage()->end - 8;
    (void)alloc.allocate(64, 8);
  }
  // ASAN validates no leak.
}

// ============================================================
// Group 4: Page struct
// ============================================================

TEST(Page, EvbStoredAtCreation) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  EXPECT_EQ(page->evb, &evb);
  EXPECT_EQ(page, folly::align_floor(page, Page::kPageSize));
  Page::destroy(page);
}

TEST(Page, BumpAllocation) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  void* a = page->allocate(64, 8);
  void* b = page->allocate(64, 8);
  EXPECT_LT(a, b);
  Page::destroy(page);
}

TEST(Page, BumpAllocationHasNoHeaderOverhead) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  auto* before = page->ptr;
  (void)page->allocate(1, 1);
  EXPECT_EQ(page->ptr - before, 1);
  Page::destroy(page);
}

TEST(Page, FromObjectRecoversAlignedPage) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  void* a = page->allocate(64, 8);
  void* b = page->allocate(256, 64);
  EXPECT_EQ(Page::fromObject(a), page);
  EXPECT_EQ(Page::fromObject(b), page);
  Page::destroy(page);
}

TEST(Page, OutstandingTracking) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  EXPECT_EQ(page->outstanding, 0);
  ++page->outstanding;
  EXPECT_EQ(page->outstanding, 1);
  --page->outstanding;
  EXPECT_EQ(page->outstanding, 0);
  EXPECT_EQ(page->evb, &evb);
  Page::destroy(page);
}

TEST(Page, EvbStoredCorrectly) {
  folly::EventBase evb;
  auto* page = Page::create(&evb);
  EXPECT_EQ(page->evb, &evb);
  Page::destroy(page);
}

// ============================================================
// Group 5: evb_local_ptr same-thread
// ============================================================

TEST(EvbLocalPtr, SameThreadDestruction) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto ptr = allocator.make_local<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(ptr->value, 42);
    ptr.reset();
    EXPECT_EQ(dtorCount.load(), 1);
  });
}

TEST(EvbLocalPtr, NullSafe) {
  evb_local_ptr<int> ptr(nullptr);
  ptr.reset();
}

TEST(EvbLocalPtr, Is8Bytes) {
  EXPECT_EQ(sizeof(evb_local_ptr<int>), sizeof(void*));
}

TEST(EvbLocalPtr, MultipleObjectsSameThread) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};
  constexpr int kCount = 50;

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    {
      std::vector<evb_local_ptr<LifecycleTracker>> ptrs;
      ptrs.reserve(kCount);
      for (int i = 0; i < kCount; ++i) {
        ptrs.push_back(
            allocator.make_local<LifecycleTracker>(ctorCount, dtorCount, i));
      }
    }
    EXPECT_EQ(dtorCount.load(), kCount);
  });
}

// ============================================================
// Group 7: make_local
// ============================================================

TEST(EvbAllocator, MakeLocal) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto ptr = allocator.make_local<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(ptr->value, 42);
  });
  EXPECT_EQ(ctorCount.load(), 1);
  EXPECT_EQ(dtorCount.load(), 1);
}

TEST(EvbAllocator, MakeLocalAlignment) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();

  struct alignas(64) AlignedObj {
    char data[64]{};
  };

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto ptr = allocator.make_local<AlignedObj>();
    EXPECT_EQ(ptr.get(), folly::align_floor(ptr.get(), 64));
  });
}

TEST(EvbAllocator, MakeLocalExceptionSafety) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();

  struct Throws {
    explicit Throws(bool shouldThrow) {
      if (shouldThrow) {
        throw std::runtime_error("boom");
      }
    }
  };

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    EXPECT_THROW(allocator.make_local<Throws>(true), std::runtime_error);
    auto ptr = allocator.make_local<int>(42);
    EXPECT_EQ(*ptr, 42);
  });
}

// ============================================================
// Group 8: Repeated lifecycle
// ============================================================

TEST(RepeatedLifecycle, SingleObjectCreateDestroyLoop) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    for (int i = 0; i < 1000; ++i) {
      auto ptr = allocator.make_local<int>(i);
      EXPECT_EQ(*ptr, i);
    }
  });
}

TEST(RepeatedLifecycle, MultipleObjectsBatch) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    for (int i = 0; i < 100; ++i) {
      evb_local_ptr<int> ptrs[5];
      for (int j = 0; j < 5; ++j) {
        ptrs[j] = allocator.make_local<int>(j);
      }
    }
  });
}

// ============================================================
// Group 9: EventBaseLocal
// ============================================================

TEST(EvbAllocator, GetOrCreate) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  evb->runInEventBaseThreadAndWait([&] {
    auto& a = EvbAllocator::getOrCreate(*evb);
    auto& b = EvbAllocator::getOrCreate(*evb);
    EXPECT_EQ(&a, &b);
  });
}

TEST(EvbAllocator, FreeFunctionEndToEnd) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    auto ptr = evb_make_local<LifecycleTracker>(*evb, ctorCount, dtorCount, 7);
    EXPECT_EQ(ptr->value, 7);
  });
  EXPECT_EQ(dtorCount.load(), 1);
}

// ============================================================
// Group 10: Hoisted allocator pattern — the real usage
// ============================================================

TEST(EvbAllocator, HoistedAllocatorPattern) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  EvbAllocator* alloc = nullptr;
  evb->runInEventBaseThreadAndWait(
      [&] { alloc = &EvbAllocator::getOrCreate(*evb); });
  evb->runInEventBaseThreadAndWait([&] {
    for (int i = 0; i < 100; ++i) {
      auto ptr = alloc->make_local<int>(i);
      EXPECT_EQ(*ptr, i);
    }
  });
}

// ============================================================
// Group 11: evb_shared_ptr
// ============================================================

TEST(EvbSharedPtr, SameThreadDestruction) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto ptr =
        allocator.make_shared<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(ptr->value, 42);
    ptr.reset();
    EXPECT_EQ(dtorCount.load(), 1);
  });
}

TEST(EvbSharedPtr, CrossThreadDestruction) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb_shared_ptr<LifecycleTracker> ptr;
  EvbAllocator* allocator = nullptr;
  evb->runInEventBaseThreadAndWait([&] {
    allocator = new EvbAllocator(evb);
    ptr = allocator->make_shared<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(ptr->value, 42);
  });
  // Destroy off EventBase thread - should schedule back
  ptr.reset();
  // Wait for EventBase to process the scheduled destruction
  evb->runInEventBaseThreadAndWait([] {});
  EXPECT_EQ(dtorCount.load(), 1);
  // Cleanup allocator
  evb->runInEventBaseThreadAndWait([&] { delete allocator; });
}

TEST(EvbSharedPtr, Is16Bytes) {
  // pointer + KeepAlive
  EXPECT_EQ(sizeof(evb_shared_ptr<int>), 16);
}

// ============================================================
// Group 12: Upgrade/Downgrade
// ============================================================

TEST(PointerConversion, UpgradeLocalToShared) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto local =
        allocator.make_local<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(local->value, 42);
    // Upgrade to shared
    auto shared = std::move(local).upgrade();
    EXPECT_EQ(shared->value, 42);
    EXPECT_FALSE(local); // NOLINT(bugprone-use-after-move)
  });
  EXPECT_EQ(dtorCount.load(), 1);
}

TEST(PointerConversion, DowngradeSharedToLocal) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    auto shared =
        allocator.make_shared<LifecycleTracker>(ctorCount, dtorCount, 42);
    EXPECT_EQ(shared->value, 42);
    // Downgrade to local (must be on EventBase thread)
    auto local = std::move(shared).downgrade();
    EXPECT_EQ(local->value, 42);
    EXPECT_FALSE(shared); // NOLINT(bugprone-use-after-move)
  });
  EXPECT_EQ(dtorCount.load(), 1);
}

TEST(PointerConversion, UpgradeAndCrossThreadDestroy) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();
  std::atomic<int> ctorCount{0}, dtorCount{0};

  evb_shared_ptr<LifecycleTracker> shared;
  EvbAllocator* allocator = nullptr;
  evb->runInEventBaseThreadAndWait([&] {
    allocator = new EvbAllocator(evb);
    auto local =
        allocator->make_local<LifecycleTracker>(ctorCount, dtorCount, 42);
    shared = std::move(local).upgrade();
  });
  // Destroy off EventBase thread
  shared.reset();
  evb->runInEventBaseThreadAndWait([] {});
  EXPECT_EQ(dtorCount.load(), 1);
  // Cleanup allocator
  evb->runInEventBaseThreadAndWait([&] { delete allocator; });
}

// ============================================================
// Group 13: Page Reclamation
// ============================================================

TEST(PageReclamation, PagesReclaimedWhenEmpty) {
  folly::ScopedEventBaseThread evbThread;
  auto* evb = evbThread.getEventBase();

  evb->runInEventBaseThreadAndWait([&] {
    EvbAllocator allocator(evb);
    EXPECT_EQ(allocator.pageCount(), 1 + EvbAllocator::kPrewarmedFreePages);

    std::vector<evb_local_ptr<int>> ptrs;
    ptrs.push_back(allocator.make_local<int>(1));

    allocator.activePage()->ptr = allocator.activePage()->end - 8;
    (void)allocator.allocate(64, 8);
    EXPECT_EQ(allocator.pageCount(), 1 + EvbAllocator::kPrewarmedFreePages);

    allocator.reclaimEmptyPages();
    EXPECT_EQ(allocator.pageCount(), 1 + EvbAllocator::kPrewarmedFreePages);

    ptrs.clear();

    allocator.reclaimEmptyPages();

    size_t pageCountBefore = allocator.pageCount();

    allocator.activePage()->ptr = allocator.activePage()->end - 8;
    (void)allocator.allocate(64, 8);

    EXPECT_EQ(allocator.pageCount(), pageCountBefore);
  });
}
