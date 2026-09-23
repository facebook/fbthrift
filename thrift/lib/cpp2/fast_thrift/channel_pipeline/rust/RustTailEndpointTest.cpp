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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/rust/RustTailEndpoint.h>

#include <algorithm>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/test/TestLogHandler.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockHandler.h>

namespace channel_pipeline_rust::test {
namespace {

namespace cp = apache::thrift::fast_thrift::channel_pipeline;

HANDLER_TAG(tail_write_inner);

constexpr uint64_t kCloseOnReadToken = UINT64_MAX;
constexpr uint64_t kCloseOnReadyToken = UINT64_MAX - 1;
constexpr uint64_t kFeedbackWriteToken = UINT64_MAX - 2;
constexpr uint64_t kActivationWriteToken = UINT64_MAX - 3;

class ScopedLogCapture {
 public:
  ScopedLogCapture()
      : category_{folly::LoggerDB::get().getCategory("")},
        originalHandlers_{category_->getHandlers()},
        handler_{std::make_shared<folly::TestLogHandler>()} {
    auto handlers = originalHandlers_;
    handlers.push_back(handler_);
    category_->replaceHandlers(std::move(handlers));
  }

  ~ScopedLogCapture() {
    category_->replaceHandlers(std::move(originalHandlers_));
  }

  size_t rustTailEndpointErrorCount() const {
    const auto& messages = handler_->getMessages();
    return std::count_if(
        messages.begin(), messages.end(), [](const auto& entry) {
          return entry.first.getLevel() == folly::LogLevel::ERR &&
              entry.first.getFileName().endsWith("RustTailEndpoint.h");
        });
  }

 private:
  folly::LogCategory* category_;
  std::vector<std::shared_ptr<folly::LogHandler>> originalHandlers_;
  std::shared_ptr<folly::TestLogHandler> handler_;
};

class OrderedHead {
 public:
  explicit OrderedHead(std::vector<uint32_t>& order) : order_{order} {}

  cp::Result onWrite(
      cp::detail::ContextImpl& context, cp::TypeErasedBox&&) noexcept {
    context.fireException(
        folly::make_exception_wrapper<std::runtime_error>("write failed"));
    context.close();
    return cp::Result::Error;
  }
  void onReadReady() noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept { order_.push_back(6); }
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept { order_.push_back(3); }

 private:
  std::vector<uint32_t>& order_;
};

class ObservedTail {
 public:
  ObservedTail(
      rust::Box<RustTailEndpointOpaque> endpoint, std::vector<uint32_t>& order)
      : endpoint_{std::move(endpoint)}, order_{order} {}

  cp::Result onRead(
      cp::detail::ContextImpl& context, cp::TypeErasedBox&& message) noexcept {
    return endpoint_.onRead(context, std::move(message));
  }
  void onException(folly::exception_wrapper&& error) noexcept {
    endpoint_.onException(std::move(error));
    order_.push_back(0);
  }
  void onWriteReady() noexcept { endpoint_.onWriteReady(); }
  void onPipelineActive() noexcept { endpoint_.onPipelineActive(); }
  void onPipelineInactive() noexcept {
    endpoint_.onPipelineInactive();
    order_.push_back(1);
  }
  void handlerAdded() noexcept { endpoint_.handlerAdded(); }
  void handlerRemoved() noexcept {
    endpoint_.handlerRemoved();
    order_.push_back(4);
  }
  [[nodiscard]] bool setPipeline(cp::PipelineImpl* pipeline) noexcept {
    return endpoint_.setPipeline(pipeline);
  }

 private:
  RustTailEndpoint endpoint_;
  std::vector<uint32_t>& order_;
};

enum class ReentrantAction {
  None,
  WriteReady,
  Exception,
  ExceptionAndClose,
  Inactive,
  Close,
  DeactivateActivate,
  DeactivateActivateWriteReady,
  DeactivateActivateClose,
};

class ReentrantHead {
 public:
  ReentrantHead(
      std::vector<cp::Result> results, std::vector<ReentrantAction> actions)
      : results_{std::move(results)}, actions_{std::move(actions)} {}

