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

#include <memory>
#include <utility>

#include <folly/Function.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/ProducerHeadAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/ProducerTailAdapter.h>

namespace folly {
class EventBase;
} // namespace folly

namespace apache::thrift::fast_thrift::thrift::stream {

// The producing-end sub-pipeline's fixed endpoints. The adapters are templated
// on the context only for testability; the pipeline always instantiates them
// with the concrete ContextImpl, so these aliases are ordinary concrete types.
using ProducerHead = ProducerHeadAdapter<channel_pipeline::detail::ContextImpl>;
using ProducerTail = ProducerTailAdapter<channel_pipeline::detail::ContextImpl>;

/**
 * ProducerPipeline — one established stream's producing-end sub-pipeline,
 * bundling the pipeline with the endpoints and allocator it borrows.
 *
 * Built only through [[ProducerPipeline::Builder]], which the framework
 * constructs with the endpoints, allocator, executor, and output sink already
 * wired; the application only composes its producer handlers between them. The
 * owning thrift pipeline holds the returned ProducerPipeline for the stream's
 * lifetime and injects demand at the head via `fireRead`.
 *
 * Ownership / lifetime: the pipeline borrows raw pointers to the endpoints and
 * allocator, so it must be destroyed before them. `pipeline_` is declared last
 * so it destructs first. Because `pipeline_` is a DelayedDestruction pointer
 * whose real teardown can be deferred while a callback is in flight, a
 * ProducerPipeline must only be destroyed when its pipeline is quiescent —
 * never from within a pipeline callback (onWrite/onRead/onException/cancel).
 * The owning pipeline tears streams down out-of-band, so this holds.
 */
class ProducerPipeline {
 public:
  class Builder;

  /**
   * The application-supplied callable the framework runs to compose one
   * stream's producer into the Builder. The framework constructs the Builder
   * (wiring the endpoints/allocator/executor/sink), runs the ConfigFunc to add
   * the application's producer handlers, then calls build().
   */
  using ConfigFunc = folly::Function<void(Builder&)>;

  ProducerPipeline(ProducerPipeline&&) noexcept = default;
  ProducerPipeline& operator=(ProducerPipeline&&) noexcept = default;
  ProducerPipeline(const ProducerPipeline&) = delete;
  ProducerPipeline& operator=(const ProducerPipeline&) = delete;
  ~ProducerPipeline() = default;

  /**
   * Inject inbound demand (RequestN/Cancel) at the head; it travels head->tail
   * to the producer, whose output returns through the head's sink.
   */
  channel_pipeline::Result fireRead(
      channel_pipeline::TypeErasedBox&& msg) noexcept {
    return pipeline_->fireRead(std::move(msg));
  }

 private:
  friend class Builder;

  ProducerPipeline(
      std::unique_ptr<ProducerHead> head,
      std::unique_ptr<ProducerTail> tail,
      std::unique_ptr<channel_pipeline::SimpleBufferAllocator> allocator,
      channel_pipeline::PipelineImpl::Ptr pipeline) noexcept
      : head_(std::move(head)),
        tail_(std::move(tail)),
        allocator_(std::move(allocator)),
        pipeline_(std::move(pipeline)) {}

  std::unique_ptr<ProducerHead> head_;
  std::unique_ptr<ProducerTail> tail_;
  std::unique_ptr<channel_pipeline::SimpleBufferAllocator> allocator_;
  // Declared last: borrows the endpoints/allocator above, so it destructs
  // first.
  channel_pipeline::PipelineImpl::Ptr pipeline_;
};

/**
 * ProducerPipeline::Builder — a restricted, untemplated view over
 * PipelineBuilder for composing one stream's producing-end sub-pipeline.
 *
 * The framework constructs it with the executor and the head's output sink; the
 * Builder creates and owns the fixed head/tail endpoints (the head wired to
 * that sink) and the sub-pipeline's own allocator, and wires them into the
 * underlying PipelineBuilder. The endpoint/allocator/executor/sink setters are
 * therefore hidden — the application only adds its producer handlers via
 * addNext{Inbound,Outbound,Duplex} and then `build()` hands back a
 * ProducerPipeline that owns all four pieces.
 */
class ProducerPipeline::Builder {
 public:
  Builder(folly::EventBase* executor, StreamSink sink)
      : head_(std::make_unique<ProducerHead>(std::move(sink))),
        tail_(std::make_unique<ProducerTail>()),
        allocator_(
            std::make_unique<channel_pipeline::SimpleBufferAllocator>()) {
    builder_.setEventBase(executor)
        .setHead(head_.get())
        .setTail(tail_.get())
        .setAllocator(allocator_.get());
  }

  template <typename H, typename... Args>
  Builder& addNextInbound(Args&&... args) {
    builder_.template addNextInbound<H>(std::forward<Args>(args)...);
    return *this;
  }

  template <typename H, typename... Args>
  Builder& addNextOutbound(Args&&... args) {
    builder_.template addNextOutbound<H>(std::forward<Args>(args)...);
    return *this;
  }

  template <typename H, typename... Args>
  Builder& addNextDuplex(Args&&... args) {
    builder_.template addNextDuplex<H>(std::forward<Args>(args)...);
    return *this;
  }

  ProducerPipeline build() {
    auto pipeline = builder_.build();
    return ProducerPipeline(
        std::move(head_),
        std::move(tail_),
        std::move(allocator_),
        std::move(pipeline));
  }

 private:
  std::unique_ptr<ProducerHead> head_;
  std::unique_ptr<ProducerTail> tail_;
  std::unique_ptr<channel_pipeline::SimpleBufferAllocator> allocator_;
  channel_pipeline::PipelineBuilder<
      ProducerHead,
      ProducerTail,
      channel_pipeline::SimpleBufferAllocator>
      builder_;
};

} // namespace apache::thrift::fast_thrift::thrift::stream
