/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/lib/cpp2/async/PooledRequestChannel.h>

#include <thrift/lib/cpp2/async/FutureRequest.h>

#include <folly/futures/Future.h>

namespace apache {
namespace thrift {
namespace {
struct InteractionState {
  folly::Executor::KeepAlive<folly::EventBase> keepAlive;
  apache::thrift::ManagedStringView name;
  InteractionId id;
};

static void maybeCreateInteraction(
    const RpcOptions& options, PooledRequestChannel::Impl& channel) {
  if (auto id = options.getInteractionId()) {
    auto* state = reinterpret_cast<InteractionState*>(id);
    if (!state->id) {
      state->id = channel.registerInteraction(std::move(state->name), id);
    }
  }
}
} // namespace

folly::EventBase& PooledRequestChannel::getEvb(const RpcOptions& options) {
  if (options.getInteractionId()) {
    return *reinterpret_cast<InteractionState*>(options.getInteractionId())
                ->keepAlive.get();
  }

  auto executor = executor_.lock();
  if (!executor) {
    throw std::logic_error("IO executor already destroyed.");
  }
  return *executor->getEventBase();
}

uint16_t PooledRequestChannel::getProtocolId() {
  if (auto value = protocolId_.load(std::memory_order_relaxed)) {
    return value;
  }

  auto& evb = getEvb({});
  evb.runImmediatelyOrRunInEventBaseThreadAndWait([&] {
    protocolId_.store(impl(evb).getProtocolId(), std::memory_order_relaxed);
  });

  return protocolId_.load(std::memory_order_relaxed);
}

template <typename SendFunc>
void PooledRequestChannel::sendRequestImpl(
    SendFunc&& sendFunc, folly::EventBase& evb) {
  evb.runInEventBaseThread(
      [this,
       keepAlive = getKeepAliveToken(evb),
       sendFunc = std::forward<SendFunc>(sendFunc)]() mutable {
        sendFunc(impl(*keepAlive));
      });
}

namespace {
template <bool oneWay>
class ExecutorRequestCallback final : public RequestClientCallback {
 public:
  ExecutorRequestCallback(
      RequestClientCallback::Ptr cb,
      folly::Executor::KeepAlive<> executorKeepAlive)
      : executorKeepAlive_(std::move(executorKeepAlive)), cb_(std::move(cb)) {
    CHECK(executorKeepAlive_);
  }

  void onRequestSent() noexcept override {
    if (oneWay) {
      executorKeepAlive_.get()->add(
          [cb = std::move(cb_)]() mutable { cb.release()->onRequestSent(); });
      delete this;
    } else {
      requestSent_ = true;
    }
  }
  void onResponse(ClientReceiveState&& rs) noexcept override {
    executorKeepAlive_.get()->add([requestSent = requestSent_,
                                   cb = std::move(cb_),
                                   rs = std::move(rs)]() mutable {
      if (requestSent) {
        cb->onRequestSent();
      }
      cb.release()->onResponse(std::move(rs));
    });
    delete this;
  }
  void onResponseError(folly::exception_wrapper ex) noexcept override {
    executorKeepAlive_.get()->add([requestSent = requestSent_,
                                   cb = std::move(cb_),
                                   ex = std::move(ex)]() mutable {
      if (requestSent) {
        cb->onRequestSent();
      }
      cb.release()->onResponseError(std::move(ex));
    });
    delete this;
  }

 private:
  bool requestSent_{false};
  folly::Executor::KeepAlive<> executorKeepAlive_;
  RequestClientCallback::Ptr cb_;
};
} // namespace

void PooledRequestChannel::sendRequestResponse(
    const RpcOptions& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<false>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  sendRequestImpl(
      [options,
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestResponse(
            options,
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestNoResponse(
    const RpcOptions& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<true>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  sendRequestImpl(
      [options,
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestNoResponse(
            options,
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestStream(
    const RpcOptions& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    StreamClientCallback* cob) {
  sendRequestImpl(
      [options,
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestStream(
            options,
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            cob);
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestSink(
    const RpcOptions& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* cob) {
  sendRequestImpl(
      [options,
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestSink(
            options,
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            cob);
      },
      getEvb(options));
}

InteractionId PooledRequestChannel::createInteraction(
    ManagedStringView&& name) {
  CHECK(!name.view().empty());
  auto& evb = getEvb({});
  return createInteractionId(reinterpret_cast<int64_t>(
      new InteractionState{getKeepAliveToken(evb), std::move(name), {}}));
}

void PooledRequestChannel::terminateInteraction(InteractionId idWrapper) {
  int64_t id = idWrapper;
  releaseInteractionId(std::move(idWrapper));
  std::unique_ptr<InteractionState> state(
      reinterpret_cast<InteractionState*>(id));
  std::move(state->keepAlive)
      .add([id = std::move(state->id),
            implPtr = impl_](auto&& keepAlive) mutable {
        auto* channel = implPtr->get(*keepAlive);
        if (channel) {
          (*channel)->terminateInteraction(std::move(id));
        } else {
          // channel is only null if nothing was ever sent on that evb,
          // in which case server doesn't know about this interaction
          DCHECK(!id);
        }
      });
}

PooledRequestChannel::Impl& PooledRequestChannel::impl(folly::EventBase& evb) {
  DCHECK(evb.inRunningEventBaseThread());

  return *impl_->try_emplace_with(evb, [this, &evb] {
    auto ptr = implCreator_(evb);
    DCHECK(!!ptr);
    return ptr;
  });
}
} // namespace thrift
} // namespace apache
