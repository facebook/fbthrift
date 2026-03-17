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

#include <atomic>
#include <cstdint>
#include <optional>

#include <folly/concurrency/UnboundedQueue.h>

namespace apache::thrift::server {

// A simple MPMC queue with optional count-based limiting.
// When limit is 0, the queue is unbounded and no atomic counter overhead
// is incurred. When limit > 0, an atomic counter enforces the maximum
// number of elements in the queue.
template <typename T>
class SingleBucketRequestPileQueue {
 public:
  // limit=0 means unlimited (no atomic counter overhead).
  // limit>0 means enqueue will reject when count reaches the limit.
  explicit SingleBucketRequestPileQueue(uint64_t limit = 0) : limit_(limit) {}

  // Enqueue an element. Returns true on success, false if at limit.
  bool enqueue(T&& elem) {
    if (limit_ != 0) {
      auto prev = count_.fetch_add(1, std::memory_order_relaxed);
      if (prev >= limit_) {
        count_.fetch_sub(1, std::memory_order_relaxed);
        return false;
      }
    }
    queue_.enqueue(std::move(elem));
    return true;
  }

  // Non-blocking try-dequeue. Returns nullopt if empty.
  std::optional<T> tryDequeue() noexcept {
    if (auto res = queue_.try_dequeue()) {
      if (limit_ != 0) {
        count_.fetch_sub(1, std::memory_order_relaxed);
      }
      return std::move(*res);
    }
    return std::nullopt;
  }

  // Returns an estimate of the number of elements in the queue.
  size_t size() const { return queue_.size(); }

 private:
  // 1KB segment size to minimize allocations, matching RoundRobinRequestPile's
  // existing configuration.
  using Queue = folly::UMPMCQueue<
      T,
      /* MayBlock */ false,
      /* log2(SegmentSize=1024) */ 10>;

  Queue queue_;
  uint64_t limit_{0};
  std::atomic<uint64_t> count_{0};
};

} // namespace apache::thrift::server
