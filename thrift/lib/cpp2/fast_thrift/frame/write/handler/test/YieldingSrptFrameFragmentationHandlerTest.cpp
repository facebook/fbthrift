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

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/portability/GMock.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/StaticPipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/YieldingSrptFrameFragmentationHandler.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::frame::write::handler {
namespace {

using namespace testing;

namespace cp = channel_pipeline;
using cp::test::TestAllocator;

HANDLER_TAG(yielding_srpt_test);

class PipelineHeadHandler {
 public:
  template <typename Context>
  cp::Result onWrite(Context&, cp::TypeErasedBox&&) noexcept {
    ++writeCount_;
    return writeResult_;
  }

  void onException(folly::exception_wrapper&&) noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}
  void onReadReady() noexcept {}

  void setWriteResult(cp::Result result) noexcept { writeResult_ = result; }
  [[nodiscard]] size_t writeCount() const noexcept { return writeCount_; }

 private:
  cp::Result writeResult_{cp::Result::Success};
  size_t writeCount_{0};
};

class PipelineTailHandler {
 public:
  template <typename Context>
  cp::Result onRead(Context&, cp::TypeErasedBox&&) noexcept {
    return cp::Result::Success;
  }

  void onException(folly::exception_wrapper&&) noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}
  void onWriteReady() noexcept {}
};

constexpr size_t kQuantum = 100;
constexpr size_t kBudget = 1000;

struct RecordingTracker {
  using PublishedEvents = channel_pipeline::Events<>;
  using SubscribedEvents = channel_pipeline::Events<>;

  struct FragmentCall {
    uint32_t streamId;
    bool isLastFragment;

    bool operator==(const FragmentCall&) const = default;
  };

  void onFragment(uint32_t streamId, bool isLastFragment) noexcept {
    fragments.push_back({streamId, isLastFragment});
  }
  void onFragmentAborted() noexcept {
    ++abortCount;
    fragments.pop_back();
  }
  void onFlush() noexcept { ++flushCount; }

  std::vector<FragmentCall> fragments;
  size_t flushCount{0};
  size_t abortCount{0};
};

static_assert(FragmentCompletionTracker<RecordingTracker>);

struct TestBatchWriteCompleteEvent
    : channel_pipeline::EventTag<TestBatchWriteCompleteEvent> {
  transport::WriteCompletionStatus status;
  size_t frameCount;
  size_t bytes;
  bool quiesced;
};

struct TestFrameWriteCompleteEvent
    : channel_pipeline::EventTag<TestFrameWriteCompleteEvent> {
  uint32_t streamId;
  transport::WriteCompletionStatus status;
  bool quiesced;

  bool operator==(const TestFrameWriteCompleteEvent& other) const noexcept {
    return streamId == other.streamId && status == other.status &&
        quiesced == other.quiesced;
  }
};

struct TestEventFactory {
  using BatchWriteCompleteEventType = TestBatchWriteCompleteEvent;
  using FrameWriteCompleteEventType = TestFrameWriteCompleteEvent;

  static TestFrameWriteCompleteEvent makeFrameWriteComplete(
      transport::WriteCompletionStatus status,
      uint32_t streamId,
      bool quiesced) noexcept {
    return {
        .streamId = streamId,
        .status = status,
        .quiesced = quiesced,
    };
  }
};

struct CapturedFrame {
  FrameType frameType;
  uint32_t streamId;
  size_t metadataSize;
  size_t dataSize;
  bool follows;
};

class TestContext {
 public:
  explicit TestContext(folly::EventBase* eventBase) : eventBase_(eventBase) {}

  folly::EventBase* eventBase() const noexcept { return eventBase_; }

