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

#include <folly/CPortability.h>
#include <folly/ExceptionWrapper.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineRef.h>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace apache::thrift::fast_thrift::channel_pipeline::detail {

struct StaticHandlerContextOps {
  HandlerId (*handlerId)(const void*, std::size_t) noexcept;
  std::size_t (*handlerIndex)(const void*, std::size_t) noexcept;
  Result (*fireRead)(void*, std::size_t, TypeErasedBox&&) noexcept;
  Result (*fireWrite)(void*, std::size_t, TypeErasedBox&&) noexcept;
  void (*fireException)(
      void*, std::size_t, folly::exception_wrapper&&) noexcept;
  void (*deactivate)(void*, std::size_t) noexcept;
  void (*fireEvent)(void*, EventKey, const void*) noexcept;
  BytesPtr (*allocate)(void*, std::size_t) noexcept;
  BytesPtr (*copyBuffer)(void*, const void*, std::size_t) noexcept;
  folly::EventBase* (*eventBase)(void*) noexcept;
  PipelineRef (*pipeline)(void*) noexcept;
  void (*close)(void*) noexcept;
  void (*awaitWriteReady)(void*, std::size_t) noexcept;
  void (*cancelAwaitWriteReady)(void*, std::size_t) noexcept;
  bool (*isAwaitingWriteReady)(const void*, std::size_t) noexcept;
  void (*awaitReadReady)(void*, std::size_t) noexcept;
  void (*cancelAwaitReadReady)(void*, std::size_t) noexcept;
  bool (*isAwaitingReadReady)(const void*, std::size_t) noexcept;
};

class StaticHandlerContext {
 public:
  StaticHandlerContext() noexcept = default;

  HandlerId handlerId() const noexcept {
    return ops_->handlerId(pipeline_, index_);
  }
  std::size_t handlerIndex() const noexcept {
    return ops_->handlerIndex(pipeline_, index_);
  }

  void activate() noexcept {}

  FOLLY_ALWAYS_INLINE Result fireRead(TypeErasedBox&& msg) noexcept {
    return ops_->fireRead(pipeline_, index_, std::move(msg));
  }
  FOLLY_ALWAYS_INLINE Result fireWrite(TypeErasedBox&& msg) noexcept {
    return ops_->fireWrite(pipeline_, index_, std::move(msg));
  }
  FOLLY_ALWAYS_INLINE void fireException(
      folly::exception_wrapper&& e) noexcept {
    ops_->fireException(pipeline_, index_, std::move(e));
  }

  template <PipelineEvent E>
    requires std::is_void_v<typename E::Payload>
  void fireEvent() noexcept {
    ops_->fireEvent(pipeline_, eventKey<E>(), nullptr);
  }
  template <PipelineEvent E>
    requires(!std::is_void_v<typename E::Payload>)
  void fireEvent(const typename E::Payload& payload) noexcept {
    ops_->fireEvent(pipeline_, eventKey<E>(), &payload);
  }
  template <PipelineEvent E, std::size_t RouteIndex, typename... Args>
  void firePublishedEvent(Args&&... args) noexcept {
    static_cast<void>(RouteIndex);
    fireEvent<E>(std::forward<Args>(args)...);
  }

  void deactivate() noexcept { ops_->deactivate(pipeline_, index_); }
  PipelineRef pipeline() const noexcept { return ops_->pipeline(pipeline_); }
  BytesPtr allocate(std::size_t size) noexcept {
    return ops_->allocate(pipeline_, size);
  }
  BytesPtr copyBuffer(const void* data, std::size_t size) noexcept {
    return ops_->copyBuffer(pipeline_, data, size);
  }
  folly::EventBase* eventBase() const noexcept {
    return ops_->eventBase(pipeline_);
  }
  void close() noexcept { ops_->close(pipeline_); }

  void awaitWriteReady() noexcept { ops_->awaitWriteReady(pipeline_, index_); }
  void cancelAwaitWriteReady() noexcept {
    ops_->cancelAwaitWriteReady(pipeline_, index_);
  }
  bool isAwaitingWriteReady() const noexcept {
    return ops_->isAwaitingWriteReady(pipeline_, index_);
  }
  void awaitReadReady() noexcept { ops_->awaitReadReady(pipeline_, index_); }
  void cancelAwaitReadReady() noexcept {
    ops_->cancelAwaitReadReady(pipeline_, index_);
  }
  bool isAwaitingReadReady() const noexcept {
    return ops_->isAwaitingReadReady(pipeline_, index_);
  }

  void bind(
      void* pipeline,
      const StaticHandlerContextOps& ops,
      std::size_t index) noexcept {
    pipeline_ = pipeline;
    ops_ = &ops;
    index_ = index;
  }

 private:
  void* pipeline_{nullptr};
  const StaticHandlerContextOps* ops_{nullptr};
  std::size_t index_{0};
};

static_assert(sizeof(StaticHandlerContext) == 3 * sizeof(void*));

} // namespace apache::thrift::fast_thrift::channel_pipeline::detail