  cp::Result onWrite(
      cp::detail::ContextImpl& context, cp::TypeErasedBox&& message) noexcept {
    writtenBytes_.push_back(std::move(message.get<cp::BytesPtr>()));
    const auto index = writtenBytes_.size() - 1;
    const auto action =
        index < actions_.size() ? actions_[index] : ReentrantAction::None;
    switch (action) {
      case ReentrantAction::None:
        break;
      case ReentrantAction::WriteReady:
        context.pipeline()->onWriteReady();
        break;
      case ReentrantAction::Exception:
        context.pipeline()->fireException(
            folly::make_exception_wrapper<std::runtime_error>("write error"));
        break;
      case ReentrantAction::ExceptionAndClose:
        context.pipeline()->fireException(
            folly::make_exception_wrapper<std::runtime_error>("write error"));
        context.pipeline()->close();
        break;
      case ReentrantAction::Inactive:
        context.pipeline()->deactivate();
        break;
      case ReentrantAction::Close:
        context.pipeline()->close();
        break;
      case ReentrantAction::DeactivateActivate:
        context.pipeline()->deactivate();
        context.pipeline()->activate();
        break;
      case ReentrantAction::DeactivateActivateWriteReady:
        context.pipeline()->deactivate();
        context.pipeline()->activate();
        context.pipeline()->onWriteReady();
        break;
      case ReentrantAction::DeactivateActivateClose:
        context.pipeline()->deactivate();
        context.pipeline()->activate();
        context.pipeline()->close();
        break;
    }
    return index < results_.size() ? results_[index] : cp::Result::Success;
  }

  void onWriteReady(cp::detail::ContextImpl&) noexcept {}
  void onReadReady() noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept { ++removedCount_; }
  void onPipelineActive() noexcept { ++activeCount_; }
  void onPipelineInactive() noexcept { ++inactiveCount_; }

  const std::vector<cp::BytesPtr>& writtenBytes() const noexcept {
    return writtenBytes_;
  }
  uint32_t activeCount() const noexcept { return activeCount_; }
  uint32_t inactiveCount() const noexcept { return inactiveCount_; }
  uint32_t removedCount() const noexcept { return removedCount_; }

 private:
  std::vector<cp::Result> results_;
  std::vector<ReentrantAction> actions_;
  std::vector<cp::BytesPtr> writtenBytes_;
  uint32_t activeCount_{0};
  uint32_t inactiveCount_{0};
  uint32_t removedCount_{0};
};

TailOutcomeTestConfig outcomeConfig(
    cp::Result result,
    std::unique_ptr<folly::IOBuf> readMessage,
    uint64_t readToken = 0,
    std::unique_ptr<folly::IOBuf> readyMessage = nullptr,
    uint64_t readyToken = 0,
    std::unique_ptr<folly::IOBuf> secondReadyMessage = nullptr,
    uint64_t secondReadyToken = 0,
    bool closeOnFeedback = false) {
  TailOutcomeTestConfig config;
  config.result = static_cast<int32_t>(result);
  config.read_message = std::move(readMessage);
  config.read_token = readToken;
  config.ready_message = std::move(readyMessage);
  config.ready_token = readyToken;
  config.second_ready_message = std::move(secondReadyMessage);
  config.second_ready_token = secondReadyToken;
  config.close_on_feedback = closeOnFeedback;
  return config;
}

template <typename Head>
auto buildOutcomePipeline(
    folly::EventBase& eventBase,
    Head& head,
    cp::SimpleBufferAllocator& allocator,
    RustTailEndpoint& tail) {
  auto pipeline =
      cp::PipelineBuilder<Head, RustTailEndpoint, cp::SimpleBufferAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .build();
  EXPECT_TRUE(tail.setPipeline(pipeline.get()));
  pipeline->activate();
  return pipeline;
}

void expectReadWriteFeedback(cp::Result writeResult, size_t feedbackIndex) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{writeResult}, {ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success, folly::IOBuf::copyBuffer("response"), 17))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      writeResult);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(counts[feedbackIndex], 1);
  EXPECT_EQ(counts[9], 17);
  EXPECT_EQ(counts[8], 0);
  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(
      head.writtenBytes().front()->cloneCoalesced()->moveToFbString(),
      "response");
  pipeline->close();
}