  channel_pipeline::Result fireWrite(
      channel_pipeline::TypeErasedBox&& msg) noexcept {
    const size_t attempt = writeAttemptCount_++;
    const channel_pipeline::Result result = attempt < results_.size()
        ? results_[attempt]
        : channel_pipeline::Result::Success;
    if (writeAttemptObserver_) {
      writeAttemptObserver_();
    }
    if (result == channel_pipeline::Result::Error) {
      return result;
    }

    const ComposedFrame& frame = msg.get<ComposedFrame>();
    written_.push_back({
        .frameType = frame.frameType,
        .streamId = frame.streamId,
        .metadataSize =
            frame.metadata ? frame.metadata->computeChainDataLength() : 0,
        .dataSize = frame.data ? frame.data->computeChainDataLength() : 0,
        .follows = frame.follows,
    });
    if (std::function<void()> write = std::exchange(reentrantWrite_, nullptr);
        write) {
      write();
    }
    return result;
  }

  void fireException(folly::exception_wrapper&& exception) noexcept {
    ++fireExceptionCount_;
    lastException_ = std::move(exception);
  }

  template <channel_pipeline::PipelineEvent E>
  void fireEvent(const typename E::Payload& event) noexcept {
    static_assert(std::same_as<E, TestFrameWriteCompleteEvent>);
    completed_.push_back(event);
  }

  void awaitWriteReady() noexcept { awaitingWriteReady_ = true; }
  void cancelAwaitWriteReady() noexcept { awaitingWriteReady_ = false; }

  void setResults(std::vector<channel_pipeline::Result> results) {
    results_ = std::move(results);
  }

  void setReentrantWrite(std::function<void()> write) {
    reentrantWrite_ = std::move(write);
  }

  void setWriteAttemptObserver(std::function<void()> observer) {
    writeAttemptObserver_ = std::move(observer);
  }

  [[nodiscard]] const std::vector<CapturedFrame>& written() const noexcept {
    return written_;
  }
  [[nodiscard]] const std::vector<TestFrameWriteCompleteEvent>& completed()
      const noexcept {
    return completed_;
  }
  [[nodiscard]] size_t writeAttemptCount() const noexcept {
    return writeAttemptCount_;
  }
  [[nodiscard]] bool awaitingWriteReady() const noexcept {
    return awaitingWriteReady_;
  }
  [[nodiscard]] size_t fireExceptionCount() const noexcept {
    return fireExceptionCount_;
  }
  [[nodiscard]] const folly::exception_wrapper& lastException() const noexcept {
    return lastException_;
  }

 private:
  folly::EventBase* const eventBase_;
  std::vector<channel_pipeline::Result> results_;
  std::function<void()> reentrantWrite_;
  std::function<void()> writeAttemptObserver_;
  std::vector<CapturedFrame> written_;
  std::vector<TestFrameWriteCompleteEvent> completed_;
  size_t writeAttemptCount_{0};
  bool awaitingWriteReady_{false};
  size_t fireExceptionCount_{0};
  folly::exception_wrapper lastException_;
};

using Handler = YieldingSrptFrameFragmentationHandlerT<RecordingTracker>;
using CompletionHandler = YieldingSrptFrameFragmentationHandlerT<
    FragmentCompletionTrackerT<TestEventFactory>>;

static_assert(channel_pipeline::OutboundHandler<Handler, TestContext>);

channel_pipeline::TypeErasedBox makeFrame(
    FrameType frameType,
    uint32_t streamId,
    size_t dataSize = 0,
    bool complete = false,
    size_t metadataSize = 0) {
  ComposedFrame frame{
      .frameType = frameType,
      .streamId = streamId,
      .complete = complete,
  };
  if (metadataSize > 0) {
    frame.metadata = folly::IOBuf::create(metadataSize);
    frame.metadata->append(metadataSize);
    std::memset(frame.metadata->writableData(), 'm', metadataSize);
  }
  if (dataSize > 0) {
    frame.data = folly::IOBuf::create(dataSize);
    frame.data->append(dataSize);
    std::memset(frame.data->writableData(), 'd', dataSize);
  }
  if (frameType == FrameType::PAYLOAD) {
    frame.next = true;
  }
  return channel_pipeline::erase_and_box(std::move(frame));
}

