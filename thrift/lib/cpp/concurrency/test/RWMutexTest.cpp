/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "thrift/lib/cpp/concurrency/Mutex.h"

#include <gtest/gtest.h>
#include "common/concurrency/Timeout.h"
#include "common/time/TimeConstants.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include <thread>
#include <vector>

using namespace std;
using namespace facebook;
using namespace apache::thrift::concurrency;


const int kTimeoutUsec = 10*kUsecPerMs; // 10ms
const int kTimeoutMs = kTimeoutUsec / kUsecPerMs;
const int kMaxReaders = 10;
const int kMicroSecInMilliSec = 1000;
// user operation time on the lock in milliseconds
const int kOpTimeInMs = 100;

TEST(RWMutexTest, Max_Readers ) {
  ReadWriteMutex l;

  for (int i = 0; i < kMaxReaders; ++i) {
    EXPECT_TRUE(l.timedRead(kTimeoutMs));
  }

  EXPECT_TRUE(l.timedRead(kTimeoutMs));
}

TEST(RWMutexTest, Writer_Wait_Readers ) {
  ReadWriteMutex l;

  for (int i = 0; i < kMaxReaders; ++i) {
    EXPECT_TRUE(l.timedRead(kTimeoutMs));
    EXPECT_FALSE(l.timedWrite(kTimeoutMs));
  }

  for (int i = 0; i < kMaxReaders; ++i) {
    EXPECT_FALSE(l.timedWrite(kTimeoutMs));
    l.release();
  }

  EXPECT_TRUE(l.timedWrite(kTimeoutMs));
  l.release();

  // Testing timeout
  vector<std::thread> threads_;
  for (int i = 0; i < kMaxReaders; ++i) {
    threads_.push_back(std::thread([this, &l] {
      EXPECT_TRUE(l.timedRead(kTimeoutMs));
      usleep(kOpTimeInMs * kMicroSecInMilliSec);
      l.release();
    }));
  }
  // make sure reader lock the lock first
  usleep(1000);

  // wait shorter than the operation time will timeout
  std::thread thread1 = std::thread([this, &l] {
      EXPECT_FALSE(l.timedWrite(0.5 * kOpTimeInMs));
    });

  // wait longer than the operation time will success
  std::thread thread2 = std::thread([this, &l] {
      EXPECT_TRUE(l.timedWrite(1.5 * kOpTimeInMs));
      l.release();
    });

  for (auto& t : threads_) {
    t.join();
  }
  thread1.join();
  thread2.join();
}

TEST(RWMutexTest, Readers_Wait_Writer) {
  ReadWriteMutex l;

  EXPECT_TRUE(l.timedWrite(kTimeoutMs));

  for (int i = 0; i < kMaxReaders; ++i) {
    EXPECT_FALSE(l.timedRead(kTimeoutMs));
  }

  l.release();

  for (int i = 0; i < kMaxReaders; ++i) {
    EXPECT_TRUE(l.timedRead(kTimeoutMs));
  }

  for (int i = 0; i < kMaxReaders; ++i) {
    l.release();
  }

  // Testing Timeout
  std::thread wrThread = std::thread([&l] {
      EXPECT_TRUE(l.timedWrite(kTimeoutMs));
      usleep(kOpTimeInMs * kMicroSecInMilliSec);
      l.release();
    });

  // make sure wrThread lock the lock first
  usleep(1000);

  vector<std::thread> threads_;
  for (int i = 0; i < kMaxReaders; ++i) {
    // wait shorter than the operation time will timeout
    threads_.push_back(std::thread([&l] {
      EXPECT_FALSE(l.timedRead(0.5 * kOpTimeInMs));
    }));

    // wait longer than the operation time will success
    threads_.push_back(std::thread([&l] {
      EXPECT_TRUE(l.timedRead(1.5 * kOpTimeInMs));
      l.release();
    }));
  }

  for (auto& t : threads_) {
    t.join();
  }
  wrThread.join();
}

TEST(RWMutexTest, Writer_Wait_Writer) {
  ReadWriteMutex l;

  EXPECT_TRUE(l.timedWrite(kTimeoutMs));
  EXPECT_FALSE(l.timedWrite(kTimeoutMs));
  l.release();
  EXPECT_TRUE(l.timedWrite(kTimeoutMs));
  EXPECT_FALSE(l.timedWrite(kTimeoutMs));
  l.release();

  // Testing Timeout
  std::thread wrThread1 = std::thread([this, &l] {
      EXPECT_TRUE(l.timedWrite(kTimeoutMs));
      usleep(kOpTimeInMs * kMicroSecInMilliSec);
      l.release();
    });

  // make sure wrThread lock the lock first
  usleep(1000);

  // wait shorter than the operation time will timeout
  std::thread wrThread2 = std::thread([this, &l] {
      EXPECT_FALSE(l.timedWrite(0.5 * kOpTimeInMs));
    });

  // wait longer than the operation time will success
  std::thread wrThread3 = std::thread([this, &l] {
      EXPECT_TRUE(l.timedWrite(1.5 * kOpTimeInMs));
      l.release();
    });

  wrThread1.join();
  wrThread2.join();
  wrThread3.join();
}

TEST(RWMutexTest, Read_Holders) {
  ReadWriteMutex l;

  RWGuard guard(l, false);
  EXPECT_FALSE(l.timedWrite(kTimeoutMs));
  EXPECT_TRUE(l.timedRead(kTimeoutMs));
  l.release();
  EXPECT_FALSE(l.timedWrite(kTimeoutMs));
}

TEST(RWMutexTest, Write_Holders) {
  ReadWriteMutex l;

  RWGuard guard(l, true);
  EXPECT_FALSE(l.timedWrite(kTimeoutMs));
  EXPECT_FALSE(l.timedRead(kTimeoutMs));
}