void expectLifecycleWriteResult(
    cp::Result result, bool onActivation, bool expectClosed) {
  rust_tail_endpoint_reset_lifecycle_write_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  head.setWriteResult(result);
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("write"), onActivation)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();
  if (!onActivation) {
    EXPECT_EQ(head.writeCount(), 0);
    pipeline->onWriteReady();
  }

  EXPECT_EQ(head.writeCount(), 1);
  EXPECT_EQ(head.exceptionCount(), 0);
  EXPECT_FALSE(pipeline->hasPendingWriteReady());
  EXPECT_EQ(pipeline->isClosed(), expectClosed);
  EXPECT_EQ(head.pipelineInactiveCount(), expectClosed ? 1 : 0);
  EXPECT_EQ(head.handlerRemovedCount(), expectClosed ? 1 : 0);
  auto counts = rust_tail_endpoint_lifecycle_write_test_counts();
  ASSERT_EQ(counts.size(), 4);
  EXPECT_EQ(counts[1], expectClosed ? 1 : 0);
  EXPECT_EQ(counts[2], expectClosed ? 1 : 0);

  if (!expectClosed) {
    pipeline->onWriteReady();
  }

  EXPECT_EQ(head.writeCount(), 1);
  EXPECT_EQ(head.exceptionCount(), 0);
  EXPECT_FALSE(pipeline->hasPendingWriteReady());
  pipeline->close();
  pipeline->close();
  EXPECT_EQ(head.pipelineInactiveCount(), 1);
  EXPECT_EQ(head.handlerRemovedCount(), 1);
  counts = rust_tail_endpoint_lifecycle_write_test_counts();
  ASSERT_EQ(counts.size(), 4);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 1);
}

TEST(RustTailEndpointTest, DelegatesDataAndLifecycle) {
  rust_tail_endpoint_reset_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_echo_test()};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();

  pipeline->activate();
  auto bytes = folly::IOBuf::copyBuffer("tail");
  EXPECT_EQ(
      pipeline->fireRead(cp::erase_and_box(std::move(bytes))),
      cp::Result::Success);
  EXPECT_EQ(head.writeCount(), 1);
  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(head.writtenBytes().front()->moveToFbString(), "tail");

  pipeline->onWriteReady();
  pipeline->fireException(
      folly::make_exception_wrapper<std::runtime_error>("test"));
  pipeline->deactivate();
  pipeline->close();

  const auto counts = rust_tail_endpoint_test_counts();
  const std::vector<uint32_t> expected{1, 1, 1, 1, 1, 1, 1};
  EXPECT_EQ(std::vector<uint32_t>(counts.begin(), counts.end()), expected);
}

TEST(RustTailEndpointTest, QueuedReadRunsLaterInTheCurrentLoopIteration) {
  rust_tail_endpoint_reset_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_queued_test()};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();

  pipeline->activate();
  eventBase.runInLoop([&] {
    EXPECT_EQ(
        pipeline->fireRead(
            cp::erase_and_box(folly::IOBuf::copyBuffer("queued"))),
        cp::Result::Success);
    EXPECT_EQ(rust_tail_endpoint_queued_test_completions(), 0);
  });

  eventBase.loopOnce(EVLOOP_NONBLOCK);

  EXPECT_EQ(rust_tail_endpoint_queued_test_completions(), 1);
  pipeline->deactivate();
  pipeline->close();
}

