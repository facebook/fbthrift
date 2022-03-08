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

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/portability/GTest.h>
#include <folly/synchronization/Baton.h>
#include <folly/synchronization/Latch.h>
#include <thrift/lib/cpp2/server/ParallelConcurrencyController.h>
#include <thrift/lib/cpp2/server/RoundRobinRequestPile.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

using Func = std::function<void(
    ServerRequest&&, const AsyncProcessorFactory::MethodMetadata&)>;

class MockAsyncProcessor : public AsyncProcessor {
 public:
  void processSerializedRequest(
      ResponseChannelRequest::UniquePtr,
      SerializedRequest&&,
      protocol::PROTOCOL_TYPES,
      Cpp2RequestContext*,
      folly::EventBase*,
      concurrency::ThreadManager*) override {}

  void executeRequest(
      ServerRequest&& request,
      const AsyncProcessorFactory::MethodMetadata& methodMetadata) override {
    if (executeRequestFunc_) {
      executeRequestFunc_(std::move(request), methodMetadata);
    } else {
      LOG(FATAL) << "Unimplemented executeRequest called";
    }
  }

  void setFunc(Func func) { executeRequestFunc_ = std::move(func); }

 private:
  Func executeRequestFunc_;
};

class FIFORequestPile : public RequestPileInterface {
 public:
  std::optional<ServerRequestRejection> enqueue(
      ServerRequest&& request) override {
    queue_.enqueue(std::move(request));
    return std::nullopt;
  }

  std::pair<std::optional<ServerRequest>, std::optional<intptr_t>> dequeue()
      override {
    if (auto res = queue_.try_dequeue()) {
      return std::make_pair(std::move(*res), std::nullopt);
    } else {
      return std::make_pair(std::nullopt, std::nullopt);
    }
  }

  void onRequestFinished(intptr_t /* userData */) override {}

  uint64_t requestCount() const override { return queue_.size(); }

 private:
  using Queue = folly::UMPMCQueue<
      ServerRequest,
      /* MayBlock */ false,
      /* LgSegmentSize  */ 5>;

  Queue queue_;
};

class ResourcePool {
 public:
  ResourcePool(FIFORequestPile* pile, ParallelConcurrencyController* controller)
      : pile_(pile), controller_(controller) {}

  void enqueue(ServerRequest&& request) {
    pile_->enqueue(std::move(request));
    controller_->onEnqueued();
  }

 private:
  FIFORequestPile* pile_;
  ParallelConcurrencyController* controller_;
};

class MockResponseChannelRequest : public ResponseChannelRequest {
 public:
  bool isActive() const override { return true; }

  bool isOneway() const override { return false; }

  bool includeEnvelope() const override { return false; }

  void sendReply(
      ResponsePayload&&,
      MessageChannel::SendCallback*,
      folly::Optional<uint32_t>) override {}

  void sendErrorWrapped(folly::exception_wrapper, std::string) override {}

  bool tryStartProcessing() override { return true; }
};

namespace {

ServerRequest getRequest(AsyncProcessor* ap, folly::EventBase* eb) {
  static ServiceRequestInfo requestInfo{
      false, RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, "function", std::nullopt};
  ServerRequest req(
      ResponseChannelRequest::UniquePtr(new MockResponseChannelRequest),
      SerializedCompressedRequest(std::unique_ptr<folly::IOBuf>{}),
      eb,
      nullptr, /* ctx  */
      static_cast<protocol::PROTOCOL_TYPES>(0),
      nullptr, /* requestContext  */
      ap,
      nullptr, /* methodMetadata  */
      &requestInfo);
  return req;
}

Func blockingTaskGen(
    folly::Baton<>& baton, ParallelConcurrencyController& controller) {
  Func waitingTask = [&](ServerRequest&& /* request */,
                         const AsyncProcessorFactory::MethodMetadata&) {
    baton.wait();
    controller.onRequestFinished(0);
  };

  return waitingTask;
}

Func endingTaskGen(
    folly::Baton<>& baton, ParallelConcurrencyController& controller) {
  Func waitingTask = [&](ServerRequest&& /* request */,
                         const AsyncProcessorFactory::MethodMetadata&) {
    controller.onRequestFinished(0);
    baton.post();
  };

  return waitingTask;
}

std::unique_ptr<AsyncProcessor> makeAP(Func func) {
  auto mockAP = std::make_unique<MockAsyncProcessor>();
  mockAP->setFunc(func);
  std::unique_ptr<AsyncProcessor> endingAP(std::move(mockAP));
  return endingAP;
}

} // namespace