std::vector<uint32_t> streamIds(const std::vector<CapturedFrame>& frames) {
  std::vector<uint32_t> result;
  result.reserve(frames.size());
  for (const CapturedFrame& frame : frames) {
    result.push_back(frame.streamId);
  }
  return result;
}

std::vector<size_t> dataSizes(const std::vector<CapturedFrame>& frames) {
  std::vector<size_t> result;
  result.reserve(frames.size());
  for (const CapturedFrame& frame : frames) {
    result.push_back(frame.dataSize);
  }
  return result;
}

std::set<uint32_t> unfinishedStreams(const std::vector<CapturedFrame>& frames) {
  std::set<uint32_t> result;
  for (const CapturedFrame& frame : frames) {
    if (frame.follows) {
      result.insert(frame.streamId);
    } else {
      result.erase(frame.streamId);
    }
  }
  return result;
}

class YieldingSrptFrameFragmentationHandlerTest : public Test {
 protected:
  void runLoop() { eventBase_.loopOnce(EVLOOP_NONBLOCK); }

  folly::EventBase eventBase_;
  TestContext context_{&eventBase_};
};

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    InvalidConfigRejectsDataWithoutQueuingIt) {
  const auto expectRejected = [this](QuantumFragmentationHandlerConfig config) {
    Handler handler(config);
    handler.handlerAdded(context_);

    EXPECT_THAT(
        handler.onWrite(context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 1)),
        Eq(channel_pipeline::Result::Error));
    EXPECT_THAT(context_.written(), IsEmpty());
    EXPECT_THAT(
        std::tuple(handler.hasPendingWrites(), handler.isScheduled()),
        Eq(std::tuple(false, false)));
  };

  expectRejected({.streamQuantum = 0});
  expectRejected({.streamQuantum = kMaxStreamQuantum + 1});
  expectRejected({.callbackBudget = 0});
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    LoneLargeRequestSendsOneQuantumAndYields) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1));
  EXPECT_THAT(dataSizes(context_.written()), ElementsAre(kQuantum));
  EXPECT_THAT(context_.written()[0].follows, IsTrue());
  EXPECT_THAT(handler.hasPendingStream(1), IsTrue());
  EXPECT_THAT(handler.isScheduled(), IsTrue());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    KLargeRequestsGiveThePinnedShortestRequestKTurns) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 500));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 600));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 700));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 1, 1));
  EXPECT_THAT(dataSizes(context_.written()), ElementsAre(100, 100, 100));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    RemainingLargeTurnsMoveToTheNextShortestRequest) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 150));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 300));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 400));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 1, 3));
  EXPECT_THAT(dataSizes(context_.written()), ElementsAre(100, 50, 100));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    VisibleSingleQuantumRequestsRunFirstInRemainingByteOrder) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 20));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 7, 70));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(5, 3, 7, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    SmallRequestAdmittedBetweenPassesPreemptsPinnedLargeRequest) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 400));
  runLoop();
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 3, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    RequestAdmittedDuringAPassWaitsForTheNextPass) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 300));
  context_.setReentrantWrite([&] {
    EXPECT_THAT(
        handler.onWrite(
            context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50)),
        Eq(channel_pipeline::Result::Success));
  });

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1));
  runLoop();
  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 3, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    CancelDuringAPassDropsOnlyThatStreamsDeferredFrames) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 300));
  context_.setReentrantWrite([&] {
    (void)handler.onWrite(
        context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));
    (void)handler.onWrite(
        context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 50));
    (void)handler.onWrite(context_, makeFrame(FrameType::CANCEL, 3));
  });

  runLoop();
  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 3, 5, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    NewLargeRequestCannotPreemptPinnedLargeRequest) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 500));
  runLoop();
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 150));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 1, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    AtMostOneLargeRequestIsPartialAtPassEnd) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 150));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 250));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 400));

  runLoop();

  EXPECT_THAT(unfinishedStreams(context_.written()), ElementsAre(3));
  EXPECT_THAT(handler.hasPendingStream(5), IsTrue());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    CallbackBudgetStopsNewTurnsWithoutCuttingAStartedTurn) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = 150});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 100));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 80));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 5, 300));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(3, 1));
  EXPECT_THAT(dataSizes(context_.written()), ElementsAre(80, 100));
  EXPECT_THAT(handler.hasPendingStream(5), IsTrue());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    MetadataCountsTowardQuantumButCallbackBudgetCountsData) {
  Handler handler({.streamQuantum = 100, .callbackBudget = 40});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 60, false, 60));

  runLoop();

  EXPECT_THAT(context_.written(), SizeIs(1));
  EXPECT_THAT(
      std::tuple(
          context_.written()[0].metadataSize,
          context_.written()[0].dataSize,
          context_.written()[0].follows),
      Eq(std::tuple(size_t{60}, size_t{40}, true)));

  runLoop();

  EXPECT_THAT(context_.written(), SizeIs(2));
  EXPECT_THAT(
      std::tuple(
          context_.written()[1].metadataSize,
          context_.written()[1].dataSize,
          context_.written()[1].follows),
      Eq(std::tuple(size_t{0}, size_t{20}, false)));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest, BackpressureDoesNotStopThePass) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 20));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));

  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 3));
  EXPECT_THAT(
      std::tuple(
          context_.awaitingWriteReady(),
          handler.isBackpressured(),
          handler.hasPendingWrites(),
          handler.tracker().flushCount),
      Eq(std::tuple(true, true, false, size_t{1})));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    DataArrivingUnderBackpressureQueuesAndReportsBackpressure) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});
  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250)),
      Eq(channel_pipeline::Result::Success));
  runLoop();

  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50)),
      Eq(channel_pipeline::Result::Backpressure));
  EXPECT_THAT(context_.written(), SizeIs(1));

  runLoop();
  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 3, 1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PassThroughBackpressureReportsUntilWriteReady) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});

  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_N, 1)),
      Eq(channel_pipeline::Result::Backpressure));
  EXPECT_THAT(
      std::tuple(handler.isBackpressured(), context_.awaitingWriteReady()),
      Eq(std::tuple(true, true)));
  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_N, 3)),
      Eq(channel_pipeline::Result::Backpressure));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    WriteReadyClearsBackpressureWithoutSending) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
  runLoop();

  handler.onWriteReady(context_);

  EXPECT_THAT(
      std::tuple(
          context_.awaitingWriteReady(),
          handler.isBackpressured(),
          context_.written().size()),
      Eq(std::tuple(false, false, size_t{1})));
  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50)),
      Eq(channel_pipeline::Result::Success));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PreservesSameStreamFrameOrderAcrossPasses) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(context_, makeFrame(FrameType::PAYLOAD, 1, 150));
  (void)handler.onWrite(context_, makeFrame(FrameType::PAYLOAD, 1, 50));

  runLoop();
  runLoop();
  runLoop();

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(1, 1, 1));
  EXPECT_THAT(dataSizes(context_.written()), ElementsAre(100, 50, 50));
  const std::array<bool, 3> follows{
      context_.written()[0].follows,
      context_.written()[1].follows,
      context_.written()[2].follows};
  EXPECT_THAT(follows, ElementsAre(true, false, false));
}

