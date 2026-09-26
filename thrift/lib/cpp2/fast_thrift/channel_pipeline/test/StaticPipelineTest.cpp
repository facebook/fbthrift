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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/StaticPipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>

#include <folly/portability/GTest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::channel_pipeline::test {
namespace {

static_assert(!std::is_default_constructible_v<detail::ErasedStaticHandler>);

class StaticHeadHandler {
 public:
  template <typename Context>
  Result onWrite(Context&, TypeErasedBox&&) noexcept {
    ++writes_;
    return Result::Success;
  }
  void onReadReady() noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}
  std::size_t writeCount() const noexcept { return writes_; }

 private:
  std::size_t writes_{0};
};

class StaticTailHandler {
 public:
  template <typename Context>
  Result onRead(Context&, TypeErasedBox&&) noexcept {
    ++reads_;
    return Result::Success;
  }
  void onException(folly::exception_wrapper&&) noexcept { ++exceptions_; }
  void onWriteReady() noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}
  std::size_t readCount() const noexcept { return reads_; }
  std::size_t exceptionCount() const noexcept { return exceptions_; }

 private:
  std::size_t reads_{0};
  std::size_t exceptions_{0};
};

HANDLER_TAG(first_static);
HANDLER_TAG(second_static);
HANDLER_TAG(state_static);
HANDLER_TAG(ready_static);
HANDLER_TAG(publisher_static);
HANDLER_TAG(subscriber_static);
HANDLER_TAG(first_erased_static);
HANDLER_TAG(second_erased_static);
HANDLER_TAG(read_ready_canceller_static);
HANDLER_TAG(read_ready_target_static);
HANDLER_TAG(read_ready_reentrant_static);
HANDLER_TAG(reentrant_close_static);

class TraceHandler {
 public:
  TraceHandler(std::vector<std::string>* trace, std::string name)
      : trace_(trace), name_(std::move(name)) {}
  TraceHandler(const TraceHandler&) = delete;
  TraceHandler& operator=(const TraceHandler&) = delete;
  TraceHandler& operator=(TraceHandler&&) = delete;
  TraceHandler(TraceHandler&&) = delete;

  template <typename Context>
  void handlerAdded(Context&) noexcept {
    trace_->push_back(name_ + ".added");
  }
  template <typename Context>
  void handlerRemoved(Context&) noexcept {
    trace_->push_back(name_ + ".removed");
  }
  template <typename Context>
  void onPipelineActive(Context&) noexcept {
    trace_->push_back(name_ + ".active");
  }
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {
    trace_->push_back(name_ + ".inactive");
  }
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    trace_->push_back(name_ + ".read");
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
    trace_->push_back(name_ + ".write");
    return ctx.fireWrite(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    trace_->push_back(name_ + ".exception");
    ctx.fireException(std::move(e));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <typename Context>
  void onWriteReady(Context&) noexcept {}

 private:
  std::vector<std::string>* trace_;
  std::string name_;
};

class DestructionCountingHandler {
 public:
  explicit DestructionCountingHandler(int* destructions)
      : destructions_(destructions) {}
  ~DestructionCountingHandler() { ++*destructions_; }
  DestructionCountingHandler(const DestructionCountingHandler&) = delete;
  DestructionCountingHandler& operator=(const DestructionCountingHandler&) =
      delete;
  DestructionCountingHandler(DestructionCountingHandler&&) = delete;
  DestructionCountingHandler& operator=(DestructionCountingHandler&&) = delete;

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}

 private:
  int* destructions_;
};

struct PipelineState {
  int reads{0};
};

template <typename Context>
class StatefulHandler {
 public:
  void handlerAdded(Context&) noexcept {}
  void handlerRemoved(Context&) noexcept {}
  void onPipelineActive(Context&) noexcept {}
  void onPipelineInactive(Context&) noexcept {}
  void onReadReady(Context&) noexcept {}
  void onWriteReady(Context&) noexcept {}
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    ++ctx.template state<PipelineState>().reads;
    return ctx.fireRead(std::move(msg));
  }
  Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireWrite(std::move(msg));
  }
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
};