TEST(RustTailEndpointTest, ActivationReturnsSettingsWrite) {
  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("settings"), true)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();

  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(head.writtenBytes().front()->moveToFbString(), "settings");
  pipeline->close();
}

TEST(RustTailEndpointTest, WriteReadyReturnsPendingBytes) {
  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("ready"), false)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));
  pipeline->activate();

  pipeline->onWriteReady();

  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(head.writtenBytes().front()->moveToFbString(), "ready");
  pipeline->close();
}

TEST(RustTailEndpointTest, UnattachedLifecycleWriteIsDropped) {
  ScopedLogCapture logs;
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("unattached"), true)};

  EXPECT_NO_FATAL_FAILURE(tail.onPipelineActive());
  EXPECT_EQ(logs.rustTailEndpointErrorCount(), 1);
}

TEST(RustTailEndpointTest, LifecycleWriteAfterRemovalIsDropped) {
  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("removed"), false)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));
  pipeline->close();

  EXPECT_NO_FATAL_FAILURE(tail.onWriteReady());
  EXPECT_EQ(head.writeCount(), 0);
}

TEST(RustTailEndpointTest, ActivationWritePanicIsObservable) {
  rust_tail_endpoint_reset_lifecycle_callback_panic_count();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{
      rust_tail_endpoint_new_panicking_lifecycle_write_test(true)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();

  EXPECT_EQ(rust_tail_endpoint_lifecycle_callback_panic_count(), 1);
  EXPECT_EQ(head.writeCount(), 0);
  EXPECT_FALSE(pipeline->isClosed());
  pipeline->close();
}

TEST(RustTailEndpointTest, WriteReadyWritePanicIsObservable) {
  rust_tail_endpoint_reset_lifecycle_callback_panic_count();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{
      rust_tail_endpoint_new_panicking_lifecycle_write_test(false)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));
  pipeline->activate();

  pipeline->onWriteReady();

  EXPECT_EQ(rust_tail_endpoint_lifecycle_callback_panic_count(), 1);
  EXPECT_EQ(head.writeCount(), 0);
  EXPECT_FALSE(pipeline->isClosed());
  pipeline->close();
}

TEST(RustTailEndpointTest, AppliesWriteAfterRustBorrowBeforeTeardown) {
  rust_tail_endpoint_reset_lifecycle_write_test_counts();

  std::vector<uint32_t> order;
  folly::EventBase eventBase;
  OrderedHead head{order};
  cp::SimpleBufferAllocator allocator;
  ObservedTail tail{
      rust_tail_endpoint_new_lifecycle_write_test(
          folly::IOBuf::copyBuffer("settings"), true),
      order};
  auto inner = std::make_unique<cp::test::MockHandler>();
  inner->setOnPipelineDeactivated(
      [&](cp::detail::ContextImpl&) noexcept { order.push_back(2); });
  inner->setHandlerRemoved(
      [&](cp::detail::ContextImpl&) noexcept { order.push_back(5); });
  auto pipeline = cp::PipelineBuilder<
                      OrderedHead,
                      ObservedTail,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .addNextDuplex<cp::test::MockHandler>(
                          tail_write_inner_tag, std::move(inner))
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();

  EXPECT_TRUE(pipeline->isClosed());
  EXPECT_EQ(order, (std::vector<uint32_t>{0, 1, 2, 3, 4, 5, 6}));
  const auto counts = rust_tail_endpoint_lifecycle_write_test_counts();
  EXPECT_EQ(
      std::vector<uint32_t>(counts.begin(), counts.end()),
      (std::vector<uint32_t>{1, 1, 1, 0}));
}

