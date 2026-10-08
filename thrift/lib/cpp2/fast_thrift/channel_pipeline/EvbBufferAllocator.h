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

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory_resource>
#include <new>

#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseLocal.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/common/allocator/EvbAllocator.h>

namespace apache::thrift::fast_thrift::channel_pipeline {

namespace detail {

class EvbMemoryResource final : public std::pmr::memory_resource {
 public:
  static EvbMemoryResource& getOrCreate(folly::EventBase& eventBase) {
    static folly::EventBaseLocal<EvbMemoryResource> resources;
    return resources.try_emplace(eventBase, &eventBase);
  }

  explicit EvbMemoryResource(folly::EventBase* eventBase)
      : eventBase_(eventBase),
        allocator_(&mem::EvbAllocator::getOrCreate(*eventBase)) {}

  void* allocateObject(size_t bytes, size_t alignment) {
    if (bytes > mem::EvbAllocator::kMaxObjectSize ||
        alignment > mem::EvbAllocator::kMaxObjectAlignment) {
      throw std::bad_alloc{};
    }
    DCHECK(eventBase_->isInEventBaseThread());
    void* const result =
        allocator_->allocate(std::max<size_t>(bytes, 1), alignment);
    ++mem::Page::fromObject(result)->outstanding;
    return result;
  }

  void releaseObject(void* object) noexcept {
    DCHECK(eventBase_->isInEventBaseThread());
    auto* const page = mem::Page::fromObject(object);
    DCHECK_GT(page->outstanding, 0);
    --page->outstanding;
  }

 private:
  void* do_allocate(size_t bytes, size_t alignment) override {
    return allocateObject(bytes, alignment);
  }

  void do_deallocate(void* object, size_t, size_t) noexcept override {
    releaseObject(object);
  }

  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  folly::EventBase* eventBase_;
  mem::EvbAllocator* allocator_;
};

} // namespace detail

class EvbBufferAllocator final {
 public:
  explicit EvbBufferAllocator(folly::EventBase& eventBase)
      : resource_(&detail::EvbMemoryResource::getOrCreate(eventBase)) {}

  BytesPtr allocate(size_t size) noexcept {
    if (size > mem::EvbAllocator::kMaxObjectSize) [[unlikely]] {
      try {
        return folly::IOBuf::create(size);
      } catch (...) {
        return nullptr;
      }
    }

    try {
      void* const data =
          resource_->allocateObject(size, alignof(std::max_align_t));
      return takeOwnership(
          OwnedBuffer{
              .data = data,
              .capacity = size,
              .freeFn = &releaseObject,
              .userData = resource_,
          });
    } catch (...) {
      return nullptr;
    }
  }

  BytesPtr copyBuffer(const void* data, size_t size) noexcept {
    BytesPtr result = allocate(size);
    if (result == nullptr) [[unlikely]] {
      return nullptr;
    }
    if (size != 0) {
      std::memcpy(result->writableData(), data, size);
      result->append(size);
    }
    return result;
  }

  BytesPtr takeOwnership(OwnedBuffer buffer) noexcept {
    try {
      return folly::IOBuf::takeOwnership(
          resource_,
          buffer.data,
          buffer.capacity,
          buffer.offset,
          buffer.length,
          buffer.freeFn,
          buffer.userData,
          true);
    } catch (...) {
      return nullptr;
    }
  }

 private:
  static void releaseObject(void* object, void* resource) noexcept {
    static_cast<detail::EvbMemoryResource*>(resource)->releaseObject(object);
  }

  detail::EvbMemoryResource* resource_;
};

static_assert(BufferAllocator<EvbBufferAllocator>);
static_assert(ExternalBufferAllocator<EvbBufferAllocator>);

} // namespace apache::thrift::fast_thrift::channel_pipeline