// This test just tests the normal case where we have 2 tasks that sit in
// the Executor, the count should return 2.
// When the tasks all finish, the count should return 0
TEST(ParallelConcurrencyControllerTest, NormalCases) {
  folly::EventBase eb;

  folly::CPUThreadPoolExecutor ex(1);
  FIFORequestPile pile;
  ParallelConcurrencyController controller(pile, ex);

  folly::Baton baton1;
  folly::Baton baton2;

  auto blockingAP = makeAP(blockingTaskGen(baton1, controller));

  auto endingAP = makeAP(endingTaskGen(baton2, controller));

  ResourcePool pool(&pile, &controller);

  pool.enqueue(getRequest(blockingAP.get(), &eb));
  pool.enqueue(getRequest(endingAP.get(), &eb));

  EXPECT_EQ(controller.requestCount(), 2);
  baton1.post();

  baton2.wait();
  EXPECT_EQ(controller.requestCount(), 0);
}

// This tests when the concurrency limit is set to 2
// In this case only 2 tasks can run concurrently
TEST(ParallelConcurrencyControllerTest, LimitedTasks) {
  folly::EventBase eb;

  folly::CPUThreadPoolExecutor ex(1);
  FIFORequestPile pile;
  ParallelConcurrencyController controller(pile, ex);

  controller.setExecutionLimitRequests(2);

  folly::Baton baton1;
  folly::Baton baton2;
  folly::Baton baton3;

  auto staringBlockingAP = makeAP(blockingTaskGen(baton1, controller));

  auto blockingAP = makeAP(blockingTaskGen(baton2, controller));

  auto endingAP = makeAP(endingTaskGen(baton3, controller));

  ResourcePool pool(&pile, &controller);

  pool.enqueue(getRequest(blockingAP.get(), &eb));
  pool.enqueue(getRequest(blockingAP.get(), &eb));
  pool.enqueue(getRequest(endingAP.get(), &eb));

  EXPECT_EQ(controller.requestCount(), 2);

  baton1.post();
  baton2.post();

  baton3.wait();
  EXPECT_EQ(controller.requestCount(), 0);
}

Func edgeTaskGen(
    folly::Latch& latch,
    folly::Baton<>& baton,
    ParallelConcurrencyController& controller) {
  Func waitingTask = [&](ServerRequest&& /* request */,
                         const AsyncProcessorFactory::MethodMetadata&) {
    baton.wait();
    controller.onRequestFinished(0);

    latch.count_down();
  };

  return waitingTask;
}

std::unique_ptr<AsyncProcessor> getEdgeTaskAP(
    folly::Latch& latch,
    folly::Baton<>& baton,
    ParallelConcurrencyController& controller) {
  auto mockAP = new MockAsyncProcessor;
  mockAP->setFunc(edgeTaskGen(latch, baton, controller));
  std::unique_ptr<AsyncProcessor> endingAP(mockAP);
  return endingAP;
}

TEST(ParallelConcurrencyControllerTest, DifferentOrdering1) {
  folly::EventBase eb;

  folly::CPUThreadPoolExecutor ex(2);
  FIFORequestPile pile;
  ParallelConcurrencyController controller(pile, ex);
  controller.setExecutionLimitRequests(2);

  ResourcePool pool(&pile, &controller);

  folly::Latch latch(3);

  folly::Baton baton1;
  folly::Baton baton2;
  folly::Baton baton3;

  auto mockAP1 = getEdgeTaskAP(latch, baton1, controller);
  auto mockAP2 = getEdgeTaskAP(latch, baton2, controller);
  auto mockAP3 = getEdgeTaskAP(latch, baton3, controller);

  // one scenario is right after one task finishes
  // we push another task into the queue
  pool.enqueue(getRequest(mockAP1.get(), &eb));
  pool.enqueue(getRequest(mockAP2.get(), &eb));

  EXPECT_EQ(controller.requestCount(), 2);

  // one task will finish immediately and another task is pushed
  // This shouldn't be causing any idle thread
  baton1.post();
  pool.enqueue(getRequest(mockAP3.get(), &eb));

  baton2.post();
  baton3.post();

  latch.wait();
  EXPECT_EQ(controller.requestCount(), 0);
  EXPECT_EQ(pile.requestCount(), 0);
}

