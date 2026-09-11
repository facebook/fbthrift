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

namespace apache::thrift::fast_thrift::mem {

EvbAllocator::EvbAllocator(folly::EventBase* evb) : evb_(evb) {
  activePage_ = Page::create(evb_);
  prewarmFreePages();
  evb_->runOnDestructionStart(recyclerCleanup_);
  evb_->runBeforeLoop(&pageRecycler_);
}

EvbAllocator::~EvbAllocator() {
  recyclerCleanup_.cancel();
  pageRecycler_.cancelLoopCallback();

  // Destroy all pages
  auto* page = retiredHead_;
  while (page) {
    auto* next = page->next;
    Page::destroy(page);
    page = next;
  }

  page = freeHead_;
  while (page) {
    auto* next = page->nextFree;
    Page::destroy(page);
    page = next;
  }

  Page::destroy(activePage_);
}

void* EvbAllocator::allocateSlow(size_t bytes, size_t alignment) {
  allocateNewPage();
  auto* result = activePage_->allocate(bytes, alignment);
  DCHECK(result != nullptr);
  return result;
}

void EvbAllocator::prewarmFreePages() {
  for (size_t i = 0; i < kPrewarmedFreePages; ++i) {
    auto* page = Page::create(evb_);
    page->nextFree = freeHead_;
    freeHead_ = page;
  }
}

void EvbAllocator::allocateNewPage() {
  activePage_->next = retiredHead_;
  retiredHead_ = activePage_;
  ++retiredPageCount_;

  // Try to reuse a page from the free list first
  if (freeHead_) {
    activePage_ = freeHead_;
    freeHead_ = freeHead_->nextFree;
    activePage_->reset();
  } else {
    activePage_ = Page::create(evb_);
  }
}

EvbAllocator& EvbAllocator::getOrCreate(folly::EventBase& evb) {
  static folly::EventBaseLocal<EvbAllocator> local;
  return local.try_emplace(evb, &evb);
}

void EvbAllocator::PageRecycler::runLoopCallback() noexcept {
  if (state_ == Active) {
    // First idle detection - just mark idle, don't reclaim yet
    state_ = Idle;
  } else {
    // Second consecutive idle - reclaim empty pages
    allocator_->reclaimEmptyPages();
    state_ = Active; // Reset for next cycle
  }
  // Re-schedule for next loop iteration
  allocator_->evb()->runBeforeLoop(this);
}

void EvbAllocator::reclaimEmptyPages() {
  Page** prevNext = &retiredHead_;
  Page* page = retiredHead_;
  size_t freeCount = 0;

  // Count existing free pages
  for (auto* p = freeHead_; p; p = p->nextFree) {
    ++freeCount;
  }

  while (page) {
    if (page->outstanding == 0) {
      *prevNext = page->next;
      --retiredPageCount_;

      if (freeCount < kMaxFreePages) {
        page->nextFree = freeHead_;
        freeHead_ = page;
        ++freeCount;
      } else {
        Page::destroy(page);
      }
      page = *prevNext;
    } else {
      prevNext = &page->next;
      page = page->next;
    }
  }
}

} // namespace apache::thrift::fast_thrift::mem
