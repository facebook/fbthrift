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

#include <cstdlib>
#include <memory>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/io/async/ScopedEventBaseThread.h>

using namespace apache::thrift::fast_thrift::mem;

struct TinyObj {
  int val{0};
  ~TinyObj() { folly::doNotOptimizeAway(val); }
};

static size_t tinyObjSize() {
  auto size = sizeof(TinyObj);
  folly::makeUnpredictable(size);
  return size;
}

static void useTinyObj(TinyObj* obj, unsigned int val) {
  obj->val = val;
  folly::doNotOptimizeAway(obj);
  folly::doNotOptimizeAway(obj->val);
}

static void consumeActivePage(EvbAllocator& alloc) {
  auto* page = alloc.activePage();
  page->ptr = page->end - 8;
}

// Shared EventBase for all benchmarks to avoid setup overhead
static folly::ScopedEventBaseThread* getSharedEvbThread() {
  static folly::ScopedEventBaseThread evbThread;
  return &evbThread;
}

static folly::EventBase* getSharedEvb() {
  return getSharedEvbThread()->getEventBase();
}

static EvbAllocator* getSharedAlloc() {
  static EvbAllocator* alloc = nullptr;
  if (!alloc) {
    getSharedEvb()->runInEventBaseThreadAndWait(
        [&] { alloc = &EvbAllocator::getOrCreate(*getSharedEvb()); });
  }
  return alloc;
}

// ============================================================
// Isolate each cost component.
// All benchmarks run on the EB thread with hoisted allocator.
// ============================================================

// A. Raw bump alloc only — no header, no construct, no destruct
BENCHMARK(A_RawBumpOnly, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      auto* p = alloc->allocate(sizeof(TinyObj), alignof(TinyObj));
      folly::doNotOptimizeAway(p);
    }
  });
}

// B. Bump alloc + placement new + dtor + page recovery (no unique_ptr)
BENCHMARK_RELATIVE(B_RawBumpConstructDestruct, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      auto* mem = alloc->allocate(sizeof(TinyObj), alignof(TinyObj));
      auto* obj = ::new (mem) TinyObj();
      auto* page = Page::fromObject(mem);
      ++page->outstanding;
      useTinyObj(obj, i);
      obj->~TinyObj();
      --page->outstanding;
    }
  });
}

// C. Full evb_local_ptr lifecycle (make_local + use + destroy)
BENCHMARK_RELATIVE(C_EvbLocalPtr, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      auto ptr = alloc->make_local<TinyObj>();
      useTinyObj(ptr.get(), i);
    }
  });
}

// D. std::unique_ptr lifecycle (make_unique + use + destroy) on EB thread
BENCHMARK_RELATIVE(D_StdUniquePtr, n) {
  getSharedEvb()->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      auto ptr = std::make_unique<TinyObj>();
      useTinyObj(ptr.get(), i);
    }
  });
}

// E. malloc + placement new + dtor + free (what std::unique_ptr does)
BENCHMARK_RELATIVE(E_MallocConstructDestructFree, n) {
  getSharedEvb()->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      auto* mem = std::malloc(tinyObjSize());
      folly::doNotOptimizeAway(mem);
      auto* obj = ::new (mem) TinyObj();
      useTinyObj(obj, i);
      obj->~TinyObj();
      folly::doNotOptimizeAway(mem);
      std::free(mem);
    }
  });
}

BENCHMARK_DRAW_LINE();

BENCHMARK(P_PageSpillPrewarmed, n) {
  for (unsigned int i = 0; i < n; ++i) {
    std::unique_ptr<folly::EventBase> evb;
    std::unique_ptr<EvbAllocator> alloc;

    BENCHMARK_SUSPEND {
      evb = std::make_unique<folly::EventBase>();
      alloc = std::make_unique<EvbAllocator>(evb.get());
      consumeActivePage(*alloc);
    }

    auto* p = alloc->allocate(8, alignof(std::max_align_t));
    folly::doNotOptimizeAway(p);

    BENCHMARK_SUSPEND {
      alloc.reset();
      evb.reset();
    }
  }
}