TEST(RustTailEndpointTest, PipelineAttachmentIsNonNullAndOneShot) {
  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_echo_test()};
  EXPECT_FALSE(tail.setPipeline(nullptr));
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();

  EXPECT_TRUE(tail.setPipeline(pipeline.get()));
  EXPECT_FALSE(tail.setPipeline(pipeline.get()));
  RustTailEndpoint otherTail{rust_tail_endpoint_new_echo_test()};
  auto otherPipeline = cp::PipelineBuilder<
                           cp::test::MockHeadHandler,
                           RustTailEndpoint,
                           cp::SimpleBufferAllocator>()
                           .setEventBase(&eventBase)
                           .setHead(&head)
                           .setTail(&otherTail)
                           .setAllocator(&allocator)
                           .build();
  EXPECT_FALSE(tail.setPipeline(otherPipeline.get()));
  pipeline->close();
  EXPECT_FALSE(tail.setPipeline(otherPipeline.get()));
  otherPipeline->close();
}

TEST(RustTailEndpointTest, NullLifecycleWriteIsRejected) {
  rust_tail_endpoint_reset_lifecycle_write_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{
      rust_tail_endpoint_new_lifecycle_write_test(nullptr, true)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();

  EXPECT_EQ(head.writeCount(), 0);
  const auto counts = rust_tail_endpoint_lifecycle_write_test_counts();
  ASSERT_EQ(counts.size(), 4);
  EXPECT_EQ(counts[3], 1);
  pipeline->close();
}

TEST(RustTailEndpointTest, MultiNodeChainIsOnePipelineWrite) {
  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  auto frames = folly::IOBuf::copyBuffer("settings");
  frames->appendChain(folly::IOBuf::copyBuffer("ack"));
  RustTailEndpoint tail{
      rust_tail_endpoint_new_lifecycle_write_test(std::move(frames), true)};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();
  ASSERT_TRUE(tail.setPipeline(pipeline.get()));

  pipeline->activate();

  EXPECT_EQ(head.writeCount(), 1);
  ASSERT_EQ(head.writtenBytes().size(), 1);
  const auto& written = head.writtenBytes().front();
  EXPECT_EQ(written->countChainElements(), 2);
  EXPECT_EQ(written->computeChainDataLength(), 11);
  EXPECT_EQ(
      written->cloneCoalesced()->moveToFbString().toStdString(), "settingsack");
  pipeline->close();
}

TEST(RustTailEndpointTest, ActivationErrorClosesOnceAfterRustBorrow) {
  expectLifecycleWriteResult(cp::Result::Error, true, true);
}

TEST(RustTailEndpointTest, WriteReadyErrorClosesOnceAfterRustBorrow) {
  expectLifecycleWriteResult(cp::Result::Error, false, true);
}

TEST(RustTailEndpointTest, ActivationBackpressureDoesNotCloseOrRetry) {
  expectLifecycleWriteResult(cp::Result::Backpressure, true, false);
}

TEST(RustTailEndpointTest, WriteReadyBackpressureDoesNotCloseOrRetry) {
  expectLifecycleWriteResult(cp::Result::Backpressure, false, false);
}

class ReturnedReadWriteFeedbackTest
    : public testing::TestWithParam<std::tuple<cp::Result, size_t>> {};

TEST_P(ReturnedReadWriteFeedbackTest, ReportsTransportResult) {
  const auto [result, feedbackIndex] = GetParam();
  expectReadWriteFeedback(result, feedbackIndex);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnedReadWrite,
    ReturnedReadWriteFeedbackTest,
    testing::Values(
        std::tuple{cp::Result::Success, 1},
        std::tuple{cp::Result::Backpressure, 2},
        std::tuple{cp::Result::Error, 3}));

TEST(RustTailEndpointTest, FeedbackWithoutOutputReportsSuccess) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{}, {}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(
      outcomeConfig(cp::Result::Success, nullptr, 19))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(head.writtenBytes().size(), 0);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[9], 19);
  pipeline->close();
}