class ReadyHandler {
 public:
  WriteReadyHook writeReadyHook_;

  explicit ReadyHandler(int* calls) : calls_(calls) {}

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onWrite(Context& ctx, TypeErasedBox&&) noexcept {
    ctx.awaitWriteReady();
    return Result::Backpressure;
  }
  template <typename Context>
  void onWriteReady(Context& ctx) noexcept {
    ++*calls_;
    ctx.cancelAwaitWriteReady();
  }

 private:
  int* calls_;
};

struct ValueEvent : EventTag<int> {};

class PublisherHandler {
 public:
  using PublishedEvents = Events<ValueEvent>;

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    PublishedEvents::template fire<ValueEvent>(ctx, msg.template get<int>());
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
};

class SubscriberHandler {
 public:
  using SubscribedEvents = Events<ValueEvent>;

  explicit SubscriberHandler(int* value) : value_(value) {}

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  template <PipelineEvent E, typename Context>
  void on(Context&, const typename E::Payload& value) noexcept {
    static_assert(std::same_as<E, ValueEvent>);
    *value_ = value;
  }

 private:
  int* value_;
};

class IndexRecordingHandler {
 public:
  explicit IndexRecordingHandler(std::size_t* index) : index_(index) {}

  template <typename Context>
  void handlerAdded(Context& ctx) noexcept {
    *index_ = ctx.handlerIndex();
  }
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireWrite(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <typename Context>
  void onWriteReady(Context&) noexcept {}

 private:
  std::size_t* index_;
};

class IndexRecordingTail {
 public:
  explicit IndexRecordingTail(std::size_t* index) : index_(index) {}

  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&&) noexcept {
    *index_ = ctx.handlerIndex();
    return Result::Success;
  }
  void onException(folly::exception_wrapper&&) noexcept {}
  void onWriteReady() noexcept {}
  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}

 private:
  std::size_t* index_;
};

class DeactivateOnReadHandler {
 public:
  explicit DeactivateOnReadHandler(std::vector<std::string>* trace)
      : trace_(trace) {}

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {
    trace_->emplace_back("typed.inactive");
  }
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&&) noexcept {
    trace_->emplace_back("typed.read");
    ctx.deactivate();
    return Result::Success;
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}

 private:
  std::vector<std::string>* trace_;
};

class CountingSubscriber {
 public:
  using SubscribedEvents = Events<ValueEvent>;

  explicit CountingSubscriber(int* calls) : calls_(calls) {}

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <PipelineEvent E, typename Context>
  void on(Context&, const typename E::Payload&) noexcept {
    static_assert(std::same_as<E, ValueEvent>);
    ++*calls_;
  }

 private:
  int* calls_;
};

class ReadReadyCanceller {
 public:
  ReadReadyHook readReadyHook_;

  explicit ReadReadyCanceller(int* calls) : calls_(calls) {}

  template <typename Context>
  void handlerAdded(Context& ctx) noexcept {
    ctx.awaitReadReady();
  }
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onReadReady(Context& ctx) noexcept {
    ++*calls_;
    ctx.pipeline()
        ->context(read_ready_target_static_tag)
        ->cancelAwaitReadReady();
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

 private:
  int* calls_;
};

class ReadReadyTarget {
 public:
  ReadReadyHook readReadyHook_;

  explicit ReadReadyTarget(int* calls) : calls_(calls) {}

  template <typename Context>
  void handlerAdded(Context& ctx) noexcept {
    ctx.awaitReadReady();
  }
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {
    ++*calls_;
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

 private:
  int* calls_;
};

class ReentrantReadReadyHandler {
 public:
  ReadReadyHook readReadyHook_;

  explicit ReentrantReadReadyHandler(int* calls) : calls_(calls) {}