BENCHMARK_RELATIVE(Q_PageSpillReused, n) {
  for (unsigned int i = 0; i < n; ++i) {
    std::unique_ptr<folly::EventBase> evb;
    std::unique_ptr<EvbAllocator> alloc;

    BENCHMARK_SUSPEND {
      evb = std::make_unique<folly::EventBase>();
      alloc = std::make_unique<EvbAllocator>(evb.get());
      auto ptr = alloc->make_local<int>(7);
      consumeActivePage(*alloc);
      auto spill = alloc->allocate(8, alignof(std::max_align_t));
      folly::doNotOptimizeAway(spill);
      ptr.reset();
      alloc->reclaimEmptyPages();
      consumeActivePage(*alloc);
    }

    auto* p = alloc->allocate(8, alignof(std::max_align_t));
    folly::doNotOptimizeAway(p);

    BENCHMARK_SUSPEND {
      alloc.reset();
      evb.reset();
    }
  }
}

BENCHMARK_DRAW_LINE();

// G-K: Same breakdown but with 10-object batches
BENCHMARK(G_EvbLocalPtr_10obj, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      evb_local_ptr<TinyObj> ptrs[10];
      for (int j = 0; j < 10; ++j) {
        ptrs[j] = alloc->make_local<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

BENCHMARK_RELATIVE(H_StdUniquePtr_10obj, n) {
  getSharedEvb()->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      std::unique_ptr<TinyObj> ptrs[10];
      for (int j = 0; j < 10; ++j) {
        ptrs[j] = std::make_unique<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

// I. Raw 10 alloc+construct+destruct WITHOUT unique_ptr wrapper
BENCHMARK_RELATIVE(I_RawBump_10obj_NoWrapper, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      TinyObj* ptrs[10];
      for (int j = 0; j < 10; ++j) {
        auto* mem = alloc->allocate(sizeof(TinyObj), alignof(TinyObj));
        ptrs[j] = ::new (mem) TinyObj();
        ++Page::fromObject(ptrs[j])->outstanding;
        useTinyObj(ptrs[j], j);
      }
      for (int j = 9; j >= 0; --j) {
        ptrs[j]->~TinyObj();
        auto* page = Page::fromObject(ptrs[j]);
        --page->outstanding;
      }
    }
  });
}

BENCHMARK_DRAW_LINE();

// ============================================================
// K-P: 50 and 100 object batches.
// Allocator created once outside the timed loop. The fixed page
// holds enough TinyObj allocations to avoid spill during
// measurement. We're measuring pure alloc+construct+destroy.
// ============================================================

BENCHMARK(K_EvbLocalPtr_50obj, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      evb_local_ptr<TinyObj> ptrs[50];
      for (int j = 0; j < 50; ++j) {
        ptrs[j] = alloc->make_local<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

BENCHMARK_RELATIVE(L_StdUniquePtr_50obj, n) {
  getSharedEvb()->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      std::unique_ptr<TinyObj> ptrs[50];
      for (int j = 0; j < 50; ++j) {
        ptrs[j] = std::make_unique<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

BENCHMARK_DRAW_LINE();

BENCHMARK(N_EvbLocalPtr_100obj, n) {
  auto* evb = getSharedEvb();
  auto* alloc = getSharedAlloc();
  evb->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      evb_local_ptr<TinyObj> ptrs[100];
      for (int j = 0; j < 100; ++j) {
        ptrs[j] = alloc->make_local<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

BENCHMARK_RELATIVE(O_StdUniquePtr_100obj, n) {
  getSharedEvb()->runInEventBaseThreadAndWait([&] {
    for (unsigned int i = 0; i < n; ++i) {
      std::unique_ptr<TinyObj> ptrs[100];
      for (int j = 0; j < 100; ++j) {
        ptrs[j] = std::make_unique<TinyObj>();
        useTinyObj(ptrs[j].get(), j);
      }
    }
  });
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