TEST(RustTailEndpointTest, WriteReadyFeedbackWithoutOutputReportsSuccess) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{}, {}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(
      outcomeConfig(cp::Result::Success, nullptr, 0, nullptr, 23))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  ASSERT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  pipeline->onWriteReady();

  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(head.writtenBytes().size(), 0);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[4], 1);
  EXPECT_EQ(counts[9], 23);
  pipeline->close();
}

TEST(RustTailEndpointTest, ComposesErrorOverBackpressureOverSuccess) {
  for (const auto& [semantic, transport, expected] :
       std::vector<std::tuple<cp::Result, cp::Result, cp::Result>>{
           {cp::Result::Error, cp::Result::Backpressure, cp::Result::Error},
           {cp::Result::Backpressure,
            cp::Result::Success,
            cp::Result::Backpressure},
           {cp::Result::Success, cp::Result::Success, cp::Result::Success}}) {
    folly::EventBase eventBase;
    ReentrantHead head{{transport}, {ReentrantAction::None}};
    cp::SimpleBufferAllocator allocator;
    RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(
        outcomeConfig(semantic, folly::IOBuf::copyBuffer("response")))};
    auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

    EXPECT_EQ(
        pipeline->fireRead(
            cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
        expected);
    pipeline->close();
  }
}

TEST(RustTailEndpointTest, SynchronousWriteReadyReplaysAfterFeedback) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Backpressure, cp::Result::Backpressure, cp::Result::Success},
      {ReentrantAction::WriteReady,
       ReentrantAction::WriteReady,
       ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("read"),
      11,
      folly::IOBuf::copyBuffer("ready-one"),
      22,
      folly::IOBuf::copyBuffer("ready-two"),
      33))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Backpressure);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(head.writtenBytes().size(), 3);
  EXPECT_EQ(head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "read");
  EXPECT_EQ(
      head.writtenBytes()[1]->cloneCoalesced()->moveToFbString(), "ready-one");
  EXPECT_EQ(
      head.writtenBytes()[2]->cloneCoalesced()->moveToFbString(), "ready-two");
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 2);
  EXPECT_EQ(counts[4], 2);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[9], 33);
  EXPECT_LT(counts[10], counts[11]);
  EXPECT_LT(counts[11], counts[12]);
  EXPECT_LT(counts[12], counts[13]);
  EXPECT_LT(counts[13], counts[14]);
  pipeline->close();
}

TEST(
    RustTailEndpointTest,
    SynchronousExceptionReplaysAfterFeedbackWithoutAliasing) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::Exception}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success, folly::IOBuf::copyBuffer("response"), 41))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(counts[5], 1);
  EXPECT_EQ(counts[1], 1);
  EXPECT_LT(counts[10], counts[15]);
  EXPECT_EQ(counts[8], 0);
  pipeline->close();
}

TEST(RustTailEndpointTest, SynchronousInactiveSkipsStaleFeedback) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::Inactive}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success, folly::IOBuf::copyBuffer("response"), 51))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[8], 0);
  pipeline->close();
}

TEST(RustTailEndpointTest, SynchronousRemovalSkipsStaleFeedback) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::Close}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success, folly::IOBuf::copyBuffer("response"), 61))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Error);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[8], 0);
  EXPECT_LT(counts[16], counts[17]);
}

TEST(RustTailEndpointTest, FeedbackCloseDefersLifecycleUntilBorrowEnds) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("response"),
      71,
      nullptr,
      0,
      nullptr,
      0,
      true))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Error);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[8], 0);
  EXPECT_LT(counts[10], counts[16]);
  EXPECT_LT(counts[16], counts[17]);
}

TEST(RustTailEndpointTest, ReadCloseDiscardsReturnedWriteAndFeedback) {
  rust_tail_endpoint_reset_read_outcome_test_counts();
  ScopedLogCapture logs;

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("stale-read"),
      kCloseOnReadToken))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Error);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_TRUE(pipeline->isClosed());
  EXPECT_TRUE(head.writtenBytes().empty());
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[16], 1);
  EXPECT_EQ(counts[17], 2);
  EXPECT_EQ(logs.rustTailEndpointErrorCount(), 1);
}