  template <typename Context>
  void handlerAdded(Context& ctx) noexcept {
    ctx.awaitReadReady();
  }
  template <typename Context>
  void handlerRemoved(Context&) noexcept {}
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context&) noexcept {}
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onReadReady(Context& ctx) noexcept {
    ++*calls_;
    if (*calls_ == 1) {
      ctx.pipeline()->onReadReady();
    }
    ctx.cancelAwaitReadReady();
  }
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

 private:
  int* calls_;
};

class ReentrantCloseHandler {
 public:
  ReentrantCloseHandler(int* inactiveCalls, int* removedCalls)
      : inactiveCalls_(inactiveCalls), removedCalls_(removedCalls) {}

  template <typename Context>
  void handlerAdded(Context&) noexcept {}
  template <typename Context>
  void handlerRemoved(Context&) noexcept {
    ++*removedCalls_;
  }
  template <typename Context>
  void onPipelineActive(Context&) noexcept {}
  template <typename Context>
  void onPipelineInactive(Context& ctx) noexcept {
    ++*inactiveCalls_;
    ctx.close();
  }
  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    return ctx.fireRead(std::move(msg));
  }
  template <typename Context>
  void onReadReady(Context&) noexcept {}
  template <typename Context>
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

 private:
  int* inactiveCalls_;
  int* removedCalls_;
};

TEST(StaticPipelineTest, RoutesAndOwnsNonMovableHandlers) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextDuplex<TraceHandler>(first_static_tag, &trace, "first")
          .addNextDuplex<TraceHandler>(second_static_tag, &trace, "second")
          .build();

  static_assert(
      !std::is_polymorphic_v<std::remove_reference_t<decltype(*pipeline)>>);
  EXPECT_EQ(pipeline->handlerCount(), 2);
  pipeline->activate();
  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(
      pipeline->fireWrite(erase_and_box(folly::IOBuf::create(0))),
      Result::Success);
  pipeline->sendException(first_static_tag, {});
  EXPECT_EQ(tail.exceptionCount(), 1);
  pipeline->sendException(fnv1a_hash("missing"), {});
  EXPECT_EQ(tail.exceptionCount(), 1);
  pipeline->close();

  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "first.added",
          "second.added",
          "first.active",
          "second.active",
          "first.read",
          "second.read",
          "second.write",
          "first.write",
          "first.exception",
          "second.exception",
          "second.inactive",
          "first.inactive",
          "second.removed",
          "first.removed"}));
  EXPECT_EQ(tail.readCount(), 1);
  EXPECT_EQ(head.writeCount(), 1);
}

TEST(StaticPipelineTest, DestructionDeactivatesActiveHandlers) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;

  {
    auto pipeline =
        StaticPipelineBuilder<
            StaticHeadHandler,
            StaticTailHandler,
            TestAllocator>()
            .setEventBase(&eventBase)
            .setHead(&head)
            .setTail(&tail)
            .setAllocator(&allocator)
            .addNextDuplex<TraceHandler>(first_static_tag, &trace, "handler")
            .build();
    pipeline->activate();
  }

  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "handler.added",
          "handler.active",
          "handler.inactive",
          "handler.removed"}));
}

TEST(StaticPipelineTest, ReentrantCloseRemovesHandlersOnce) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int inactiveCalls = 0;
  int removedCalls = 0;

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextInbound<ReentrantCloseHandler>(
              reentrant_close_static_tag, &inactiveCalls, &removedCalls)
          .build();

  pipeline->activate();
  pipeline->close();

  EXPECT_EQ(inactiveCalls, 1);
  EXPECT_EQ(removedCalls, 1);
}

TEST(StaticPipelineTest, ReadReadyCallbackCanUnlinkNextHandler) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int cancellerCalls = 0;
  int targetCalls = 0;

  auto pipeline = StaticPipelineBuilder<
                      StaticHeadHandler,
                      StaticTailHandler,
                      TestAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .addNextInbound<ReadReadyCanceller>(
                          read_ready_canceller_static_tag, &cancellerCalls)
                      .addNextInbound<ReadReadyTarget>(
                          read_ready_target_static_tag, &targetCalls)
                      .build();

  pipeline->onReadReady();

  EXPECT_EQ(cancellerCalls, 1);
  EXPECT_EQ(targetCalls, 0);
}