TEST(
    YieldingSrptFrameFragmentationHandlerTerminalTest,
    CancelAndErrorRemovePinnedStreamData) {
  for (const FrameType terminal : {FrameType::CANCEL, FrameType::ERROR}) {
    folly::EventBase eventBase;
    TestContext context(&eventBase);
    Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
    handler.handlerAdded(context);
    (void)handler.onWrite(
        context, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
    eventBase.loopOnce(EVLOOP_NONBLOCK);

    EXPECT_THAT(
        handler.onWrite(context, makeFrame(terminal, 1)),
        Eq(channel_pipeline::Result::Success));

    EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
    EXPECT_THAT(context.written(), SizeIs(2));
    EXPECT_THAT(context.written()[1].frameType, Eq(terminal));
    eventBase.loopOnce(EVLOOP_NONBLOCK);
    EXPECT_THAT(context.written(), SizeIs(2));
  }
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    ControlFramesKeepTheInlinePassThroughContract) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));

  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::REQUEST_N, 1)),
      Eq(channel_pipeline::Result::Success));

  EXPECT_THAT(context_.written(), SizeIs(1));
  EXPECT_THAT(context_.written()[0].frameType, Eq(FrameType::REQUEST_N));
  EXPECT_THAT(handler.hasPendingStream(1), IsTrue());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    DownstreamErrorClearsWorkAndPropagatesAnException) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Error});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));

  runLoop();

  EXPECT_THAT(context_.writeAttemptCount(), Eq(1));
  EXPECT_THAT(context_.written(), IsEmpty());
  EXPECT_THAT(handler.tracker().fragments, IsEmpty());
  EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
  EXPECT_THAT(handler.isScheduled(), IsFalse());
  EXPECT_THAT(context_.fireExceptionCount(), Eq(1));
  EXPECT_THAT(
      std::string(context_.lastException().what()),
      HasSubstr(
          "YieldingSrptFrameFragmentationHandler: downstream write "
          "failed"));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    CompletionFiresOnceAfterTheLastFragmentAcrossPasses) {
  CompletionHandler handler(
      {.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 150));

  runLoop();
  handler.on<TestBatchWriteCompleteEvent>(
      context_,
      TestBatchWriteCompleteEvent{
          .status = transport::WriteCompletionStatus::Success,
          .frameCount = 1,
          .bytes = 100,
          .quiesced = true,
      });
  EXPECT_THAT(context_.completed(), IsEmpty());

  runLoop();
  handler.on<TestBatchWriteCompleteEvent>(
      context_,
      TestBatchWriteCompleteEvent{
          .status = transport::WriteCompletionStatus::Success,
          .frameCount = 1,
          .bytes = 50,
          .quiesced = true,
      });

  EXPECT_THAT(context_.completed(), SizeIs(1));
  EXPECT_THAT(
      std::tuple(
          context_.completed()[0].streamId,
          context_.completed()[0].status,
          context_.completed()[0].quiesced),
      Eq(std::tuple(
          uint32_t{1}, transport::WriteCompletionStatus::Success, true)));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    QueuedInlineCompletionSeesTheFragmentRecord) {
  CompletionHandler handler(
      {.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setWriteAttemptObserver([&] {
    handler.on<TestBatchWriteCompleteEvent>(
        context_,
        TestBatchWriteCompleteEvent{
            .status = transport::WriteCompletionStatus::Success,
            .frameCount = 1,
            .bytes = 20,
            .quiesced = true,
        });
  });
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 20));

  runLoop();

  const std::vector<TestFrameWriteCompleteEvent> expected{{
      .streamId = 1,
      .status = transport::WriteCompletionStatus::Success,
      .quiesced = true,
  }};
  EXPECT_THAT(context_.completed(), Eq(expected));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PassThroughInlineCompletionSeesTheFragmentRecord) {
  CompletionHandler handler(
      {.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setWriteAttemptObserver([&] {
    handler.on<TestBatchWriteCompleteEvent>(
        context_,
        TestBatchWriteCompleteEvent{
            .status = transport::WriteCompletionStatus::Success,
            .frameCount = 1,
            .bytes = 0,
            .quiesced = true,
        });
  });

  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::KEEPALIVE, 0)),
      Eq(channel_pipeline::Result::Success));

  const std::vector<TestFrameWriteCompleteEvent> expected{{
      .streamId = 0,
      .status = transport::WriteCompletionStatus::Success,
      .quiesced = true,
  }};
  EXPECT_THAT(context_.completed(), Eq(expected));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    RemovedDeferredFramesDoNotBlockQuiescence) {
  CompletionHandler handler(
      {.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 20));
  context_.setReentrantWrite([&] {
    (void)handler.onWrite(
        context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 50));
    (void)handler.onWrite(context_, makeFrame(FrameType::CANCEL, 3));
    handler.on<TestBatchWriteCompleteEvent>(
        context_,
        TestBatchWriteCompleteEvent{
            .status = transport::WriteCompletionStatus::Success,
            .frameCount = 2,
            .bytes = 20,
            .quiesced = true,
        });
  });

  runLoop();

  const std::vector<TestFrameWriteCompleteEvent> expected{
      {
          .streamId = 1,
          .status = transport::WriteCompletionStatus::Success,
          .quiesced = false,
      },
      {
          .streamId = 3,
          .status = transport::WriteCompletionStatus::Success,
          .quiesced = true,
      },
  };
  EXPECT_THAT(context_.completed(), Eq(expected));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    QueuedWriteErrorAbortsTheRecordedFragment) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  size_t recordsDuringWrite = 0;
  context_.setWriteAttemptObserver(
      [&] { recordsDuringWrite = handler.tracker().fragments.size(); });
  context_.setResults({channel_pipeline::Result::Error});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 20));

  runLoop();

  EXPECT_THAT(recordsDuringWrite, Eq(1));
  EXPECT_THAT(handler.tracker().fragments, IsEmpty());
  EXPECT_THAT(handler.tracker().abortCount, Eq(1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PassThroughWriteErrorAbortsTheRecordedFragment) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  size_t recordsDuringWrite = 0;
  context_.setWriteAttemptObserver(
      [&] { recordsDuringWrite = handler.tracker().fragments.size(); });
  context_.setResults({channel_pipeline::Result::Error});

  EXPECT_THAT(
      handler.onWrite(context_, makeFrame(FrameType::KEEPALIVE, 0)),
      Eq(channel_pipeline::Result::Error));

  EXPECT_THAT(recordsDuringWrite, Eq(1));
  EXPECT_THAT(handler.tracker().fragments, IsEmpty());
  EXPECT_THAT(handler.tracker().abortCount, Eq(1));
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    HandlerRemovalCancelsCallbackAndClearsWork) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));

  handler.handlerRemoved(context_);

  EXPECT_THAT(handler.isScheduled(), IsFalse());
  EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
  runLoop();
  EXPECT_THAT(context_.written(), IsEmpty());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    HandlerRemovalCancelsWriteReadyHook) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
  runLoop();
  EXPECT_THAT(context_.awaitingWriteReady(), IsTrue());

  handler.handlerRemoved(context_);

  EXPECT_THAT(context_.awaitingWriteReady(), IsFalse());
  EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
  EXPECT_THAT(handler.isScheduled(), IsFalse());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PipelineInactiveCancelsCallbackAndClearsWork) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));

  handler.onPipelineInactive(context_);

  EXPECT_THAT(handler.isScheduled(), IsFalse());
  EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
  runLoop();
  EXPECT_THAT(context_.written(), IsEmpty());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PipelineInactiveCancelsWriteReadyHook) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  context_.setResults({channel_pipeline::Result::Backpressure});
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 250));
  runLoop();
  EXPECT_THAT(context_.awaitingWriteReady(), IsTrue());

  handler.onPipelineInactive(context_);

  EXPECT_THAT(context_.awaitingWriteReady(), IsFalse());
  EXPECT_THAT(handler.hasPendingWrites(), IsFalse());
  EXPECT_THAT(handler.isScheduled(), IsFalse());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    PipelineInactiveDuringAPassEndsThePass) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kBudget});
  handler.handlerAdded(context_);
  // A socket write that fails inside the pass closes the pipeline before the
  // write returns.
  context_.setWriteAttemptObserver(
      [&] { handler.onPipelineInactive(context_); });
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 20));
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 3, 20));

  runLoop();

  EXPECT_THAT(
      std::tuple(
          context_.writeAttemptCount(),
          handler.hasPendingWrites(),
          handler.isScheduled(),
          context_.fireExceptionCount()),
      Eq(std::tuple(size_t{1}, false, false, size_t{0})));
}

