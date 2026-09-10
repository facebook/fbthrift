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

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/Executor.h>
#include <folly/Portability.h>
#include <folly/Synchronized.h>
#include <folly/container/F14Map.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Event.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::fast_thrift::thrift {

/**
 * ThriftServerAppAdapter — base class for generated fast server handlers.
 *
 * The generated <Service>AppAdapter extends this class, inheriting pipeline
 * integration, request dispatch, and the single writeResponse write entry
 * point. Payload construction lives in ResponsePayloads.h — callers build a
 * ThriftServerResponseMessage there, then hand it to writeResponse().
 *
 * Satisfies TailEndpointHandler concept (onRead / onException) and the
 * ThriftAppAdapter concept consumed by ThriftServerCompositeAppAdapter.
 *
 * The adapter holds no terminal-phase state of its own. Close coordination
 * (drain, in-flight reap, deferred closeCallback) is owned by the
 * pipeline-resident ThriftServerConnectionCloseHandler; the adapter just
 * emits the CloseConnection event from close() and fires the user
 * closeCallback in response to the inbound ConnectionClosed event.
 *
 * Lifetime: upper-layer owners must hold the adapter Ptr until
 * closeCallback fires. After that, the adapter may persist independently
 * of the pipeline until in-flight FastHandlerCallbacks drain — each one
 * holds a DestructorGuard on the adapter, so the underlying object stays
 * valid even if the upper-layer Ptr is dropped. Once the pipeline is
 * gone (signaled to the adapter via ConnectionClosed), pipelineActive_
 * is false and straggler writeResponse calls are silently dropped.
 *
 * Threading: when a CPU executor is attached, the generated service path runs
 * off the connection's EventBase. Two refcounts reachable from a request
 * are deliberately non-atomic — this adapter's DelayedDestruction
 * guardCount_, and the ThriftConnContext refcount held via
 * ThriftRequestContext. Both are per-connection, so concurrent requests
 * would contend on them.
 *
 * The invariant that keeps that sound: *both are mutated only on this
 * adapter's EventBase.* Three things uphold it, and all three must survive
 * refactoring together —
 *   1. FastHandlerCallback is constructed on the EventBase, before any
 *      offload, so acquiring the adapter guard happens there.
 *   2. FastHandlerCallback releases on the EventBase, hopping if needed,
 *      which covers both its adapter guard and its request context.
 *   3. Moving a request context never touches the refcount, so building a
 *      response off-EventBase is safe.
 * Serialized responses return through writeResponse, which performs the hop
 * back to the EventBase.
 */
class ThriftServerAppAdapter : public folly::DelayedDestruction {
 public:
  // Canonical owning-pointer alias. Adapters are folly::DelayedDestruction
  // objects, so they can't be deleted with plain delete. Every site that
  // owns an adapter (FastThriftServer's per-connection state, factory
  // returns, composite children) should use this alias.
  using Ptr = std::
      unique_ptr<ThriftServerAppAdapter, folly::DelayedDestruction::Destructor>;

  // Per-method handler signature for SINGLE_REQUEST_SINGLE_RESPONSE RPCs.
  // The dispatcher pre-extracts the components codegen actually needs from
  // the inbound frame so the handler doesn't have to reach back into
  // ThriftServerRequestMessage / ParsedFrame.
  //   streamId       — Rocket stream id; echo into the response so the client
  //                    can match it to the in-flight request.
  //   data           — request payload IOBuf, ready to feed into a protocol
  //                    reader for argument deserialization.
  //   protocol       — wire protocol id (Binary / Compact / ...) for the
  //                    codec.
  //   requestContext — per-request context stamped by
  //                    ThriftServerRequestContextHandler; ownership moves into
  //                    the FastHandlerCallback so it outlives async handlers.
  using RequestResponseProcessFn = void (*)(
      ThriftServerAppAdapter*,
      uint32_t streamId,
      std::unique_ptr<folly::IOBuf> data,
      apache::thrift::ProtocolId protocol,
      std::unique_ptr<ThriftRequestContext> requestContext) noexcept;

