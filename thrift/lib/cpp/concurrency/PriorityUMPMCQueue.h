/*
 * Copyright 2014-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include <folly/concurrency/UnboundedQueue.h>

namespace apache {
namespace thrift {
namespace concurrency {

/*
 * PriorityUMPMCQueue is a thin wrapper on folly::UMPMCQueue, providing
 * priorities by managing multiple underlying UMPMCQueue instances. As of now,
 * this does not implement a blocking interface. For the purposes of this class,
 * lower priority is more important.
 */
template <typename T>
class PriorityUMPMCQueue {
 public:
  PriorityUMPMCQueue(size_t numPriorities) : queues_(numPriorities) {}

  size_t getNumPriorities() {
    return queues_.size();
  }

  // Add at medium priority by default
  void write(T&& item) {
    writeWithPriority(std::move(item), getNumPriorities() / 2);
  }

  void writeWithPriority(T&& item, size_t priority) {
    size_t index = std::min(getNumPriorities() - 1, priority);
    CHECK_LT(index, queues_.size());
    queues_.at(index).enqueue(std::move(item));
  }

  bool read(T& item) {
    return std::any_of(queues_.begin(), queues_.end(), [&](auto& q) {
      return q.try_dequeue(item);
    });
  }

  size_t size() const {
    size_t total_size = 0;
    for (auto& q : queues_) {
      total_size += q.size();
    }
    return total_size;
  }

  bool isEmpty() const {
    return std::all_of(
        queues_.begin(), queues_.end(), [](auto& q) { return q.empty(); });
  }

 private:
  std::vector<folly::UMPMCQueue<T, /* MayBlock = */ false>> queues_;
};

} // namespace concurrency
} // namespace thrift
} // namespace apache
