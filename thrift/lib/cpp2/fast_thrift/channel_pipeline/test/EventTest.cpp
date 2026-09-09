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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockHandler.h>

#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::channel_pipeline::test {

namespace {

struct ValueEvent : EventTag<int> {};
struct SignalEvent : EventTag<> {};
struct UnobservedEvent : EventTag<> {};
struct EventState {};

static_assert(PipelineEvent<ValueEvent>);
static_assert(PipelineEvent<SignalEvent>);
static_assert(!PipelineEvent<EventTag<int>>);

class TypeEventHead : public MockHeadHandler {
 public:
  using PublishedEvents = Events<ValueEvent, SignalEvent, UnobservedEvent>;
  using SubscribedEvents = Events<ValueEvent, SignalEvent>;

  explicit TypeEventHead(std::vector<std::string>* order = nullptr) noexcept
      : order_(order) {}

  template <typename E>
    requires std::same_as<E, ValueEvent>
  void on(const int& value) noexcept {
    value_ = value;
  }

  template <typename E>
    requires std::same_as<E, SignalEvent>
  void on() noexcept {
    if (order_) {
      order_->emplace_back("head");
    }
  }

  int value() const noexcept { return value_; }

 private:
  std::vector<std::string>* order_;
  int value_{-1};
};

class TypeEventTail : public MockTailHandler {
 public:
  using SubscribedEvents = Events<SignalEvent>;

  explicit TypeEventTail(std::vector<std::string>* order = nullptr) noexcept
      : order_(order) {}

  template <typename E>
    requires std::same_as<E, SignalEvent>
  void on() noexcept {
    ++signals_;
    if (order_) {
      order_->emplace_back("tail");
    }
  }

  int signals() const noexcept { return signals_; }

 private:
  std::vector<std::string>* order_;
  int signals_{0};
};

class TypeEventHandler : public MockHandler {
 public:
  using SubscribedEvents = Events<ValueEvent, SignalEvent>;

  explicit TypeEventHandler(
      std::string name = {}, std::vector<std::string>* order = nullptr) noexcept
      : name_(std::move(name)), order_(order) {}

  template <typename E, typename Context>
    requires std::same_as<E, ValueEvent>
  void on(Context&, const int& value) noexcept {
    value_ = value;
  }

  template <typename E, typename Context>
    requires std::same_as<E, SignalEvent>
  void on(Context&) noexcept {
    ++signals_;
    if (order_) {
      order_->push_back(name_);
    }
  }

  int value() const noexcept { return value_; }
  int signals() const noexcept { return signals_; }

 private:
  std::string name_;
  std::vector<std::string>* order_;
  int value_{-1};
  int signals_{0};
};

class PublishingTypeEventHandler : public MockHandler {
 public:
  using PublishedEvents = Events<UnobservedEvent, ValueEvent>;

  template <typename Context>
  Result onRead(Context& ctx, TypeErasedBox&& msg) noexcept {
    PublishedEvents::fire<ValueEvent>(ctx, msg.get<int>());
    return ctx.fireRead(std::move(msg));
  }
};

static_assert(TypeEventPublisher<PublishingTypeEventHandler>);

static_assert(TypeEventPublisher<TypeEventHead>);
static_assert(EndpointTypeEventSubscriber<TypeEventHead>);
static_assert(EndpointTypeEventSubscriber<TypeEventTail>);
static_assert(TypeEventSubscriber<TypeEventHandler, detail::ContextImpl>);

struct ForeignEvent : EventTag<> {};

template <typename EventSet, typename E>
concept CanPublishSignal =
    requires(PipelineImpl& pipeline) { EventSet::template fire<E>(pipeline); };

static_assert(CanPublishSignal<TypeEventHead::PublishedEvents, SignalEvent>);
static_assert(!CanPublishSignal<TypeEventHead::PublishedEvents, ForeignEvent>);

HANDLER_TAG(a);
HANDLER_TAG(b);

} // namespace

// =============================================================================
// Type-based event registration
// =============================================================================