  class ResolvedMethod {
   public:
    FOLLY_ALWAYS_INLINE channel_pipeline::Result onRead(
        channel_pipeline::detail::ContextImpl&,
        channel_pipeline::TypeErasedBox&& msg) const noexcept;

   private:
    ResolvedMethod(
        ThriftServerAppAdapter* owner, RequestResponseProcessFn method) noexcept
        : owner_(owner), method_(method) {}

    ThriftServerAppAdapter* owner_;
    RequestResponseProcessFn method_;

    friend class ThriftServerAppAdapter;
  };

  ThriftServerAppAdapter() = default;
  ThriftServerAppAdapter(ThriftServerAppAdapter&&) = delete;
  ThriftServerAppAdapter& operator=(ThriftServerAppAdapter&&) = delete;

  void setCloseCallback(std::function<void()> cb);

  void setPipeline(channel_pipeline::PipelineImpl* pipeline) noexcept;

  // Attach the executor that generated dispatchers use for request
  // deserialization, method execution, and response serialization. When
  // unset, the service path stays inline on the connection's EventBase.
  //
  // Must be called before the connection starts reading — the executor is
  // read without synchronization on the dispatch path.
  void setCPUExecutor(folly::Executor::KeepAlive<> executor) noexcept {
    cpuExecutor_ = std::move(executor);
  }

  // Drops the pipeline reference and releases the DestructorGuard taken in
  // setPipeline. Must be called before the adapter is destroyed.
  //
  // EventBase-only: it mutates guardCount_ and clears evb_, both of which
  // in-flight off-EventBase work depends on. See the threading note above.
  void resetPipeline() noexcept;

  channel_pipeline::PipelineImpl* pipeline() const noexcept {
    return pipeline_;
  }

  // === TailEndpointHandler interface ===

  // Unresolved entry point, used when this adapter is the pipeline tail on
  // its own. Resolves the method against dispatch_ and forwards to the
  // resolved overload below.
  channel_pipeline::Result onRead(
      channel_pipeline::detail::ContextImpl&,
      channel_pipeline::TypeErasedBox&& msg) noexcept;

  void onException(folly::exception_wrapper&& e) noexcept;

  void handlerAdded() noexcept {}
  void handlerRemoved() noexcept {}
  void onPipelineActive() noexcept {}
  void onPipelineInactive() noexcept {}

  using PublishedEvents =
      channel_pipeline::Events<ThriftServerCloseConnectionEvent>;
  using SubscribedEvents =
      channel_pipeline::Events<ThriftServerConnectionClosedEvent>;

  template <channel_pipeline::PipelineEvent E>
    requires std::same_as<E, ThriftServerConnectionClosedEvent>
  void on() noexcept {
    onConnectionClosed();
  }
  void onWriteReady() noexcept {}

  // Write entry point for callers that may be off the EventBase.
  //
  // `adapterGuard` must be a guard the caller already owns — typically the
  // one a FastHandlerCallback took at construction time. It is *moved*, never
  // constructed here: constructing a DestructorGuard bumps this adapter's
  // non-atomic guardCount_, which is only safe on the EventBase, whereas
  // moving one just transfers a pointer. Ownership rides into the hop so the
  // adapter cannot be destroyed before the write runs, and the guard is
  // released on the EventBase.
  //
  // Build the message via the free functions in
  // thrift/server/util/ResponsePayloads.h (makeResponseMessage,
  // makeErrorMessage, makeFrameworkErrorMessage, makeUnknownExceptionMessage,
  // makeSuccessResponseMessage<>, makeDeclaredExceptionMessage<>, ...).
  void writeResponse(
      ThriftServerResponseMessage&& message,
      folly::DelayedDestruction::DestructorGuard&& adapterGuard) noexcept;

  // Write entry point for callers already on the EventBase, which therefore
  // need no guard to survive a hop. DCHECKs the thread.
  void writeResponse(ThriftServerResponseMessage&& message) noexcept;

