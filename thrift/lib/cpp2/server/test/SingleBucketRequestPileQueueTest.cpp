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

#include <gtest/gtest.h>
#include <folly/ScopeGuard.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <thrift/lib/cpp2/server/SingleBucketRequestPileQueue.h>

using namespace apache::thrift::server;

TEST(SingleBucketRequestPileQueueTest, UnlimitedOrdering) {
  SingleBucketRequestPileQueue<int> queue;

  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(queue.enqueue(int(i)));
  }

  EXPECT_EQ(queue.size(), 10);

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(*queue.tryDequeue(), i);
  }

  EXPECT_EQ(queue.tryDequeue(), std::nullopt);
  EXPECT_EQ(queue.size(), 0);
}

TEST(SingleBucketRequestPileQueueTest, LimitRejectsAtCapacity) {
  SingleBucketRequestPileQueue<int> queue(5);

  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(queue.enqueue(int(i)));
  }
  EXPECT_EQ(queue.size(), 5);

  // At capacity — should reject
  EXPECT_FALSE(queue.enqueue(int(5)));
  EXPECT_FALSE(queue.enqueue(int(6)));
  EXPECT_EQ(queue.size(), 5);
}

TEST(SingleBucketRequestPileQueueTest, DequeueReenablesEnqueue) {
  SingleBucketRequestPileQueue<int> queue(3);

  EXPECT_TRUE(queue.enqueue(int(1)));
  EXPECT_TRUE(queue.enqueue(int(2)));
  EXPECT_TRUE(queue.enqueue(int(3)));
  EXPECT_FALSE(queue.enqueue(int(4)));

  // Dequeue one — should allow one more enqueue
  EXPECT_EQ(*queue.tryDequeue(), 1);
  EXPECT_TRUE(queue.enqueue(int(4)));
  EXPECT_FALSE(queue.enqueue(int(5)));

  // Drain the rest
  EXPECT_EQ(*queue.tryDequeue(), 2);
  EXPECT_EQ(*queue.tryDequeue(), 3);
  EXPECT_EQ(*queue.tryDequeue(), 4);
  EXPECT_EQ(queue.tryDequeue(), std::nullopt);
}

TEST(SingleBucketRequestPileQueueTest, ZeroLimitIsUnlimited) {
  SingleBucketRequestPileQueue<int> queue(0);

  // Should accept many elements without rejection
  for (int i = 0; i < 10000; ++i) {
    EXPECT_TRUE(queue.enqueue(int(i)));
  }
  EXPECT_EQ(queue.size(), 10000);
}

TEST(SingleBucketRequestPileQueueTest, ConcurrentEnqueueDequeue) {
  constexpr uint64_t kLimit = 1000;
  SingleBucketRequestPileQueue<int> queue(kLimit);

  std::atomic<uint64_t> enqueued{0};
  std::atomic<uint64_t> rejected{0};
  std::atomic<uint64_t> dequeued{0};

  {
    folly::CPUThreadPoolExecutor ex(32);

    // Producers
    for (int i = 0; i < 16; ++i) {
      ex.add([&] {
        for (int j = 0; j < 1000; ++j) {
          if (queue.enqueue(int(j))) {
            enqueued.fetch_add(1, std::memory_order_relaxed);
          } else {
            rejected.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }

    // Consumers
    for (int i = 0; i < 16; ++i) {
      ex.add([&] {
        for (int j = 0; j < 1000; ++j) {
          if (queue.tryDequeue()) {
            dequeued.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }
  }
  // Executor destroyed — all threads joined

  // Drain remaining
  while (queue.tryDequeue()) {
    dequeued.fetch_add(1, std::memory_order_relaxed);
  }

  // Every successfully enqueued item must be dequeued
  EXPECT_EQ(enqueued.load(), dequeued.load());
  // Total attempts = enqueued + rejected
  EXPECT_EQ(enqueued.load() + rejected.load(), 16 * 1000);
}

TEST(SingleBucketRequestPileQueueTest, ConcurrentUnlimited) {
  SingleBucketRequestPileQueue<int> queue;
  constexpr int kPerThread = 10000;
  constexpr int kThreads = 8;

  {
    folly::CPUThreadPoolExecutor ex(kThreads);
    for (int i = 0; i < kThreads; ++i) {
      ex.add([&] {
        for (int j = 0; j < kPerThread; ++j) {
          queue.enqueue(int(j));
        }
      });
    }
  }

  EXPECT_EQ(queue.size(), kPerThread * kThreads);

  uint64_t count = 0;
  while (queue.tryDequeue()) {
    ++count;
  }
  EXPECT_EQ(count, kPerThread * kThreads);
}
