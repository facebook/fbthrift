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

#include <atomic>
#include <pthread.h>
#include <list>
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TEventBaseManager.h"
#include <stdio.h>

#include <gtest/gtest.h>

#define THREAD_COUNT 50
#define TRIALS 50
#define TEST_TIME_LIMIT_SEC 10


pthread_mutex_t mutex;
pthread_cond_t startCondition;
std::atomic<int> threadReadyCount;
bool threadShouldRunFlag;

void * thread_main(void *theManager) {
  apache::thrift::async::TEventBaseManager *manager =
    (apache::thrift::async::TEventBaseManager *) theManager;

  pthread_mutex_lock(&mutex);
  ++ (*&threadReadyCount);

  while (! threadShouldRunFlag) {
    pthread_cond_wait(&startCondition, &mutex);
  }

  pthread_mutex_unlock(&mutex);
  // ready to do real testing under simulated somewhat-heavy load
  // (just to exercise code that might result in race conditions)

  // get a new event base for this thread
  apache::thrift::async::TEventBase *eventBase = manager->getEventBase();
  EXPECT_TRUE(eventBase != nullptr);

  // ensure it's the same one if a repeated attempt
  // (this is stashed in this thread's threadlocals)
  EXPECT_EQ(eventBase, manager->getEventBase());

  manager->withEventBaseSet([&] (
   const std::set<apache::thrift::async::TEventBase *> all) {
    EXPECT_GT(all.size(), 0);

    // ensure ours is in the returned collection
    bool foundMine = false;
    for (auto *evb : all) {
      if (evb == eventBase) {
        foundMine = true;
        break;
      }
    }
    EXPECT_TRUE(foundMine);
  });

  // done!
  pthread_exit(nullptr);
  return nullptr;  // doesn't happen but make it look like this returns...
}

void doSingleTest() {
  apache::thrift::async::TEventBaseManager manager;
  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&startCondition, nullptr);
  // threadReadyCount = new std::atomic<int> (0);
  threadShouldRunFlag = false;

  pthread_t threads[THREAD_COUNT];
  time_t startedAtUnixtime = time(nullptr);
  // build threads and start them
  for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_create(&(threads[i]), nullptr, thread_main, &manager);
    EXPECT_TRUE(0 != (long) threads[i]);
  }

  // spin until all are ready
  while (threadReadyCount.load() < THREAD_COUNT) {
    usleep(100000);
    EXPECT_LT(
      time(nullptr),
      startedAtUnixtime + TEST_TIME_LIMIT_SEC);
  }

  // all are ready; pull the trigger and start them all
  threadShouldRunFlag = true;
  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&startCondition);
  pthread_mutex_unlock(&mutex);

  // wait for all
  // XXX need a timeout here and fail if exceeded
  for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_join(threads[i], nullptr);
  }

  // threads all done (XXX should be within some timeout)
  // so we're done
}

TEST(TEventBasePerThreadTest, ThreadsGetAnEventBaseProperly) {
  doSingleTest();
}

TEST(TEventBasePerThreadTest, ThreadsGetAnEventBaseProperlyLooped) {
  for (int i = 0; i < TRIALS; i++) {
    doSingleTest();
  }
}