class YieldingSrptFrameFragmentationHandlerPipelineTest : public Test {
 protected:
  cp::PipelineImpl::Ptr buildDynamicPipeline() {
    return cp::PipelineBuilder<
               PipelineHeadHandler,
               PipelineTailHandler,
               TestAllocator>()
        .setEventBase(&eventBase_)
        .setHead(&transport_)
        .setTail(&app_)
        .setAllocator(&allocator_)
        .addNextOutbound<Handler>(
            yielding_srpt_test_tag,
            std::make_unique<Handler>(QuantumFragmentationHandlerConfig{
                .streamQuantum = kQuantum,
                .callbackBudget = kBudget,
            }))
        .build();
  }

  auto buildStaticPipeline() {
    return cp::StaticPipelineBuilder<
               PipelineHeadHandler,
               PipelineTailHandler,
               TestAllocator>()
        .setEventBase(&eventBase_)
        .setHead(&transport_)
        .setTail(&app_)
        .setAllocator(&allocator_)
        .addNextOutbound<Handler>(
            yielding_srpt_test_tag,
            QuantumFragmentationHandlerConfig{
                .streamQuantum = kQuantum,
                .callbackBudget = kBudget,
            })
        .build();
  }

  void verifyHookWiring(auto pipeline) {
    transport_.setWriteResult(cp::Result::Backpressure);
    EXPECT_THAT(
        pipeline->fireWrite(makeFrame(FrameType::REQUEST_RESPONSE, 1, 250)),
        Eq(cp::Result::Success));
    eventBase_.loopOnce(EVLOOP_NONBLOCK);
    EXPECT_THAT(transport_.writeCount(), Eq(1));
    EXPECT_THAT(pipeline->hasPendingWriteReady(), IsTrue());
    EXPECT_THAT(
        pipeline->fireWrite(makeFrame(FrameType::REQUEST_RESPONSE, 3, 50)),
        Eq(cp::Result::Backpressure));
    EXPECT_THAT(transport_.writeCount(), Eq(1));

    transport_.setWriteResult(cp::Result::Success);
    pipeline->onWriteReady();
    EXPECT_THAT(pipeline->hasPendingWriteReady(), IsFalse());
    EXPECT_THAT(transport_.writeCount(), Eq(1));
    eventBase_.loopOnce(EVLOOP_NONBLOCK);
    EXPECT_THAT(transport_.writeCount(), Eq(3));
  }

