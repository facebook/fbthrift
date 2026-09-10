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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/ThriftServerAppAdapter.h>

#include <utility>

#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Event.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ServerAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponsePayloads.h>

namespace apache::thrift::fast_thrift::thrift {

static_assert(
    ServerInboundAppAdapter<ThriftServerAppAdapter>,
    "ThriftServerAppAdapter must satisfy ServerInboundAppAdapter");
static_assert(
    ServerOutboundAppAdapter<ThriftServerAppAdapter>,
    "ThriftServerAppAdapter must satisfy ServerOutboundAppAdapter");
static_assert(
    ServerComposableAppAdapter<ThriftServerAppAdapter>,
    "ThriftServerAppAdapter must satisfy ServerComposableAppAdapter");

ThriftServerAppAdapter::~ThriftServerAppAdapter() {
  resetPipeline();
  auto cb = closeCallback_.withWLock([](auto& fn) { return std::move(fn); });
  if (cb) {
    cb();
  }
}

void ThriftServerAppAdapter::setCloseCallback(std::function<void()> cb) {
  closeCallback_.withWLock([&](auto& fn) { fn = std::move(cb); });
}

void ThriftServerAppAdapter::setPipeline(
    channel_pipeline::PipelineImpl* pipeline) noexcept {
  DCHECK(pipeline);
  pipeline_ = pipeline;
  pipelineGuard_ =
      std::make_unique<folly::DelayedDestruction::DestructorGuard>(pipeline_);
  pipelineActive_ = true;
  evb_ = folly::getKeepAliveToken(pipeline_->eventBase());
}

void ThriftServerAppAdapter::resetPipeline() noexcept {
  folly::DelayedDestruction::DestructorGuard dg(this);
  pipelineActive_ = false;
  pipeline_ = nullptr;
  pipelineGuard_.reset();
  evb_ = {};
}

void ThriftServerAppAdapter::onConnectionClosed() noexcept {
  // Detach from pipeline. Stragglers (FastHandlerCallbacks still
  // alive, each holding a DG on us) call writeResponse, hit
  // pipelineActive_==false in writeResponseOnEventBase, and drop
  // silently. The pipeline is free to die — its DG count from us
  // just dropped.
  pipelineActive_ = false;
  pipelineGuard_.reset();
  fireCloseCallback();
}

void ThriftServerAppAdapter::fireCloseCallback() noexcept {
  auto cb = closeCallback_.withWLock([](auto& fn) { return std::move(fn); });
  if (cb && evb_) {
    evb_->runInEventBaseThread(std::move(cb));
  }
}

channel_pipeline::Result ThriftServerAppAdapter::onRead(
    channel_pipeline::detail::ContextImpl& ctx,
    channel_pipeline::TypeErasedBox&& msg) noexcept {
  const auto& request = msg.get<ThriftServerRequestMessage>();
  DCHECK(request.streamId != 0) << "Invalid stream ID";

  const auto routing = getServerRequestRoutingMetadata(request);
  if (FOLLY_UNLIKELY(
          routing.status == ServerRequestRoutingStatus::MissingMetadata)) {
    handleUnknownMethod(request.streamId, {});
    return channel_pipeline::Result::Success;
  }
  if (FOLLY_UNLIKELY(
          routing.status == ServerRequestRoutingStatus::UnsupportedRpcKind)) {
    handleWrongRpcKind(request.streamId, routing.kind);
    return channel_pipeline::Result::Success;
  }

  auto it = dispatch_.find(routing.methodName);
  if (FOLLY_UNLIKELY(it == dispatch_.end())) {
    handleUnknownMethod(request.streamId, routing.methodName);
    return channel_pipeline::Result::Success;
  }
  return ResolvedMethod(this, it->second).onRead(ctx, std::move(msg));
}

void ThriftServerAppAdapter::onException(
    folly::exception_wrapper&& e) noexcept {
  // Tail-of-pipeline contract: any exception reaching here is unhandled.
  // The pipeline is in a broken state — no point waiting for a graceful
  // drain (close() would). Tear it down immediately. PipelineImpl's
  // deactivate() is idempotent (state-gated), so racing with a
  // transport-initiated deactivate is safe — only the first call
  // cascades; the close handler's onPipelineInactive does the rest.
  XLOG_EVERY_MS(ERR, 60'000) << "Pipeline exception: " << e.what();
  if (pipeline_) {
    pipeline_->deactivate();
  }
}

void ThriftServerAppAdapter::addMethodHandler(
    std::string_view name, RequestResponseProcessFn handler) {
  dispatch_[std::string(name)] = handler;
}

std::vector<std::pair<std::string_view, ThriftServerAppAdapter::ResolvedMethod>>
ThriftServerAppAdapter::methodTable() noexcept {
  std::vector<std::pair<std::string_view, ResolvedMethod>> table;
  table.reserve(dispatch_.size());
  for (const auto& [name, handler] : dispatch_) {
    table.emplace_back(name, ResolvedMethod(this, handler));
  }
  return table;
}

void ThriftServerAppAdapter::handleWrongRpcKind(
    uint32_t streamId, apache::thrift::RpcKind kind) noexcept {
  XLOG(ERR) << "Unsupported RPC kind: " << static_cast<int>(kind);
  writeResponseOnEventBase(makeWrongRpcKindMessage(streamId, kind));
}

void ThriftServerAppAdapter::handleUnknownMethod(
    uint32_t streamId, std::string_view methodName) noexcept {
  writeResponseOnEventBase(makeUnknownMethodMessage(streamId, methodName));
}

void ThriftServerAppAdapter::writeResponse(
    ThriftServerResponseMessage&& message,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard) noexcept {
  if (evb_->isInEventBaseThread()) {
    writeResponseOnEventBase(std::move(message));
    return;
  }
  // adapterGuard is moved, not copied: a copy would construct a new guard on
  // this thread and mutate the non-atomic guardCount_ off the EventBase. The
  // move keeps us alive across the hop and releases on the EventBase.
  evb_->runInEventBaseThread([this,
                              adapterGuard = std::move(adapterGuard),
                              message = std::move(message)]() mutable {
    writeResponseOnEventBase(std::move(message));
  });
}

void ThriftServerAppAdapter::writeResponse(
    ThriftServerResponseMessage&& message) noexcept {
  DCHECK(evb_->isInEventBaseThread())
      << "writeResponse without a guard is EventBase-only; off-thread callers "
         "must donate a DestructorGuard so the adapter survives the hop";
  writeResponseOnEventBase(std::move(message));
}

void ThriftServerAppAdapter::writeResponseOnEventBase(
    ThriftServerResponseMessage&& message) noexcept {
  // Straggler after force-close (or any post-ConnectionClosed write):
  // the pipeline may already be gone. Drop silently — pipeline_ may
  // be dangling, do NOT dereference.
  if (FOLLY_UNLIKELY(!pipelineActive_)) {
    return;
  }
  if (FOLLY_UNLIKELY(!pipeline_)) {
    handleMissingPipeline();
    return;
  }
  auto result =
      pipeline_->fireWrite(channel_pipeline::erase_and_box(std::move(message)));
  // A failed write means the response can't reach the wire — pipeline
  // is in a broken or torn-down state. Tear down immediately rather
  // than wait for a graceful drain. Idempotent at the pipeline level.
  if (FOLLY_UNLIKELY(result == channel_pipeline::Result::Error)) {
    pipeline_->deactivate();
  }
}

void ThriftServerAppAdapter::handleMissingPipeline() noexcept {
  XLOG(ERR) << "Pipeline not set, cannot send response";
}

void ThriftServerAppAdapter::close() noexcept {
  if (!pipeline_) {
    return;
  }
  PublishedEvents::template fire<ThriftServerCloseConnectionEvent>(
      *pipeline_, ThriftServerCloseConnectionEvent{});
}

} // namespace apache::thrift::fast_thrift::thrift