TEST(StaticPipelineTest, DefersReentrantReadReadyDispatch) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int calls = 0;

  auto pipeline = StaticPipelineBuilder<
                      StaticHeadHandler,
                      StaticTailHandler,
                      TestAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .addNextInbound<ReentrantReadReadyHandler>(
                          read_ready_reentrant_static_tag, &calls)
                      .build();

  pipeline->onReadReady();

  EXPECT_EQ(calls, 1);
  EXPECT_FALSE(pipeline->hasPendingReadReady());
}

TEST(StaticPipelineTest, ErasedHandlersDestroyOwnersExactlyOnce) {
  int destructions = 0;
  {
    std::vector<detail::ErasedStaticHandler> handlers;
    handlers.reserve(1);
    handlers.push_back(
        detail::makeErasedStaticHandler<DestructionCountingHandler>(
            first_erased_static_tag.id, &destructions));
    handlers.push_back(
        detail::makeErasedStaticHandler<DestructionCountingHandler>(
            second_erased_static_tag.id, &destructions));
    EXPECT_EQ(destructions, 0);

    auto replacement =
        detail::makeErasedStaticHandler<DestructionCountingHandler>(
            first_static_tag.id, &destructions);
    handlers.front() = std::move(replacement);
    EXPECT_EQ(destructions, 1);
  }
  EXPECT_EQ(destructions, 3);
}

TEST(StaticPipelineTest, SplicesErasedHandlersInOrder) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_erased_static_tag.id, &trace, "erased-first"));
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          second_erased_static_tag.id, &trace, "erased-second"));

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextDuplex<TraceHandler>(first_static_tag, &trace, "typed-first")
          .addStaticHandlers(std::move(erasedHandlers))
          .addNextDuplex<TraceHandler>(
              second_static_tag, &trace, "typed-second")
          .build();

  EXPECT_EQ(pipeline->handlerCount(), 4);
  pipeline->activate();
  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(
      pipeline->fireWrite(erase_and_box(folly::IOBuf::create(0))),
      Result::Success);
  pipeline->close();

  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "typed-first.added",     "erased-first.added",
          "erased-second.added",   "typed-second.added",
          "typed-first.active",    "erased-first.active",
          "erased-second.active",  "typed-second.active",
          "typed-first.read",      "erased-first.read",
          "erased-second.read",    "typed-second.read",
          "typed-second.write",    "erased-second.write",
          "erased-first.write",    "typed-first.write",
          "typed-second.inactive", "erased-second.inactive",
          "erased-first.inactive", "typed-first.inactive",
          "typed-second.removed",  "erased-second.removed",
          "erased-first.removed",  "typed-first.removed"}));
}

TEST(StaticPipelineTest, TargetsHandlersWithoutCrossingSplice) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_erased_static_tag.id, &trace, "erased-first"));
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          second_erased_static_tag.id, &trace, "erased-second"));

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextDuplex<TraceHandler>(first_static_tag, &trace, "typed-first")
          .addStaticHandlers(std::move(erasedHandlers))
          .addNextDuplex<TraceHandler>(
              second_static_tag, &trace, "typed-second")
          .build();

  trace.clear();
  EXPECT_EQ(
      pipeline->sendRead(second_static_tag, TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(trace, (std::vector<std::string>{"typed-second.read"}));

  trace.clear();
  EXPECT_EQ(
      pipeline->sendWrite(
          first_static_tag, erase_and_box(folly::IOBuf::create(0))),
      Result::Success);
  EXPECT_EQ(trace, (std::vector<std::string>{"typed-first.write"}));

  trace.clear();
  pipeline->sendException(second_static_tag, {});
  EXPECT_EQ(trace, (std::vector<std::string>{"typed-second.exception"}));

  trace.clear();
  EXPECT_EQ(
      pipeline->sendRead(first_erased_static_tag, TypeErasedBox{1}),
      Result::Success);
  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "erased-first.read", "erased-second.read", "typed-second.read"}));

  trace.clear();
  EXPECT_EQ(
      pipeline->sendWrite(
          second_erased_static_tag, erase_and_box(folly::IOBuf::create(0))),
      Result::Success);
  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "erased-second.write", "erased-first.write", "typed-first.write"}));

  trace.clear();
  pipeline->sendException(first_erased_static_tag, {});
  EXPECT_EQ(
      trace,
      (std::vector<std::string>{
          "erased-first.exception",
          "erased-second.exception",
          "typed-second.exception"}));
}

