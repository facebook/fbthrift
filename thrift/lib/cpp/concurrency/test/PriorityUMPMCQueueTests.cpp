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

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/concurrency/PriorityUMPMCQueue.h>

using namespace apache::thrift::concurrency;

TEST(PriorityUMPMCQueueTests, BasicOps) {
  // With just one priority, this should behave like a normal UMPMCQueue
  PriorityUMPMCQueue<size_t> queue(1);
  EXPECT_TRUE(queue.isEmpty());
  EXPECT_EQ(1, queue.getNumPriorities());

  queue.write(9);
  queue.write(8);

  EXPECT_FALSE(queue.isEmpty());
  EXPECT_EQ(2, queue.size());

  size_t item;
  queue.read(item);
  EXPECT_EQ(9, item);
  EXPECT_FALSE(queue.isEmpty());
  EXPECT_EQ(1, queue.size());

  queue.read(item);
  EXPECT_EQ(8, item);
  EXPECT_TRUE(queue.isEmpty());
  EXPECT_EQ(0, queue.size());
}

TEST(PriorityUMPMCQueueTests, TestPriorities) {
  PriorityUMPMCQueue<size_t> queue(3);
  EXPECT_TRUE(queue.isEmpty());
  EXPECT_EQ(3, queue.getNumPriorities());

  // This should go to the lowpri queue, as we only
  // have 3 priorities
  queue.writeWithPriority(5, 50);
  // unqualified writes should be mid-pri
  queue.write(3);
  queue.writeWithPriority(6, 2);
  queue.writeWithPriority(1, 0);
  queue.write(4);
  queue.writeWithPriority(2, 0);

  EXPECT_FALSE(queue.isEmpty());
  EXPECT_EQ(6, queue.size());

  size_t item;
  for (int i = 1; i <= 6; i++) {
    queue.read(item);
    EXPECT_EQ(i, item);
    EXPECT_EQ(6 - i, queue.size());
  }
}