TEST(RustTailEndpointTest, WriteReadyCloseDiscardsReturnedWriteAndFeedback) {
  rust_tail_endpoint_reset_read_outcome_test_counts();
  ScopedLogCapture logs;

  folly::EventBase eventBase;
  ReentrantHead head{{cp::Result::Success}, {ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      nullptr,
      0,
      folly::IOBuf::copyBuffer("stale-ready"),
      kCloseOnReadyToken))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  ASSERT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  pipeline->onWriteReady();

  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  EXPECT_TRUE(pipeline->isClosed());
  EXPECT_TRUE(head.writtenBytes().empty());
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[4], 1);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[11], 1);
  EXPECT_EQ(counts[16], 2);
  EXPECT_EQ(counts[17], 3);
  EXPECT_EQ(logs.rustTailEndpointErrorCount(), 1);
}

TEST(RustTailEndpointTest, FeedbackExceptionIsDeferredWithoutLosingPayload) {
  rust_tail_endpoint_reset_read_outcome_test_counts();

  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Success, cp::Result::Error},
      {ReentrantAction::None, ReentrantAction::ExceptionAndClose}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("response"),
      91,
      nullptr,
      0,
      folly::IOBuf::copyBuffer("feedback-write"),
      kFeedbackWriteToken))};
  std::vector<std::string> exceptions;
  tail.setOnException([&](folly::exception_wrapper&& error) noexcept {
    exceptions.push_back(error.what().toStdString());
  });
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Error);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  ASSERT_EQ(head.writtenBytes().size(), 2);
  EXPECT_EQ(
      head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "response");
  EXPECT_EQ(
      head.writtenBytes()[1]->cloneCoalesced()->moveToFbString(),
      "feedback-write");
  ASSERT_EQ(exceptions.size(), 1);
  EXPECT_EQ(exceptions.front(), "std::runtime_error: write error");
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[5], 1);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[8], 0);
  EXPECT_LT(counts[10], counts[15]);
  EXPECT_LT(counts[15], counts[16]);
  EXPECT_LT(counts[16], counts[17]);
}

TEST(RustTailEndpointTest, ReturnedWriteReplaysInactiveThenActiveWithOutput) {
  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Success, cp::Result::Success},
      {ReentrantAction::DeactivateActivate, ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("response"),
      101,
      folly::IOBuf::copyBuffer("activation"),
      kActivationWriteToken))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);
  rust_tail_endpoint_reset_read_outcome_test_counts();

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  ASSERT_EQ(head.writtenBytes().size(), 2);
  EXPECT_EQ(
      head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "response");
  EXPECT_EQ(
      head.writtenBytes()[1]->cloneCoalesced()->moveToFbString(), "activation");
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 0);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[18], 1);
  EXPECT_EQ(counts[16], 1);
  EXPECT_EQ(counts[19], 2);
  EXPECT_EQ(head.activeCount(), 2);
  EXPECT_EQ(head.inactiveCount(), 1);
  EXPECT_EQ(head.removedCount(), 0);
  EXPECT_FALSE(pipeline->isClosed());
  pipeline->close();
}

TEST(RustTailEndpointTest, FeedbackReplaysInactiveThenActiveWithOutput) {
  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Success, cp::Result::Success, cp::Result::Success},
      {ReentrantAction::None,
       ReentrantAction::DeactivateActivate,
       ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("response"),
      102,
      folly::IOBuf::copyBuffer("activation"),
      kActivationWriteToken,
      folly::IOBuf::copyBuffer("feedback-write"),
      kFeedbackWriteToken))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);
  rust_tail_endpoint_reset_read_outcome_test_counts();

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  ASSERT_EQ(head.writtenBytes().size(), 3);
  EXPECT_EQ(
      head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "response");
  EXPECT_EQ(
      head.writtenBytes()[1]->cloneCoalesced()->moveToFbString(),
      "feedback-write");
  EXPECT_EQ(
      head.writtenBytes()[2]->cloneCoalesced()->moveToFbString(), "activation");
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 0);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[18], 1);
  EXPECT_LT(counts[10], counts[16]);
  EXPECT_LT(counts[16], counts[19]);
  EXPECT_EQ(head.activeCount(), 2);
  EXPECT_EQ(head.inactiveCount(), 1);
  EXPECT_EQ(head.removedCount(), 0);
  EXPECT_FALSE(pipeline->isClosed());
  pipeline->close();
}