TEST(StaticPipelineTest, ReportsLogicalHandlerIndicesAcrossSplice) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  std::size_t tailIndex = 0;
  IndexRecordingTail tail(&tailIndex);
  TestAllocator allocator;
  std::size_t firstIndex = 0;
  std::size_t erasedIndex = 0;
  std::size_t secondIndex = 0;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<IndexRecordingHandler>(
          first_erased_static_tag.id, &erasedIndex));

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          IndexRecordingTail,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextDuplex<IndexRecordingHandler>(first_static_tag, &firstIndex)
          .addStaticHandlers(std::move(erasedHandlers))
          .addNextDuplex<IndexRecordingHandler>(second_static_tag, &secondIndex)
          .build();

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(firstIndex, 0);
  EXPECT_EQ(erasedIndex, 1);
  EXPECT_EQ(secondIndex, 2);
  EXPECT_EQ(tailIndex, 3);
}

TEST(StaticPipelineTest, TypedDeactivateDoesNotCrossSplice) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_erased_static_tag.id, &trace, "erased"));

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextInbound<DeactivateOnReadHandler>(first_static_tag, &trace)
          .addStaticHandlers(std::move(erasedHandlers))
          .build();

  pipeline->activate();
  trace.clear();
  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(trace, (std::vector<std::string>{"typed.read", "typed.inactive"}));
}

TEST(StaticPipelineTest, BoundEventDispatchesErasedSubscribersOnce) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int typedCalls = 0;
  int erasedCalls = 0;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<CountingSubscriber>(
          first_erased_static_tag.id, &erasedCalls));

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextInbound<CountingSubscriber>(first_static_tag, &typedCalls)
          .addStaticHandlers(std::move(erasedHandlers))
          .build();

  auto publisher = pipeline->bindEvents<Events<ValueEvent>>();
  publisher.fire<ValueEvent>(23);
  EXPECT_EQ(typedCalls, 1);
  EXPECT_EQ(erasedCalls, 1);
}

TEST(StaticPipelineTest, RejectsDuplicateHandlerIds) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  std::vector<std::string> trace;

  std::vector<detail::ErasedStaticHandler> duplicateTypedId;
  duplicateTypedId.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_static_tag.id, &trace, "erased"));
  EXPECT_THROW(
      (StaticPipelineBuilder<
           StaticHeadHandler,
           StaticTailHandler,
           TestAllocator>()
           .setEventBase(&eventBase)
           .setHead(&head)
           .setTail(&tail)
           .setAllocator(&allocator)
           .addNextDuplex<TraceHandler>(first_static_tag, &trace, "typed")
           .addStaticHandlers(std::move(duplicateTypedId))
           .build()),
      std::invalid_argument);

  std::vector<detail::ErasedStaticHandler> duplicateErasedIds;
  duplicateErasedIds.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_erased_static_tag.id, &trace, "first"));
  duplicateErasedIds.push_back(
      detail::makeErasedStaticHandler<TraceHandler>(
          first_erased_static_tag.id, &trace, "second"));
  auto builder = StaticPipelineBuilder<
      StaticHeadHandler,
      StaticTailHandler,
      TestAllocator>();
  builder.setEventBase(&eventBase)
      .setHead(&head)
      .setTail(&tail)
      .setAllocator(&allocator);
  EXPECT_THROW(
      std::move(builder)
          .addStaticHandlers(std::move(duplicateErasedIds))
          .build(),
      std::invalid_argument);
}