  // Initiate connection close. Internally fires a
  // ThriftServerCloseConnectionEvent pipeline event; the
  // pipeline-resident ThriftServerConnectionCloseHandler picks it up and runs
  // the terminal state machine (drain timeout → reap → force-close on stuck
  // handler callbacks). The user closeCallback fires when the connection has
  // fully settled. No-op if the pipeline is not yet wired.
  void close() noexcept;

  // Methods registered via addMethodHandler, each paired with a resolved method
  // that binds this adapter to its typed process function. The composite copies
  // that two-pointer value so the child does not repeat the method-name lookup.
  std::vector<std::pair<std::string_view, ResolvedMethod>>
  methodTable() noexcept;

 protected:
  void addMethodHandler(
      std::string_view name, RequestResponseProcessFn handler);

  channel_pipeline::PipelineImpl* pipeline_{nullptr};
  // Keeps pipeline_ alive while pipelineActive_ is true. Released on
  // ConnectionClosed (or in resetPipeline) so the pipeline can die
  // independently of any straggler FastHandlerCallbacks. Straggler
  // writes are gated by pipelineActive_ and never touch pipeline_
  // after release.
  std::unique_ptr<folly::DelayedDestruction::DestructorGuard> pipelineGuard_;
  // EVB-only. Set true in setPipeline(), cleared on ConnectionClosed
  // (and on adapter destruction via resetPipeline). When false,
  // writeResponseOnEventBase drops without touching pipeline_.
  bool pipelineActive_{false};

  ~ThriftServerAppAdapter() override;

  folly::EventBase* getEventBase() const { return evb_.get(); }

  // Executor for the generated service path, or null when it should run
  // inline on the EventBase.
  folly::Executor* cpuExecutor() const noexcept { return cpuExecutor_.get(); }

 private:
  FOLLY_NOINLINE void handleWrongRpcKind(
      uint32_t streamId, apache::thrift::RpcKind kind) noexcept;
  FOLLY_NOINLINE void handleUnknownMethod(
      uint32_t streamId, std::string_view methodName) noexcept;
  FOLLY_NOINLINE void handleMissingPipeline() noexcept;

  void onConnectionClosed() noexcept;

  // Must be called on evb_.
  void writeResponseOnEventBase(ThriftServerResponseMessage&& message) noexcept;

  folly::Executor::KeepAlive<folly::EventBase> evb_{};
  // Null unless the server was configured with a CPU executor. Written once
  // at wiring time, read on every dispatch.
  folly::Executor::KeepAlive<> cpuExecutor_{};
  folly::F14FastMap<std::string, RequestResponseProcessFn> dispatch_;
  folly::Synchronized<std::function<void()>> closeCallback_;

  void fireCloseCallback() noexcept;
};

FOLLY_ALWAYS_INLINE channel_pipeline::Result
ThriftServerAppAdapter::ResolvedMethod::onRead(
    channel_pipeline::detail::ContextImpl&,
    channel_pipeline::TypeErasedBox&& msg) const noexcept {
  auto request = msg.take<ThriftServerRequestMessage>();
  DCHECK(request.streamId != 0) << "Invalid stream ID";
  DCHECK(owner_ != nullptr);
  DCHECK(method_ != nullptr);

  auto& inbound = request.payload;
  if (FOLLY_UNLIKELY(!inbound.is<ThriftRequestResponsePayload>())) {
    // Result::Error propagates to TransportHandler, which closes the
    // connection.
    return channel_pipeline::Result::Error;
  }
  auto& requestResponse = inbound.get<ThriftRequestResponsePayload>();
  DCHECK(requestResponse.metadata != nullptr);
  const auto protocol = requestResponse.metadata->protocol().value_or(0);
  method_(
      owner_,
      request.streamId,
      std::move(requestResponse.data),
      protocol,
      std::move(request.requestContext));
  return channel_pipeline::Result::Success;
}

} // namespace apache::thrift::fast_thrift::thrift