TEST(EventTest, TypeEventsDeliverTypedPayloadsAndSignals) {
  folly::EventBase evb;
  TypeEventHead head;
  TypeEventTail tail;
  TestAllocator alloc;

  auto handler = std::make_unique<TypeEventHandler>();
  auto* handlerPtr = handler.get();

  auto pipeline =
      PipelineBuilder<TypeEventHead, TypeEventTail, TestAllocator>()
          .setEventBase(&evb)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&alloc)
          .addNextDuplex<TypeEventHandler>(a_tag, std::move(handler))
          .build();
  auto publisher = pipeline->bindEvents<TypeEventHead::PublishedEvents>();

  publisher.fire<ValueEvent>(42);
  EXPECT_EQ(head.value(), 42);
  EXPECT_EQ(handlerPtr->value(), 42);
  EXPECT_EQ(tail.signals(), 0);

  publisher.fire<SignalEvent>();
  EXPECT_EQ(handlerPtr->signals(), 1);
  EXPECT_EQ(tail.signals(), 1);

  TypeEventHead::PublishedEvents::fire<UnobservedEvent>(*pipeline);
  EXPECT_EQ(handlerPtr->signals(), 1);

  pipeline->close();
  publisher.fire<ValueEvent>(99);
  EXPECT_EQ(handlerPtr->value(), 42);
}

TEST(EventTest, InternalHandlerPublishesThroughItsContext) {
  folly::EventBase evb;
  MockHeadHandler head;
  MockTailHandler tail;
  TestAllocator alloc;

  auto subscriber = std::make_unique<TypeEventHandler>();
  auto* subscriberPtr = subscriber.get();

  auto pipeline =
      PipelineBuilder<MockHeadHandler, MockTailHandler, TestAllocator>()
          .setEventBase(&evb)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&alloc)
          .addNextDuplex<PublishingTypeEventHandler>(a_tag)
          .addNextDuplex<TypeEventHandler>(b_tag, std::move(subscriber))
          .build();

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox(17)), Result::Success);
  EXPECT_EQ(subscriberPtr->value(), 17);
}

TEST(EventTest, InternalHandlerPublishesThroughTypedContext) {
  folly::EventBase evb;
  MockHeadHandler head;
  MockTailHandler tail;
  TestAllocator alloc;

  auto subscriber = std::make_unique<TypeEventHandler>();
  auto* subscriberPtr = subscriber.get();

  auto pipeline =
      PipelineBuilder<MockHeadHandler, MockTailHandler, TestAllocator>()
          .setEventBase(&evb)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&alloc)
          .addState<EventState>()
          .addNextDuplex<PublishingTypeEventHandler>(a_tag)
          .addNextDuplex<TypeEventHandler>(b_tag, std::move(subscriber))
          .build();

  EXPECT_EQ(pipeline->fireRead(TypeErasedBox(23)), Result::Success);
  EXPECT_EQ(subscriberPtr->value(), 23);
}

TEST(EventTest, TypeEventsPreserveSubscriberOrder) {
  folly::EventBase evb;
  TestAllocator alloc;
  std::vector<std::string> order;
  TypeEventHead head{&order};
  TypeEventTail tail{&order};

  auto pipeline =
      PipelineBuilder<TypeEventHead, TypeEventTail, TestAllocator>()
          .setEventBase(&evb)
          .setHead(&head)
          .setTail(&tail)
          .setAllocator(&alloc)
          .addNextDuplex<TypeEventHandler>(
              a_tag, std::make_unique<TypeEventHandler>("a", &order))
          .addNextDuplex<TypeEventHandler>(
              b_tag, std::make_unique<TypeEventHandler>("b", &order))
          .build();

  TypeEventHead::PublishedEvents::fire<SignalEvent>(*pipeline);

  EXPECT_EQ(order, (std::vector<std::string>{"tail", "b", "a", "head"}));
}

} // namespace apache::thrift::fast_thrift::channel_pipeline::test
