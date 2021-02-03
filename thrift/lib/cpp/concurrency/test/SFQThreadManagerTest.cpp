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

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <thread>

#include <folly/Synchronized.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/SFQThreadManager.h>

using namespace apache::thrift::concurrency;
using testing::_;
using testing::AnyNumber;
using testing::AtLeast;

class SFQThreadManagerTest : public testing::Test {
 public:
  MOCK_METHOD1(bogusTask, void(int));

 protected:
  std::shared_ptr<ThreadManager> newSFQTM(
      std::chrono::seconds perturb,
      size_t numQueues) {
    SFQThreadManagerConfig config;
    config.setPerturbInterval(perturb)
        .setNumFairQueuesForUpstream(numQueues)
        .setExecutors(
            {ThreadManager::newSimpleThreadManager(1),
             ThreadManager::newSimpleThreadManager(1),
             ThreadManager::newSimpleThreadManager(1),
             ThreadManager::newSimpleThreadManager(1),
             ThreadManager::newSimpleThreadManager(1)});
    return std::make_shared<SFQThreadManager>(std::move(config));
  }
};

// Verify tasks are executed at all.
TEST_F(SFQThreadManagerTest, SmokeTest) {
  auto tm = newSFQTM(std::chrono::seconds(1), 1);
  tm->start();
  ThreadManager::ExecutionScope es(PRIORITY::NORMAL);
  es.setTenantId(123);
  auto ka = tm->getKeepAlive(std::move(es), ThreadManager::Source::UPSTREAM);

  EXPECT_CALL(*this, bogusTask(0)).Times(1);
  ka->add([this]() { this->bogusTask(0); });
}

// Ensure the queuing is fair and that higher priority tasks pre-empt low pri.
TEST_F(SFQThreadManagerTest, FairnessPreemptTest) {
  // Disabling perturbation so we can actually test this.
  auto tm = newSFQTM(std::chrono::seconds(0), 10000);
  const auto source = ThreadManager::Source::UPSTREAM;
  tm->start();

  // This will dictate the expected order of placing the tasks.
  std::vector<folly::Baton<>> addOrderBaton(4);

  std::vector<folly::Baton<>> c0Baton(2), c1Baton(2);
  size_t c0{0}, c1{0};

  ThreadManager::ExecutionScope es(PRIORITY::NORMAL);
  es.setTenantId(0);
  tm->getKeepAlive(es, source)->add([&]() {
    addOrderBaton[0].wait();
    ++c0;
    c0Baton[0].post();
  });

  es.setTenantId(0);
  tm->getKeepAlive(es, source)->add([&]() {
    addOrderBaton[1].wait();
    ++c0;
    c0Baton[1].post();
  });

  es.setTenantId(1);
  tm->getKeepAlive(es, source)->add([&]() {
    addOrderBaton[2].wait();
    ++c1;
    c1Baton[0].post();
  });

  es.setTenantId(1);
  tm->getKeepAlive(es, source)->add([&]() {
    addOrderBaton[3].wait();
    ++c1;
    c1Baton[1].post();
  });

  // No tasks have run at this point.
  EXPECT_EQ(0, c0);
  EXPECT_EQ(0, c1);

  // Tenant 0 was added first, so we expect this to execute.
  addOrderBaton[0].post();
  c0Baton[0].wait();
  EXPECT_EQ(1, c0);
  EXPECT_EQ(0, c1);

  // Tenant 1 should be next even though it was added 3rd. Posting the 3rd
  // add-order baton would lock up here if it were unfair.
  addOrderBaton[2].post();
  c1Baton[0].wait();
  EXPECT_EQ(1, c0);
  EXPECT_EQ(1, c1);

  // Tenant 0 will then be up next. It was the task added 2nd.
  addOrderBaton[1].post();
  c0Baton[1].wait();
  EXPECT_EQ(2, c0);
  EXPECT_EQ(1, c1);

  // Tenant 1 would be up next, but let's preempt all this with a higher
  // priority source.
  folly::Baton<> hpribaton, hpribatonOuter;
  es = ThreadManager::ExecutionScope(PRIORITY::HIGH);
  es.setTenantId(123);
  tm->getKeepAlive(es, ThreadManager::Source::INTERNAL)->add([&]() {
    hpribaton.wait();
    hpribatonOuter.post();
  });
  hpribaton.post();
  hpribatonOuter.wait();

  // Now we should be able to execute the tenant 1 task after the
  // source-preempted task.
  addOrderBaton[3].post();
  c1Baton[1].wait();
  EXPECT_EQ(2, c0);
  EXPECT_EQ(2, c1);

  addOrderBaton.clear();
  c0Baton.clear();
  c1Baton.clear();
}
