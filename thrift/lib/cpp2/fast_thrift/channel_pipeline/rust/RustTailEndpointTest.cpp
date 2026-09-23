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

#include <memory>
#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
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
  RustTailEndpoint tail{rust_tail_endpoint_new_lifecycle_write_test(
      folly::IOBuf::copyBuffer("unattached"), true)};

  EXPECT_NO_FATAL_FAILURE(tail.onPipelineActive());
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

} // namespace
} // namespace channel_pipeline_rust::test
