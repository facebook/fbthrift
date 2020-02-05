/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <iostream>

#include <atomic>
#include <memory>

#include <thrift/lib/cpp/concurrency/Mutex.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>

#include <folly/portability/Unistd.h>

#include <folly/portability/GTest.h>

using namespace std;
using namespace apache::thrift::concurrency;

namespace {

class Locker : public Runnable {
 protected:
  Locker(shared_ptr<ReadWriteMutex> rwlock, bool writer)
      : rwlock_(rwlock),
        writer_(writer),
        started_(false),
        gotLock_(false),
        signaled_(false) {}

 public:
  void run() override {
    started_ = true;
    if (writer_) {
      rwlock_->acquireWrite();
    } else {
      rwlock_->acquireRead();
    }
    gotLock_ = true;
    while (!signaled_) {
      usleep(5000);
    }
    rwlock_->release();
  }

  bool started() const {
    return started_;
  }
  bool gotLock() const {
    return gotLock_;
  }
  void signal() {
    signaled_ = true;
  }

 protected:
  shared_ptr<ReadWriteMutex> rwlock_;
  bool writer_;
  atomic<bool> started_;
  atomic<bool> gotLock_;
  atomic<bool> signaled_;
};

class Reader : public Locker {
 public:
  explicit Reader(shared_ptr<ReadWriteMutex> rwlock) : Locker(rwlock, false) {}
};

class Writer : public Locker {
 public:
  explicit Writer(shared_ptr<ReadWriteMutex> rwlock) : Locker(rwlock, true) {}
};

class RWMutexStarveTest
    : public testing::TestWithParam<PosixThreadFactory::POLICY> {};

} // namespace

TEST_P(RWMutexStarveTest, test_starve) {
  // the man pages for pthread_wrlock_rdlock suggest that any OS guarantee about
  // writer starvation may be influenced by the scheduling policy, so let's try
  // all 3 policies to see if any of them work.
  PosixThreadFactory factory(GetParam());
  factory.setDetached(false);

  auto rwlock = make_shared<NoStarveReadWriteMutex>();

  auto reader1 = make_shared<Reader>(rwlock);
  auto reader2 = make_shared<Reader>(rwlock);
  auto writer = make_shared<Writer>(rwlock);

  auto treader1 = factory.newThread(reader1);
  auto treader2 = factory.newThread(reader2);
  auto twriter = factory.newThread(writer);

  // launch a reader and make sure he has the lock
  treader1->start();
  while (!reader1->gotLock()) {
    usleep(2000);
  }

  // launch a writer and make sure he's blocked on the lock
  twriter->start();
  while (!writer->started()) {
    usleep(2000);
  }
  // tricky part... we can never be 100% sure that the writer is actually
  // blocked on the lock, but we can pretty reasonably sure because we know
  // he just executed the line immediately before getting the lock, and
  // we'll wait a full second for him to get on it.
  sleep(1);

  // launch a second reader... if the RWMutex guarantees that writers won't
  // starve, this reader should not be able to acquire the lock until the writer
  // has acquired and released it.
  treader2->start();
  while (!reader2->started()) {
    usleep(2000);
  }
  // again... can't be 100% sure the reader is waiting on (or has) the lock
  // but we can be close.
  sleep(1);

  // tell reader 1 to let go of the lock
  reader1->signal();

  // wait for someone to get the lock
  while (!reader2->gotLock() && !writer->gotLock()) {
    usleep(2000);
  }

  // the test succeeded if the WRITER got the lock.
  bool success = writer->gotLock();

  // tell everyone we're done and wait for them to finish
  reader2->signal();
  writer->signal();
  treader1->join();
  treader2->join();
  twriter->join();

  // make sure it worked.
  EXPECT_TRUE(success) << "writer is starving";
}

INSTANTIATE_TEST_CASE_P(
    RWMutexStarveTest,
    RWMutexStarveTest,
    testing::Values(
        PosixThreadFactory::POLICY::OTHER,
        PosixThreadFactory::POLICY::FIFO,
        PosixThreadFactory::POLICY::ROUND_ROBIN));