TEST(RustTailEndpointTest, LifecycleAndActivationReplayBeforeRelatchedReady) {
  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Success, cp::Result::Success, cp::Result::Success},
      {ReentrantAction::DeactivateActivateWriteReady,
       ReentrantAction::None,
       ReentrantAction::None}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("activation"),
      kActivationWriteToken,
      folly::IOBuf::copyBuffer("ready-one"),
      201,
      folly::IOBuf::copyBuffer("ready-two"),
      202))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);
  ASSERT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Success);
  ASSERT_TRUE(head.writtenBytes().empty());
  rust_tail_endpoint_reset_read_outcome_test_counts();

  pipeline->onWriteReady();

  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  ASSERT_EQ(head.writtenBytes().size(), 3);
  EXPECT_EQ(
      head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "ready-one");
  EXPECT_EQ(
      head.writtenBytes()[1]->cloneCoalesced()->moveToFbString(), "activation");
  EXPECT_EQ(
      head.writtenBytes()[2]->cloneCoalesced()->moveToFbString(), "ready-two");
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[4], 2);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 0);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[9], 202);
  EXPECT_EQ(counts[11], 1);
  EXPECT_EQ(counts[16], 2);
  EXPECT_EQ(counts[19], 3);
  EXPECT_EQ(counts[13], 4);
  EXPECT_EQ(counts[10], 5);
  EXPECT_EQ(head.activeCount(), 2);
  EXPECT_EQ(head.inactiveCount(), 1);
  EXPECT_EQ(head.removedCount(), 0);
  EXPECT_FALSE(pipeline->isClosed());
  pipeline->close();
}

TEST(RustTailEndpointTest, RemovalSuppressesPendingActivationOutput) {
  folly::EventBase eventBase;
  ReentrantHead head{
      {cp::Result::Success}, {ReentrantAction::DeactivateActivateClose}};
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_read_outcome_test(outcomeConfig(
      cp::Result::Success,
      folly::IOBuf::copyBuffer("response"),
      103,
      folly::IOBuf::copyBuffer("stale-activation"),
      kActivationWriteToken))};
  auto pipeline = buildOutcomePipeline(eventBase, head, allocator, tail);
  rust_tail_endpoint_reset_read_outcome_test_counts();

  EXPECT_EQ(
      pipeline->fireRead(
          cp::erase_and_box(folly::IOBuf::copyBuffer("request"))),
      cp::Result::Error);
  const auto counts = rust_tail_endpoint_read_outcome_test_counts();
  ASSERT_EQ(counts.size(), 20);
  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(
      head.writtenBytes()[0]->cloneCoalesced()->moveToFbString(), "response");
  EXPECT_EQ(counts[1] + counts[2] + counts[3], 0);
  EXPECT_EQ(counts[6], 1);
  EXPECT_EQ(counts[7], 1);
  EXPECT_EQ(counts[8], 0);
  EXPECT_EQ(counts[18], 0);
  EXPECT_EQ(counts[16], 1);
  EXPECT_EQ(counts[17], 2);
  EXPECT_EQ(head.activeCount(), 2);
  EXPECT_EQ(head.inactiveCount(), 2);
  EXPECT_EQ(head.removedCount(), 1);
  EXPECT_TRUE(pipeline->isClosed());
}

} // namespace
} // namespace channel_pipeline_rust::test