TEST(ParallelConcurrencyControllerTest, DifferentOrdering2) {
  folly::EventBase eb;

  folly::CPUThreadPoolExecutor ex(2);
  FIFORequestPile pile;
  ParallelConcurrencyController controller(pile, ex);
  controller.setExecutionLimitRequests(2);

  ResourcePool pool(&pile, &controller);

  folly::Latch latch(3);

  folly::Baton baton1;
  folly::Baton baton2;
  folly::Baton baton3;

  auto mockAP1 = getEdgeTaskAP(latch, baton1, controller);
  auto mockAP2 = getEdgeTaskAP(latch, baton2, controller);
  auto mockAP3 = getEdgeTaskAP(latch, baton3, controller);

  // another scenario is right before one task finishes
  // we push another task into the queue
  pool.enqueue(getRequest(mockAP1.get(), &eb));
  pool.enqueue(getRequest(mockAP2.get(), &eb));

  EXPECT_EQ(controller.requestCount(), 2);

  // one task will finish immediately and another task is pushed
  pool.enqueue(getRequest(mockAP3.get(), &eb));
  baton1.post();
  baton2.post();
  baton3.post();

  latch.wait();
  EXPECT_EQ(controller.requestCount(), 0);
  EXPECT_EQ(pile.requestCount(), 0);
}

// This test verifies that tasks added through the underlying executor
// of the ConcurrencyController will get higher priority than the task
// added through ConcurrencyController
// This is mimicing the scenario where one request may spawn other tasks
// in their handler code and we should execute those first
TEST(ParallelConcurrencyControllerTest, ExistingTasksHaveHigherPriority) {
  folly::EventBase eb;

  std::atomic<size_t> counter{0};

  Func task1 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 1);
  };
  Func task2 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 3);
  };
  Func task3 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 4);
  };
  auto task4 = [&] {
    ++counter;
    EXPECT_EQ(counter, 2);
  };

  Func task5 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 5);
  };
  Func task6 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 6);
  };
  Func task7 = [&](ServerRequest&& /* request */,
                   const AsyncProcessorFactory::MethodMetadata&) {
    ++counter;
    EXPECT_EQ(counter, 7);
  };

  auto ap1 = std::make_unique<MockAsyncProcessor>();
  ap1->setFunc(task1);
  auto ap2 = std::make_unique<MockAsyncProcessor>();
  ap2->setFunc(task2);
  auto ap3 = std::make_unique<MockAsyncProcessor>();
  ap3->setFunc(task3);

  auto ap5 = std::make_unique<MockAsyncProcessor>();
  ap5->setFunc(task5);
  auto ap6 = std::make_unique<MockAsyncProcessor>();
  ap6->setFunc(task6);
  auto ap7 = std::make_unique<MockAsyncProcessor>();
  ap7->setFunc(task7);

  folly::Baton baton;

  folly::CPUThreadPoolExecutor ex(1);
  FIFORequestPile pile;
  ParallelConcurrencyController controller(pile, ex);

  ResourcePool pool(&pile, &controller);

  ex.add([&] { baton.wait(); });

  // This block is verifying that when a task is added through
  // underlying executor, it will get higher priority
  {
    // task1 is pushed by ConcurrencyController
    // and gets blocked
    // This is executed first so counter == 1
    pool.enqueue(getRequest(ap1.get(), &eb));
    // task2 is pushed
    // This is executed last so counter == 3
    pool.enqueue(getRequest(ap2.get(), &eb));
    // task2 is pushed
    // This is executed last so counter == 4
    pool.enqueue(getRequest(ap3.get(), &eb));
    // task3 is pushed through executor which should
    // get higher priority than task2
    // This is executed before task2 so counter == 2
    ex.add(task4);
  }

  // This block is verifying the normal behavior that
  // tasks added through ConcurrencyController are
  // executed in order
  {
    // task5 is pushed by ConcurrencyController
    // and gets blocked
    // This is executed first so counter == 5
    pool.enqueue(getRequest(ap5.get(), &eb));
    // task6 is pushed
    // This is executed last so counter == 6
    pool.enqueue(getRequest(ap6.get(), &eb));
    // task7 is pushed
    // This is executed last so counter == 6
    pool.enqueue(getRequest(ap7.get(), &eb));
  }

  baton.post();
}
