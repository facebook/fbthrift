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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/EndpointAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/StaticPipeline.h>

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::channel_pipeline {

namespace detail {
struct StaticPipelineBuilderRebindTag {};

template <HandlerId Id, typename... Entry>
inline constexpr bool kStaticHandlerIdUnique = ((Entry::Spec::id != Id) && ...);
} // namespace detail

/** Builder for a pipeline whose handler types, IDs, and order are fixed. */
template <
    typename HeadHandler,
    typename TailHandler,
    typename Allocator = SimpleBufferAllocator,
    typename StateTuple = std::tuple<>,
    typename StaticHandlerConfig = detail::NoErasedStaticHandlers,
    typename... Entry>
class StaticPipelineBuilder {
  static_assert(BufferAllocator<Allocator>);

  template <typename, typename, typename, typename, typename, typename...>
  friend class StaticPipelineBuilder;

 public:
  StaticPipelineBuilder() = default;
  ~StaticPipelineBuilder() = default;
  StaticPipelineBuilder(const StaticPipelineBuilder&) = delete;
  StaticPipelineBuilder& operator=(const StaticPipelineBuilder&) = delete;
  StaticPipelineBuilder(StaticPipelineBuilder&&) noexcept = default;
  StaticPipelineBuilder& operator=(StaticPipelineBuilder&&) noexcept = default;

  StaticPipelineBuilder& setEventBase(folly::EventBase* eventBase) & noexcept {
    eventBase_ = eventBase;
    return *this;
  }
  StaticPipelineBuilder&& setEventBase(
      folly::EventBase* eventBase) && noexcept {
    eventBase_ = eventBase;
    return std::move(*this);
  }

  StaticPipelineBuilder& setHead(HeadHandler* head) & noexcept {
    headHandler_ = head;
    return *this;
  }
  StaticPipelineBuilder&& setHead(HeadHandler* head) && noexcept {
    headHandler_ = head;
    return std::move(*this);
  }

  StaticPipelineBuilder& setTail(TailHandler* tail) & noexcept {
    tailHandler_ = tail;
    return *this;
  }
  StaticPipelineBuilder&& setTail(TailHandler* tail) && noexcept {
    tailHandler_ = tail;
    return std::move(*this);
  }

  StaticPipelineBuilder& setAllocator(Allocator* allocator) & noexcept {
    allocator_ = allocator;
    return *this;
  }
  StaticPipelineBuilder&& setAllocator(Allocator* allocator) && noexcept {
    allocator_ = allocator;
    return std::move(*this);
  }

  template <typename T, typename... Args>
    requires(sizeof...(Entry) == 0)
  auto addState(Args&&... args) && {
    static_assert(
        std::is_move_constructible_v<T>,
        "StaticPipelineBuilder::addState<T> requires move-constructible T");
    static_assert(
        detail::StaticTupleTypeCount<StateTuple, T>::value == 0,
        "each static pipeline state type may be registered once");
    using NewStateTuple = decltype(std::tuple_cat(
        std::declval<StateTuple&&>(), std::declval<std::tuple<T>>()));
    return StaticPipelineBuilder<
        HeadHandler,
        TailHandler,
        Allocator,
        NewStateTuple,
        StaticHandlerConfig>(
        detail::StaticPipelineBuilderRebindTag{},
        eventBase_,
        headHandler_,
        tailHandler_,
        allocator_,
        std::tuple<>{},
        std::tuple_cat(
            std::move(stateTuple_),
            std::tuple<T>(T(std::forward<Args>(args)...))),
        std::move(staticHandlers_));
  }

  template <typename H, HandlerId Id, typename... Args>
  auto addNextInbound(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandler<detail::StaticHandlerDirection::Inbound, H, Id>(
            std::forward<Args>(args)...);
  }

  template <typename H, HandlerId Id, typename... Args>
  auto addNextOutbound(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandler<detail::StaticHandlerDirection::Outbound, H, Id>(
            std::forward<Args>(args)...);
  }

  template <typename H, HandlerId Id, typename... Args>
  auto addNextDuplex(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandler<detail::StaticHandlerDirection::Duplex, H, Id>(
            std::forward<Args>(args)...);
  }

  template <template <typename> typename H, HandlerId Id, typename... Args>
  auto addNextInboundTemplate(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandlerTemplate<
            detail::StaticHandlerDirection::Inbound,
            H,
            Id>(std::forward<Args>(args)...);
  }

  template <template <typename> typename H, HandlerId Id, typename... Args>
  auto addNextOutboundTemplate(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandlerTemplate<
            detail::StaticHandlerDirection::Outbound,
            H,
            Id>(std::forward<Args>(args)...);
  }

  template <template <typename> typename H, HandlerId Id, typename... Args>
  auto addNextDuplexTemplate(HandlerTag<Id>, Args&&... args) && {
    return std::move(*this)
        .template addHandlerTemplate<
            detail::StaticHandlerDirection::Duplex,
            H,
            Id>(std::forward<Args>(args)...);
  }

  auto addStaticHandlers(std::vector<detail::ErasedStaticHandler> handlers) &&
    requires(!StaticHandlerConfig::enabled)
  {
    if (handlers.empty()) {
      throw std::invalid_argument(
          "StaticPipelineBuilder: static handler list must not be empty");
    }
    using Config = detail::ErasedStaticHandlersAt<sizeof...(Entry)>;
    return StaticPipelineBuilder<
        HeadHandler,
        TailHandler,
        Allocator,
        StateTuple,
        Config,
        Entry...>(
        detail::StaticPipelineBuilderRebindTag{},
        eventBase_,
        headHandler_,
        tailHandler_,
        allocator_,
        std::move(factories_),
        std::move(stateTuple_),
        std::move(handlers));
  }

  auto build() && {
    validateRequired();
    validateHandlerIds();
    using Impl = detail::StaticPipelineImpl<
        HeadHandler,
        TailHandler,
        Allocator,
        StateTuple,
        StaticHandlerConfig,
        typename Entry::Spec...>;
    typename Impl::Ptr pipeline(new Impl(
        eventBase_,
        headHandler_,
        tailHandler_,
        allocator_,
        std::move(stateTuple_),
        factories_,
        std::move(staticHandlers_)));
    pipeline->finishBuild();
    return pipeline;
  }

 private:
  template <
      detail::StaticHandlerDirection Direction,
      typename H,
      HandlerId Id,
      typename... Args>
  auto addHandler(Args&&... args) && {
    static_assert(
        detail::kStaticHandlerIdUnique<Id, Entry...>,
        "static pipeline handler IDs must be unique");
    using NewEntry =
        detail::StaticHandlerFactory<H, Id, Direction, std::decay_t<Args>...>;
    return StaticPipelineBuilder<
        HeadHandler,
        TailHandler,
        Allocator,
        StateTuple,
        StaticHandlerConfig,
        Entry...,
        NewEntry>(
        detail::StaticPipelineBuilderRebindTag{},
        eventBase_,
        headHandler_,
        tailHandler_,
        allocator_,
        std::tuple_cat(
            std::move(factories_),
            std::tuple<NewEntry>(NewEntry(std::forward<Args>(args)...))),
        std::move(stateTuple_),
        std::move(staticHandlers_));
  }

  template <
      detail::StaticHandlerDirection Direction,
      template <typename> typename H,
      HandlerId Id,
      typename... Args>
  auto addHandlerTemplate(Args&&... args) && {
    static_assert(
        detail::kStaticHandlerIdUnique<Id, Entry...>,
        "static pipeline handler IDs must be unique");
    using NewEntry = detail::
        StaticHandlerTemplateFactory<H, Id, Direction, std::decay_t<Args>...>;
    return StaticPipelineBuilder<
        HeadHandler,
        TailHandler,
        Allocator,
        StateTuple,
        StaticHandlerConfig,
        Entry...,
        NewEntry>(
        detail::StaticPipelineBuilderRebindTag{},
        eventBase_,
        headHandler_,
        tailHandler_,
        allocator_,
        std::tuple_cat(
            std::move(factories_),
            std::tuple<NewEntry>(NewEntry(std::forward<Args>(args)...))),
        std::move(stateTuple_),
        std::move(staticHandlers_));
  }

  void validateRequired() const {
    if (eventBase_ == nullptr) {
      throw std::runtime_error("StaticPipelineBuilder: EventBase is required");
    }
    if (headHandler_ == nullptr) {
      throw std::runtime_error("StaticPipelineBuilder: head is required");
    }
    if (tailHandler_ == nullptr) {
      throw std::runtime_error("StaticPipelineBuilder: tail is required");
    }
    if (allocator_ == nullptr) {
      throw std::runtime_error("StaticPipelineBuilder: allocator is required");
    }
  }

  void validateHandlerIds() const {
    for (std::size_t i = 0; i < staticHandlers_.size(); ++i) {
      const auto id = staticHandlers_[i].handlerId();
      if (((Entry::Spec::id == id) || ...)) {
        throw std::invalid_argument(
            "StaticPipelineBuilder: handler IDs must be unique");
      }
      for (std::size_t j = 0; j < i; ++j) {
        if (staticHandlers_[j].handlerId() == id) {
          throw std::invalid_argument(
              "StaticPipelineBuilder: handler IDs must be unique");
        }
      }
    }
  }

  StaticPipelineBuilder(
      detail::StaticPipelineBuilderRebindTag,
      folly::EventBase* eventBase,
      HeadHandler* headHandler,
      TailHandler* tailHandler,
      Allocator* allocator,
      std::tuple<Entry...>&& factories,
      StateTuple&& stateTuple,
      std::vector<detail::ErasedStaticHandler>&& staticHandlers) noexcept
      : eventBase_(eventBase),
        headHandler_(headHandler),
        tailHandler_(tailHandler),
        allocator_(allocator),
        factories_(std::move(factories)),
        stateTuple_(std::move(stateTuple)),
        staticHandlers_(std::move(staticHandlers)) {}

  folly::EventBase* eventBase_{nullptr};
  HeadHandler* headHandler_{nullptr};
  TailHandler* tailHandler_{nullptr};
  Allocator* allocator_{nullptr};
  std::tuple<Entry...> factories_;
  StateTuple stateTuple_;
  std::vector<detail::ErasedStaticHandler> staticHandlers_;
};

} // namespace apache::thrift::fast_thrift::channel_pipeline
