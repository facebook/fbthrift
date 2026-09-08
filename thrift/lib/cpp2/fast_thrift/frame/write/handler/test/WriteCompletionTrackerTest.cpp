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

#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/WriteCompletionTracker.h>

#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::frame::write::handler {
namespace {

struct TestTransportWriteCompleteEvent
    : channel_pipeline::EventTag<TestTransportWriteCompleteEvent> {
  transport::WriteCompletionStatus status;
  size_t bytes;
};

struct TestBatchWriteCompleteEvent
    : channel_pipeline::EventTag<TestBatchWriteCompleteEvent> {
  transport::WriteCompletionStatus status;
  size_t frameCount;
  size_t bytes;
  bool quiesced;
};

struct TestEventFactory {
  using TransportWriteCompleteEventType = TestTransportWriteCompleteEvent;
  using BatchWriteCompleteEventType = TestBatchWriteCompleteEvent;
  using FlushWritesEventType = void;
  using PublishedEvents =
      channel_pipeline::Events<TransportWriteCompleteEventType>;

  static TestTransportWriteCompleteEvent make(
      transport::WriteCompletionStatus status, size_t bytes) noexcept {
    return {.status = status, .bytes = bytes};
  }

  static TestBatchWriteCompleteEvent makeBatchWriteComplete(
      transport::WriteCompletionStatus status,
      size_t frameCount,
      size_t bytes,
      bool quiesced) noexcept {
    return {
        .status = status,
        .frameCount = frameCount,
        .bytes = bytes,
        .quiesced = quiesced,
    };
  }
};

class CapturingContext {
 public:
  template <channel_pipeline::PipelineEvent E>
  void fireEvent(const typename E::Payload& event) noexcept {
    static_assert(std::same_as<E, TestBatchWriteCompleteEvent>);
    events_.push_back(event);
  }

  const std::vector<TestBatchWriteCompleteEvent>& events() const noexcept {
    return events_;
  }

 private:
  std::vector<TestBatchWriteCompleteEvent> events_;
};

// Helper: build a TransportWriteComplete box (what transport would fire).
TestTransportWriteCompleteEvent transportWriteComplete(
    transport::WriteCompletionStatus status, size_t bytes) noexcept {
  return TestEventFactory::make(status, bytes);
}

} // namespace

TEST(WriteCompletionTrackerTest, SingleBatchFiresOneEnrichedEvent) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  tracker.onWrite();
  tracker.onWrite();
  tracker.onWrite();
  tracker.onFlush();
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].status, transport::WriteCompletionStatus::Success);
  EXPECT_EQ(ctx.events()[0].frameCount, 3u);
  EXPECT_EQ(ctx.events()[0].bytes, 0u);
  // Nothing else outstanding and nothing buffered: egress is idle.
  EXPECT_TRUE(ctx.events()[0].quiesced);
}

TEST(WriteCompletionTrackerTest, MultipleInFlightBatchesPreserveFifoOrder) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  // Batch 1: 2 frames.
  tracker.onWrite();
  tracker.onWrite();
  tracker.onFlush();

  // Batch 2: 5 frames.
  for (int i = 0; i < 5; ++i) {
    tracker.onWrite();
  }
  tracker.onFlush();

  // Batch 3: 1 frame.
  tracker.onWrite();
  tracker.onFlush();

  // writeSuccess events arrive in FIFO order (AsyncSocket guarantee).
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 3u);
  EXPECT_EQ(ctx.events()[0].frameCount, 2u);
  EXPECT_EQ(ctx.events()[1].frameCount, 5u);
  EXPECT_EQ(ctx.events()[2].frameCount, 1u);
  // Only the last completion drains the queue, so it alone reports quiescence.
  EXPECT_FALSE(ctx.events()[0].quiesced);
  EXPECT_FALSE(ctx.events()[1].quiesced);
  EXPECT_TRUE(ctx.events()[2].quiesced);
}

TEST(WriteCompletionTrackerTest, BufferedFramesAwaitingFlushDeferQuiescence) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  tracker.onWrite();
  tracker.onFlush();

  // More frames buffered for the next batch, not yet flushed. The socket has
  // nothing left outstanding, but the connection is not idle — this is the
  // case an in-flight-count-only predicate would get wrong.
  tracker.onWrite();
  tracker.onWrite();

  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_FALSE(ctx.events()[0].quiesced);

  // Once that buffered batch flushes and completes, egress really is idle.
  tracker.onFlush();
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 2u);
  EXPECT_EQ(ctx.events()[1].frameCount, 2u);
  EXPECT_TRUE(ctx.events()[1].quiesced);
}

TEST(WriteCompletionTrackerTest, ErrorStatusAndBytesPropagateToEvent) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  tracker.onWrite();
  tracker.onWrite();
  tracker.onFlush();
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Error, 137));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].status, transport::WriteCompletionStatus::Error);
  EXPECT_EQ(ctx.events()[0].frameCount, 2u);
  EXPECT_EQ(ctx.events()[0].bytes, 137u);
}

TEST(WriteCompletionTrackerTest, EmptyBatchOnFlushIsIgnored) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  // Flush with no preceding onWrite — tracker should not push a 0-count
  // batch (otherwise the next writeSuccess would fire a meaningless event).
  tracker.onFlush();

  // Subsequent real batch is the only thing in the FIFO.
  tracker.onWrite();
  tracker.onFlush();
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].frameCount, 1u);
}

TEST(WriteCompletionTrackerTest, DiscardDropsCountsForFramesThatNeverFlush) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  // One batch on the wire and a partial batch still buffered, then the batcher
  // throws the buffered frames away. Without onDiscard the partial batch would
  // be charged to whatever flushes next and quiescence would never be reached.
  tracker.onWrite();
  tracker.onFlush();
  tracker.onWrite();
  tracker.onWrite();
  tracker.onDiscard();

  tracker.onWrite();
  tracker.onFlush();
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].frameCount, 1u);
  EXPECT_TRUE(ctx.events()[0].quiesced);
}

TEST(WriteCompletionTrackerTest, WriteCompleteWithEmptyFifoIsNoop) {
  WriteCompletionTrackerT<TestEventFactory> tracker;
  CapturingContext ctx;

  // Defensive: writeSuccess arriving without a corresponding flush (shouldn't
  // happen in practice) is dropped rather than UB.
  tracker.on<TestTransportWriteCompleteEvent>(
      ctx,
      transportWriteComplete(transport::WriteCompletionStatus::Success, 0));

  EXPECT_TRUE(ctx.events().empty());
}

} // namespace apache::thrift::fast_thrift::frame::write::handler