TEST(StaticPipelineTest, StaticHandlersDispatchReadyAndEvents) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int readyCalls = 0;
  int eventValue = 0;
  std::vector<detail::ErasedStaticHandler> erasedHandlers;
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<ReadyHandler>(
          ready_static_tag.id, &readyCalls));
  erasedHandlers.push_back(
      detail::makeErasedStaticHandler<SubscriberHandler>(
          subscriber_static_tag.id, &eventValue));

  auto pipeline = StaticPipelineBuilder<
                      StaticHeadHandler,
                      StaticTailHandler,
                      TestAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .addNextInbound<PublisherHandler>(publisher_static_tag)
                      .addStaticHandlers(std::move(erasedHandlers))
                      .build();

  EXPECT_EQ(
      pipeline->fireWrite(erase_and_box(folly::IOBuf::create(0))),
      Result::Backpressure);
  EXPECT_TRUE(pipeline->hasPendingWriteReady());
  pipeline->onWriteReady();
  EXPECT_EQ(readyCalls, 1);
  EXPECT_FALSE(pipeline->hasPendingWriteReady());

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{17}), Result::Success);
  EXPECT_EQ(eventValue, 17);

  auto publisher = pipeline->bindEvents<Events<ValueEvent>>();
  publisher.fire<ValueEvent>(23);
  EXPECT_EQ(eventValue, 23);
}

TEST(StaticPipelineTest, ExposesTypedState) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;

  auto builder = StaticPipelineBuilder<
                     StaticHeadHandler,
                     StaticTailHandler,
                     TestAllocator>()
                     .setEventBase(&eventBase)
                     .setHead(&head)
                     .setTail(&tail)
                     .setAllocator(&allocator)
                     .addState<PipelineState>();
  auto pipeline =
      std::move(builder)
          .template addNextDuplexTemplate<StatefulHandler>(state_static_tag)
          .build();

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{1}), Result::Success);
  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{2}), Result::Success);
  EXPECT_EQ(tail.readCount(), 2);
  EXPECT_EQ(pipeline->state<PipelineState>().reads, 2);
}

TEST(StaticPipelineTest, DispatchesReadyAndTypedEvents) {
  folly::EventBase eventBase;
  StaticHeadHandler head;
  StaticTailHandler tail;
  TestAllocator allocator;
  int readyCalls = 0;
  int eventValue = 0;

  auto pipeline =
      StaticPipelineBuilder<
          StaticHeadHandler,
          StaticTailHandler,
          TestAllocator>()
          .setEventBase(&eventBase)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&allocator)
          .addNextOutbound<ReadyHandler>(ready_static_tag, &readyCalls)
          .addNextInbound<PublisherHandler>(publisher_static_tag)
          .addNextInbound<SubscriberHandler>(subscriber_static_tag, &eventValue)
          .build();

  EXPECT_EQ(
      pipeline->fireWrite(erase_and_box(folly::IOBuf::create(0))),
      Result::Backpressure);
  EXPECT_TRUE(pipeline->hasPendingWriteReady());
  pipeline->onWriteReady();
  EXPECT_EQ(readyCalls, 1);
  EXPECT_FALSE(pipeline->hasPendingWriteReady());

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox{17}), Result::Success);
  EXPECT_EQ(eventValue, 17);

  auto publisher = pipeline->bindEvents<Events<ValueEvent>>();
  publisher.fire<ValueEvent>(23);
  EXPECT_EQ(eventValue, 23);
}

} // namespace
} // namespace apache::thrift::fast_thrift::channel_pipeline::test
