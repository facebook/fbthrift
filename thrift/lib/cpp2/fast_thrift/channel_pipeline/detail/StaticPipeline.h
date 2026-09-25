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

#pragma once

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/EndpointAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/StaticHandler.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace apache::thrift::fast_thrift::channel_pipeline::detail {

enum class StaticHandlerDirection { Inbound, Outbound, Duplex };

template <StaticHandlerDirection Direction, typename H, typename Context>
concept StaticHandlerForDirection =
    (Direction == StaticHandlerDirection::Inbound &&
     InboundHandler<H, Context>) ||
    (Direction == StaticHandlerDirection::Outbound &&
     OutboundHandler<H, Context>) ||
    (Direction == StaticHandlerDirection::Duplex && DuplexHandler<H, Context>);

template <typename H, HandlerId Id, StaticHandlerDirection Direction>
struct StaticHandlerSpec {
  template <typename>
  using Handler = H;
  static constexpr HandlerId id = Id;
  static constexpr StaticHandlerDirection direction = Direction;
};

template <
    template <typename> typename H,
    HandlerId Id,
    StaticHandlerDirection Direction>
struct StaticHandlerTemplateSpec {
  template <typename Context>
  using Handler = H<Context>;
  static constexpr HandlerId id = Id;
  static constexpr StaticHandlerDirection direction = Direction;
};

template <
    typename H,
    HandlerId Id,
    StaticHandlerDirection Direction,
    typename... Args>
class StaticHandlerFactory {
 public:
  using Spec = StaticHandlerSpec<H, Id, Direction>;

  template <typename... Input>
    requires(sizeof...(Input) == sizeof...(Args))
  explicit StaticHandlerFactory(Input&&... args)
      : args_(std::forward<Input>(args)...) {}

  ~StaticHandlerFactory() = default;
  StaticHandlerFactory(StaticHandlerFactory&&) noexcept = default;
  StaticHandlerFactory& operator=(StaticHandlerFactory&&) noexcept = default;
  StaticHandlerFactory(const StaticHandlerFactory&) = delete;
  StaticHandlerFactory& operator=(const StaticHandlerFactory&) = delete;

  operator H() && {
    return std::apply(
        []<typename... StoredArgs>(StoredArgs&&... args) -> H {
          return H(std::forward<StoredArgs>(args)...);
        },
        std::move(args_));
  }

 private:
  std::tuple<Args...> args_;
};

template <
    template <typename> typename H,
    HandlerId Id,
    StaticHandlerDirection Direction,
    typename... Args>
class StaticHandlerTemplateFactory {
 public:
  using Spec = StaticHandlerTemplateSpec<H, Id, Direction>;

  template <typename... Input>
    requires(sizeof...(Input) == sizeof...(Args))
  explicit StaticHandlerTemplateFactory(Input&&... args)
      : args_(std::forward<Input>(args)...) {}

  ~StaticHandlerTemplateFactory() = default;
  StaticHandlerTemplateFactory(StaticHandlerTemplateFactory&&) noexcept =
      default;
  StaticHandlerTemplateFactory& operator=(
      StaticHandlerTemplateFactory&&) noexcept = default;
  StaticHandlerTemplateFactory(const StaticHandlerTemplateFactory&) = delete;
  StaticHandlerTemplateFactory& operator=(const StaticHandlerTemplateFactory&) =
      delete;

  template <typename Context>
  operator H<Context>() && {
    return std::apply(
        []<typename... StoredArgs>(StoredArgs&&... args) -> H<Context> {
          return H<Context>(std::forward<StoredArgs>(args)...);
        },
        std::move(args_));
  }

 private:
  std::tuple<Args...> args_;
};

template <
    typename Pipeline,
    std::size_t Index,
    typename StateTuple,
    typename... Specs>
class StaticHandlerStorage;

template <typename Pipeline, std::size_t Index, typename StateTuple>
class StaticHandlerStorage<Pipeline, Index, StateTuple> {
 public:
  template <typename FactoryTuple>
  StaticHandlerStorage(Pipeline*, FactoryTuple&) noexcept {}
};

template <typename H, typename Context>
concept StaticHandlerLifecycle = HandlerLifecycle<H, Context>;

template <
    typename Pipeline,
    std::size_t Index,
    typename StateTuple,
    typename Spec>
using StaticHandlerSlot = StaticHandler<
    typename Spec::template Handler<
        StaticContext<Pipeline, Index, Spec::id, StateTuple>>,
    Spec::id,
    Pipeline,
    Index,
    StateTuple>;

template <
    typename Pipeline,
    std::size_t Index,
    typename StateTuple,
    typename Spec,
    typename... Rest>
class StaticHandlerStorage<Pipeline, Index, StateTuple, Spec, Rest...>
    : private StaticHandlerSlot<Pipeline, Index, StateTuple, Spec>,
      private StaticHandlerStorage<Pipeline, Index + 1, StateTuple, Rest...> {
  using Slot = StaticHandlerSlot<Pipeline, Index, StateTuple, Spec>;
  using H = typename Slot::Handler;
  using Context = typename Slot::Context;
  using Tail = StaticHandlerStorage<Pipeline, Index + 1, StateTuple, Rest...>;

  static_assert(StaticHandlerLifecycle<H, Context>);
  static_assert(
      StaticHandlerForDirection<Spec::direction, H, Context>,
      "static pipeline handler does not satisfy its declared direction");

 public:
  template <typename FactoryTuple>
  StaticHandlerStorage(Pipeline* pipeline, FactoryTuple& factories)
      : Slot(pipeline, std::move(std::get<Index>(factories))),
        Tail(pipeline, factories) {}

  template <std::size_t Target>
  decltype(auto) get() noexcept {
    if constexpr (Target == Index) {
      return static_cast<Slot&>(*this);
    } else {
      return Tail::template get<Target>();
    }
  }

  template <std::size_t Target>
  decltype(auto) get() const noexcept {
    if constexpr (Target == Index) {
      return static_cast<const Slot&>(*this);
    } else {
      return Tail::template get<Target>();
    }
  }
};

template <typename EventSet, typename E>
inline constexpr bool kStaticEventSetContains = false;

template <PipelineEvent E, PipelineEvent... Evs>
inline constexpr bool kStaticEventSetContains<Events<Evs...>, E> =
    (std::same_as<E, Evs> || ...);

template <typename Pipeline, typename EventSet>
class StaticEventPublisherHandle;

template <typename Pipeline, PipelineEvent... Evs>
class StaticEventPublisherHandle<Pipeline, Events<Evs...>> {
 public:
  explicit StaticEventPublisherHandle(Pipeline& pipeline) noexcept
      : pipeline_(&pipeline) {}

  template <PipelineEvent E>
    requires(
        (std::same_as<E, Evs> || ...) && std::is_void_v<typename E::Payload>)
  void fire() const noexcept {
    pipeline_->template fireEvent<E>();
  }

  template <PipelineEvent E>
    requires(
        (std::same_as<E, Evs> || ...) && (!std::is_void_v<typename E::Payload>))
  void fire(const typename E::Payload& payload) const noexcept {
    pipeline_->template fireEvent<E>(payload);
  }

 private:
  Pipeline* pipeline_;
};

template <
    typename HeadHandler,
    typename TailHandler,
    typename Allocator,
    typename StateTuple,
    typename... Specs>
class StaticPipelineImpl final {
 public:
  using Self = StaticPipelineImpl;
  static constexpr std::size_t kHandlerCount = sizeof...(Specs);
  using EndpointContext = StaticContext<Self, kHandlerCount, 0, StateTuple>;

  class Guard {
   public:
    explicit Guard(Self* pipeline) noexcept : pipeline_(pipeline) {
      if (pipeline_ != nullptr) {
        ++pipeline_->guardCount_;
      }
    }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&& other) noexcept
        : pipeline_(std::exchange(other.pipeline_, nullptr)) {}
    Guard& operator=(Guard&&) = delete;
    ~Guard() {
      if (pipeline_ != nullptr && --pipeline_->guardCount_ == 0 &&
          pipeline_->destroyPending_) {
        pipeline_->destroyNow();
      }
    }

   private:
    Self* pipeline_;
  };

  struct Deleter {
    void operator()(Self* pipeline) const noexcept {
      if (pipeline != nullptr) {
        pipeline->destroy();
      }
    }
  };
  using Ptr = std::unique_ptr<Self, Deleter>;

  template <typename FactoryTuple>
  StaticPipelineImpl(
      folly::EventBase* eventBase,
      HeadHandler* headHandler,
      TailHandler* tailHandler,
      Allocator* allocator,
      StateTuple&& state,
      FactoryTuple& factories)
      : eventBase_(eventBase),
        headHandler_(headHandler),
        tailHandler_(tailHandler),
        allocator_(allocator),
        stateStorage_(std::move(state)),
        handlers_(this, factories),
        endpointContext_(this) {
    static_assert(ValidEndpointPair<HeadHandler, TailHandler, EndpointContext>);
    initializeHooks(std::make_index_sequence<kHandlerCount>{});
  }

  StaticPipelineImpl(const StaticPipelineImpl&) = delete;
  StaticPipelineImpl& operator=(const StaticPipelineImpl&) = delete;
  StaticPipelineImpl(StaticPipelineImpl&&) = delete;
  StaticPipelineImpl& operator=(StaticPipelineImpl&&) = delete;

  void finishBuild() noexcept { callHandlerAdded(); }

  void activate() noexcept {
    Guard guard(this);
    if (state_ != State::Inactive) {
      return;
    }
    state_ = State::Active;
    headHandler_->onPipelineActive();
    activateHandlers(std::make_index_sequence<kHandlerCount>{});
    tailHandler_->onPipelineActive();
  }

  void deactivate() noexcept {
    Guard guard(this);
    deactivateImpl();
  }

  Result fireRead(TypeErasedBox&& msg) noexcept {
    if (FOLLY_UNLIKELY(isClosed())) {
      return Result::Error;
    }
    Guard guard(this);
    return fireReadFrom<0>(std::move(msg));
  }

  Result fireWrite(TypeErasedBox&& msg) noexcept {
    if (FOLLY_UNLIKELY(isClosed())) {
      return Result::Error;
    }
    Guard guard(this);
    return fireWriteFrom<kHandlerCount>(std::move(msg));
  }

  void fireException(folly::exception_wrapper&& e) noexcept {
    if (FOLLY_UNLIKELY(isClosed())) {
      return;
    }
    Guard guard(this);
    fireExceptionFrom<0>(std::move(e));
  }

  template <std::size_t Index>
  Result fireReadFrom(TypeErasedBox&& msg) noexcept {
    if constexpr (Index == kHandlerCount) {
      return tailHandler_->onRead(endpointContext_, std::move(msg));
    } else {
      auto& slot = handlerAt<Index>();
      using H = typename std::remove_reference_t<decltype(slot)>::Handler;
      using Context = typename std::remove_reference_t<decltype(slot)>::Context;
      if constexpr (InboundHandler<H, Context>) {
        return slot.handler().onRead(slot.context(), std::move(msg));
      } else {
        return fireReadFrom<Index + 1>(std::move(msg));
      }
    }
  }

  template <std::size_t Count>
  Result fireWriteFrom(TypeErasedBox&& msg) noexcept {
    if constexpr (Count == 0) {
      return headHandler_->onWrite(endpointContext_, std::move(msg));
    } else {
      auto& slot = handlerAt<Count - 1>();
      using H = typename std::remove_reference_t<decltype(slot)>::Handler;
      using Context = typename std::remove_reference_t<decltype(slot)>::Context;
      if constexpr (OutboundHandler<H, Context>) {
        return slot.handler().onWrite(slot.context(), std::move(msg));
      } else {
        return fireWriteFrom<Count - 1>(std::move(msg));
      }
    }
  }

  template <std::size_t Index>
  void fireExceptionFrom(folly::exception_wrapper&& e) noexcept {
    if constexpr (Index == kHandlerCount) {
      tailHandler_->onException(std::move(e));
    } else {
      auto& slot = handlerAt<Index>();
      using H = typename std::remove_reference_t<decltype(slot)>::Handler;
      using Context = typename std::remove_reference_t<decltype(slot)>::Context;
      if constexpr (InboundHandler<H, Context>) {
        slot.handler().onException(slot.context(), std::move(e));
      } else {
        fireExceptionFrom<Index + 1>(std::move(e));
      }
    }
  }

  template <std::size_t Count>
  void deactivateFrom() noexcept {
    if constexpr (Count > 0) {
      auto& slot = handlerAt<Count - 1>();
      using H = typename std::remove_reference_t<decltype(slot)>::Handler;
      using Context = typename std::remove_reference_t<decltype(slot)>::Context;
      if constexpr (requires(H& handler, Context& context) {
                      {
                        handler.onPipelineInactive(context)
                      } noexcept -> std::same_as<void>;
                    }) {
        slot.handler().onPipelineInactive(slot.context());
      }
      deactivateFrom<Count - 1>();
    }
  }

  Result sendRead(HandlerId id, TypeErasedBox&& msg) noexcept {
    Guard guard(this);
    return sendReadAt<0>(id, std::move(msg));
  }
  Result sendWrite(HandlerId id, TypeErasedBox&& msg) noexcept {
    Guard guard(this);
    return sendWriteAt<0>(id, std::move(msg));
  }
  void sendException(HandlerId id, folly::exception_wrapper&& e) noexcept {
    Guard guard(this);
    sendExceptionAt<0>(id, std::move(e));
  }

  void close() noexcept {
    Guard guard(this);
    if (isClosed()) {
      return;
    }
    const bool wasActive = state_ == State::Active;
    state_ = State::Closing;
    if (wasActive) {
      tailHandler_->onPipelineInactive();
      deactivateFrom<kHandlerCount>();
      headHandler_->onPipelineInactive();
    }
    state_ = State::Closed;
    writeReadyList_.clear();
    readReadyList_.clear();
    callHandlerRemovedImpl();
  }

  bool isClosed() const noexcept {
    return state_ == State::Closing || state_ == State::Closed;
  }
  static constexpr std::size_t handlerCount() noexcept { return kHandlerCount; }
  bool hasPendingWriteReady() const noexcept {
    return !writeReadyList_.empty();
  }
  bool hasPendingReadReady() const noexcept { return !readReadyList_.empty(); }

  folly::EventBase* eventBase() const noexcept { return eventBase_; }
  BytesPtr allocate(std::size_t size) noexcept {
    return allocator_->allocate(size);
  }
  BytesPtr copyBuffer(const void* data, std::size_t size) noexcept {
    return allocator_->copyBuffer(data, size);
  }

  template <typename T>
    requires(StaticTupleTypeCount<StateTuple, T>::value == 1)
  T& state() noexcept {
    return std::get<T>(stateStorage_);
  }
  template <typename T>
    requires(StaticTupleTypeCount<StateTuple, T>::value == 1)
  const T& state() const noexcept {
    return std::get<T>(stateStorage_);
  }

  template <HandlerId Id>
  auto* context(HandlerTag<Id>) noexcept {
    constexpr auto index = handlerIndex<Id, 0>();
    static_assert(index < kHandlerCount, "handler ID is not in this pipeline");
    return &handlerAt<index>().context();
  }

  template <HandlerId Id>
  decltype(auto) handler(HandlerTag<Id>) noexcept {
    constexpr auto index = handlerIndex<Id, 0>();
    static_assert(index < kHandlerCount, "handler ID is not in this pipeline");
    return handlerAt<index>().handler();
  }

  template <typename EventSet>
    requires kIsEventSet<EventSet>
  auto bindEvents() noexcept {
    return StaticEventPublisherHandle<Self, EventSet>{*this};
  }

  BoundEventRoute bindEvent(EventKey key) noexcept {
    return BoundEventRoute{key};
  }

  void fireBoundEvent(BoundEventRoute route, const void* payload) noexcept {
    if (isClosed()) {
      return;
    }
    Guard guard(this);
    const auto key = static_cast<EventKey>(route.value);
    dispatchEndpointEventByKey(*tailHandler_, key, payload);
    dispatchHandlerEventsReverseByKey(
        key, payload, std::make_index_sequence<kHandlerCount>{});
    dispatchEndpointEventByKey(*headHandler_, key, payload);
  }
  template <PipelineEvent E>
    requires std::is_void_v<typename E::Payload>
  void fireEvent() noexcept {
    fireEventImpl<E>(nullptr);
  }
  template <PipelineEvent E>
    requires(!std::is_void_v<typename E::Payload>)
  void fireEvent(const typename E::Payload& payload) noexcept {
    fireEventImpl<E>(&payload);
  }

  template <
      std::size_t PublisherIndex,
      PipelineEvent E,
      std::size_t RouteIndex,
      typename... Args>
  void firePublishedEvent(Args&&... args) noexcept {
    using Slot = std::remove_reference_t<decltype(handlerAt<PublisherIndex>())>;
    using PublishedEvents = typename Slot::Handler::PublishedEvents;
    static_assert(kStaticEventSetContains<PublishedEvents, E>);
    static_assert(RouteIndex == PublishedEvents::template index<E>);
    fireEvent<E>(std::forward<Args>(args)...);
  }

  template <std::size_t Index>
  void awaitWriteReady() noexcept {
    if (isClosed()) {
      return;
    }
    auto* hook = writeReadyHook<Index>();
    if (hook != nullptr && !hook->hook.is_linked()) {
      writeReadyList_.push_back(*hook);
    }
  }
  template <std::size_t Index>
  void cancelAwaitWriteReady() noexcept {
    auto* hook = writeReadyHook<Index>();
    if (hook != nullptr && hook->hook.is_linked()) {
      hook->hook.unlink();
    }
  }
  template <std::size_t Index>
  bool isAwaitingWriteReady() const noexcept {
    auto* hook = const_cast<Self*>(this)->template writeReadyHook<Index>();
    return hook != nullptr && hook->hook.is_linked();
  }
  template <std::size_t Index>
  void awaitReadReady() noexcept {
    if (isClosed()) {
      return;
    }
    auto* hook = readReadyHook<Index>();
    if (hook != nullptr && !hook->hook.is_linked()) {
      readReadyList_.push_back(*hook);
    }
  }
  template <std::size_t Index>
  void cancelAwaitReadReady() noexcept {
    auto* hook = readReadyHook<Index>();
    if (hook != nullptr && hook->hook.is_linked()) {
      hook->hook.unlink();
    }
  }
  template <std::size_t Index>
  bool isAwaitingReadReady() const noexcept {
    auto* hook = const_cast<Self*>(this)->template readReadyHook<Index>();
    return hook != nullptr && hook->hook.is_linked();
  }

  void onWriteReady() noexcept {
    Guard guard(this);
    if (isClosed()) {
      return;
    }
    if (writeReadyDispatching_) {
      writeReadyDispatchPending_ = true;
      return;
    }
    writeReadyDispatching_ = true;
    do {
      bool blocked = false;
      writeReadyDispatchPending_ = false;
      const auto generation = ++writeReadyGeneration_;
      while (true) {
        WriteReadyHook* next = nullptr;
        for (auto& hook : writeReadyList_) {
          if (hook.lastNotifiedGeneration != generation) {
            next = &hook;
            break;
          }
        }
        if (next == nullptr) {
          break;
        }
        next->lastNotifiedGeneration = generation;
        if (next->handlerIndex == kHandlerCount) {
          if constexpr (requires(HeadHandler& h, EndpointContext& ctx) {
                          h.onWriteReady(ctx);
                        }) {
            headHandler_->onWriteReady(endpointContext_);
          }
        } else {
          dispatchWriteReady<0>(next->handlerIndex);
        }
        if (isClosed() || headWriteReadyHook_.hook.is_linked()) {
          blocked = true;
          break;
        }
      }
      if (!blocked) {
        tailHandler_->onWriteReady();
      }
    } while (writeReadyDispatchPending_ && !isClosed());
    writeReadyDispatching_ = false;
  }

  void onReadReady() noexcept {
    Guard guard(this);
    if (isClosed()) {
      return;
    }
    if (readReadyDispatching_) {
      readReadyDispatchPending_ = true;
      return;
    }
    readReadyDispatching_ = true;
    do {
      readReadyDispatchPending_ = false;
      const auto generation = ++readReadyGeneration_;
      while (true) {
        ReadReadyHook* next = nullptr;
        for (auto& hook : readReadyList_) {
          if (hook.lastNotifiedGeneration != generation) {
            next = &hook;
            break;
          }
        }
        if (next == nullptr) {
          break;
        }
        next->lastNotifiedGeneration = generation;
        dispatchReadReady<0>(next->handlerIndex);
        if (isClosed()) {
          break;
        }
      }
      if (!isClosed()) {
        headHandler_->onReadReady();
      }
    } while (readReadyDispatchPending_ && !isClosed());
    readReadyDispatching_ = false;
  }

 private:
  enum class State : std::uint8_t { Inactive, Active, Closing, Closed };

  ~StaticPipelineImpl() {
    if (state_ != State::Closed) {
      deactivateImpl();
      state_ = State::Closed;
      writeReadyList_.clear();
      readReadyList_.clear();
      callHandlerRemovedImpl();
    }
  }

  void deactivateImpl() noexcept {
    if (state_ != State::Active) {
      return;
    }
    state_ = State::Inactive;
    tailHandler_->onPipelineInactive();
    deactivateFrom<kHandlerCount>();
    headHandler_->onPipelineInactive();
  }

  void destroy() noexcept {
    if (guardCount_ != 0) {
      destroyPending_ = true;
      return;
    }
    destroyNow();
  }
  void destroyNow() noexcept { delete this; }

  template <std::size_t Index>
  decltype(auto) handlerAt() noexcept {
    return handlers_.template get<Index>();
  }
  template <std::size_t Index>
  decltype(auto) handlerAt() const noexcept {
    return handlers_.template get<Index>();
  }

  template <std::size_t Index>
  Result sendReadAt(HandlerId id, TypeErasedBox&& msg) noexcept {
    if constexpr (Index == kHandlerCount) {
      return Result::Error;
    } else {
      using Spec = std::tuple_element_t<Index, std::tuple<Specs...>>;
      if (id == Spec::id) {
        return fireReadFrom<Index>(std::move(msg));
      }
      return sendReadAt<Index + 1>(id, std::move(msg));
    }
  }
  template <std::size_t Index>
  Result sendWriteAt(HandlerId id, TypeErasedBox&& msg) noexcept {
    if constexpr (Index == kHandlerCount) {
      return Result::Error;
    } else {
      using Spec = std::tuple_element_t<Index, std::tuple<Specs...>>;
      if (id == Spec::id) {
        return fireWriteFrom<Index + 1>(std::move(msg));
      }
      return sendWriteAt<Index + 1>(id, std::move(msg));
    }
  }
  template <std::size_t Index>
  void sendExceptionAt(HandlerId id, folly::exception_wrapper&& e) noexcept {
    if constexpr (Index < kHandlerCount) {
      using Spec = std::tuple_element_t<Index, std::tuple<Specs...>>;
      if (id == Spec::id) {
        fireExceptionFrom<Index>(std::move(e));
        return;
      }
      sendExceptionAt<Index + 1>(id, std::move(e));
    }
  }

  template <std::size_t... Index>
  void activateHandlers(std::index_sequence<Index...>) noexcept {
    (activateHandler<Index>(), ...);
  }
  template <std::size_t Index>
  void activateHandler() noexcept {
    auto& slot = handlerAt<Index>();
    using H = typename std::remove_reference_t<decltype(slot)>::Handler;
    using Context = typename std::remove_reference_t<decltype(slot)>::Context;
    if constexpr (requires(H& handler, Context& context) {
                    {
                      handler.onPipelineActive(context)
                    } noexcept -> std::same_as<void>;
                  }) {
      slot.handler().onPipelineActive(slot.context());
    }
  }

  void callHandlerAdded() noexcept {
    Guard guard(this);
    headHandler_->handlerAdded();
    callHandlerAddedImpl(std::make_index_sequence<kHandlerCount>{});
    tailHandler_->handlerAdded();
  }
  template <std::size_t... Index>
  void callHandlerAddedImpl(std::index_sequence<Index...>) noexcept {
    (handlerAt<Index>().handler().handlerAdded(handlerAt<Index>().context()),
     ...);
  }
  void callHandlerRemovedImpl() noexcept {
    tailHandler_->handlerRemoved();
    removeHandlers<kHandlerCount>();
    headHandler_->handlerRemoved();
  }
  template <std::size_t Count>
  void removeHandlers() noexcept {
    if constexpr (Count > 0) {
      auto& slot = handlerAt<Count - 1>();
      slot.handler().handlerRemoved(slot.context());
      removeHandlers<Count - 1>();
    }
  }

  template <std::size_t... Index>
  void initializeHooks(std::index_sequence<Index...>) noexcept {
    headWriteReadyHook_.handlerIndex = kHandlerCount;
    (initializeHook<Index>(), ...);
  }
  template <std::size_t Index>
  void initializeHook() noexcept {
    auto& slot = handlerAt<Index>();
    if (auto* hook = slot.writeReadyHook()) {
      hook->handlerIndex = Index;
    }
    if (auto* hook = slot.readReadyHook()) {
      hook->handlerIndex = Index;
    }
  }
  template <std::size_t Index>
  WriteReadyHook* writeReadyHook() noexcept {
    if constexpr (Index == kHandlerCount) {
      return &headWriteReadyHook_;
    } else {
      return handlerAt<Index>().writeReadyHook();
    }
  }
  template <std::size_t Index>
  ReadReadyHook* readReadyHook() noexcept {
    if constexpr (Index == kHandlerCount) {
      return nullptr;
    } else {
      return handlerAt<Index>().readReadyHook();
    }
  }

  template <std::size_t Index>
  void dispatchWriteReady(std::size_t target) noexcept {
    if constexpr (Index < kHandlerCount) {
      if (target == Index) {
        auto& slot = handlerAt<Index>();
        using H = typename std::remove_reference_t<decltype(slot)>::Handler;
        using Context =
            typename std::remove_reference_t<decltype(slot)>::Context;
        if constexpr (OutboundHandler<H, Context>) {
          slot.handler().onWriteReady(slot.context());
        }
        return;
      }
      dispatchWriteReady<Index + 1>(target);
    }
  }
  template <std::size_t Index>
  void dispatchReadReady(std::size_t target) noexcept {
    if constexpr (Index < kHandlerCount) {
      if (target == Index) {
        auto& slot = handlerAt<Index>();
        using H = typename std::remove_reference_t<decltype(slot)>::Handler;
        using Context =
            typename std::remove_reference_t<decltype(slot)>::Context;
        if constexpr (InboundHandler<H, Context>) {
          slot.handler().onReadReady(slot.context());
        }
        return;
      }
      dispatchReadReady<Index + 1>(target);
    }
  }

  template <PipelineEvent E>
  void fireEventImpl(const void* payload) noexcept {
    if (isClosed()) {
      return;
    }
    Guard guard(this);
    dispatchEndpointEvent<E>(*tailHandler_, payload);
    dispatchHandlerEventsReverse<E>(
        payload, std::make_index_sequence<kHandlerCount>{});
    dispatchEndpointEvent<E>(*headHandler_, payload);
  }
  template <PipelineEvent E, typename H>
  static void dispatchEndpointEvent(H& handler, const void* payload) noexcept {
    if constexpr (requires { typename H::SubscribedEvents; }) {
      if constexpr (kStaticEventSetContains<typename H::SubscribedEvents, E>) {
        static_assert(EndpointTypeEventSubscriber<H>);
        if constexpr (std::is_void_v<typename E::Payload>) {
          handler.template on<E>();
        } else {
          handler.template on<E>(
              *static_cast<const typename E::Payload*>(payload));
        }
      }
    }
  }
  template <PipelineEvent E, std::size_t Index>
  void dispatchHandlerEvent(const void* payload) noexcept {
    auto& slot = handlerAt<Index>();
    using Slot = std::remove_reference_t<decltype(slot)>;
    using H = typename Slot::Handler;
    using Context = typename Slot::Context;
    if constexpr (requires { typename H::SubscribedEvents; }) {
      if constexpr (kStaticEventSetContains<typename H::SubscribedEvents, E>) {
        static_assert(TypeEventSubscriber<H, Context>);
        if constexpr (std::is_void_v<typename E::Payload>) {
          slot.handler().template on<E>(slot.context());
        } else {
          slot.handler().template on<E>(
              slot.context(),
              *static_cast<const typename E::Payload*>(payload));
        }
      }
    }
  }
  template <PipelineEvent E, std::size_t... Index>
  void dispatchHandlerEventsReverse(
      const void* payload, std::index_sequence<Index...>) noexcept {
    (dispatchHandlerEvent<E, kHandlerCount - 1 - Index>(payload), ...);
  }
  template <typename H, PipelineEvent... Evs>
  static void dispatchEndpointEventSetByKey(
      H& handler, EventKey key, const void* payload, Events<Evs...>) noexcept {
    static_cast<void>(
        ((key == eventKey<Evs>()
              ? (dispatchEndpointEvent<Evs>(handler, payload), true)
              : false) ||
         ...));
  }

  template <typename H>
  static void dispatchEndpointEventByKey(
      H& handler, EventKey key, const void* payload) noexcept {
    if constexpr (requires { typename H::SubscribedEvents; }) {
      dispatchEndpointEventSetByKey(
          handler, key, payload, typename H::SubscribedEvents{});
    }
  }

  template <std::size_t Index, PipelineEvent... Evs>
  void dispatchHandlerEventSetByKey(
      EventKey key, const void* payload, Events<Evs...>) noexcept {
    static_cast<void>(
        ((key == eventKey<Evs>()
              ? (dispatchHandlerEvent<Evs, Index>(payload), true)
              : false) ||
         ...));
  }

  template <std::size_t Index>
  void dispatchHandlerEventByKey(EventKey key, const void* payload) noexcept {
    auto& slot = handlerAt<Index>();
    using H = typename std::remove_reference_t<decltype(slot)>::Handler;
    if constexpr (requires { typename H::SubscribedEvents; }) {
      dispatchHandlerEventSetByKey<Index>(
          key, payload, typename H::SubscribedEvents{});
    }
  }

  template <std::size_t... Index>
  void dispatchHandlerEventsReverseByKey(
      EventKey key,
      const void* payload,
      std::index_sequence<Index...>) noexcept {
    (dispatchHandlerEventByKey<kHandlerCount - 1 - Index>(key, payload), ...);
  }

  template <HandlerId Id, std::size_t Index>
  static consteval std::size_t handlerIndex() {
    if constexpr (Index == kHandlerCount) {
      return kHandlerCount;
    } else {
      using Spec = std::tuple_element_t<Index, std::tuple<Specs...>>;
      if constexpr (Spec::id == Id) {
        return Index;
      } else {
        return handlerIndex<Id, Index + 1>();
      }
    }
  }

  folly::EventBase* eventBase_;
  HeadHandler* headHandler_;
  TailHandler* tailHandler_;
  Allocator* allocator_;
  StateTuple stateStorage_;
  StaticHandlerStorage<Self, 0, StateTuple, Specs...> handlers_;
  EndpointContext endpointContext_;
  State state_{State::Inactive};
  WriteReadyList writeReadyList_;
  ReadReadyList readReadyList_;
  WriteReadyHook headWriteReadyHook_;
  std::size_t writeReadyGeneration_{0};
  std::size_t readReadyGeneration_{0};
  std::size_t guardCount_{0};
  bool writeReadyDispatching_{false};
  bool writeReadyDispatchPending_{false};
  bool readReadyDispatching_{false};
  bool readReadyDispatchPending_{false};
  bool destroyPending_{false};
};

} // namespace apache::thrift::fast_thrift::channel_pipeline::detail