  folly::EventBase eventBase_;
  PipelineHeadHandler transport_;
  PipelineTailHandler app_;
  TestAllocator allocator_;
};

TEST_F(
    YieldingSrptFrameFragmentationHandlerPipelineTest,
    DynamicPipelineWiresWriteReadyHook) {
  verifyHookWiring(buildDynamicPipeline());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerPipelineTest,
    StaticPipelineWiresWriteReadyHook) {
  verifyHookWiring(buildStaticPipeline());
}

TEST_F(
    YieldingSrptFrameFragmentationHandlerTest,
    SustainedSingleQuantumTrafficCanStarvePinnedLargeRequest) {
  Handler handler({.streamQuantum = kQuantum, .callbackBudget = kQuantum});
  handler.handlerAdded(context_);
  (void)handler.onWrite(
      context_, makeFrame(FrameType::REQUEST_RESPONSE, 1, 300));

  for (const uint32_t smallStreamId : {3u, 5u, 7u}) {
    (void)handler.onWrite(
        context_,
        makeFrame(FrameType::REQUEST_RESPONSE, smallStreamId, kQuantum));
    runLoop();
  }

  EXPECT_THAT(streamIds(context_.written()), ElementsAre(3, 5, 7));
  EXPECT_THAT(handler.hasPendingStream(1), IsTrue());
}

} // namespace
} // namespace apache::thrift::fast_thrift::frame::write::handler
