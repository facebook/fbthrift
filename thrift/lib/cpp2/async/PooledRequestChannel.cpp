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
};
} // namespace

folly::EventBase& PooledRequestChannel::getEvb(const RpcOptions& options) {
  InteractionState* state;
  if (options.getInteractionId()) {
    state = reinterpret_cast<InteractionState*>(options.getInteractionId());
  } else if (options.getInteractionCreate()) {
    state = reinterpret_cast<InteractionState*>(
        options.getInteractionCreate()->get_interactionId());
  } else {
    auto executor = executor_.lock();
    if (!executor) {
      throw std::logic_error("IO executor already destroyed.");
    }
    return *executor->getEventBase();
  }
  return *state->keepAlive.get();
}

uint16_t PooledRequestChannel::getProtocolId() {
  folly::call_once(protocolIdInitFlag_, [&] {
    auto& evb = getEvb({});
    evb.runImmediatelyOrRunInEventBaseThreadAndWait(
        [&] { protocolId_ = impl(evb).getProtocolId(); });
  });

  return protocolId_;
}

template <typename SendFunc>
void PooledRequestChannel::sendRequestImpl(
    SendFunc&& sendFunc,
    folly::EventBase& evb) {
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
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<false>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  sendRequestImpl(
      [options,
       methodNameStr = methodName.str(),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        channel.sendRequestResponse(
            options,
            methodNameStr,
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestNoResponse(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<true>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  sendRequestImpl(
      [options,
       methodNameStr = methodName.str(),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        channel.sendRequestNoResponse(
            options,
            methodNameStr,
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestStream(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    StreamClientCallback* cob) {
  sendRequestImpl(
      [options,
       methodNameStr = methodName.str(),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        channel.sendRequestStream(
            options, methodNameStr, std::move(request), std::move(header), cob);
      },
      getEvb(options));
}

void PooledRequestChannel::sendRequestSink(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* cob) {
  sendRequestImpl(
      [options,
       methodNameStr = methodName.str(),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        channel.sendRequestSink(
            options, methodNameStr, std::move(request), std::move(header), cob);
      },
      getEvb(options));
}

int64_t PooledRequestChannel::getNextInteractionId() {
  auto& evb = getEvb({});

  if (folly::kIsDebug) {
    sendRequestImpl(
        [](Impl& channel) mutable {
          CHECK_EQ(channel.getNextInteractionId(), 0)
              << "Cannot nest multiple channels opinionated about interaction id";
        },
        evb);
  }

  return reinterpret_cast<int64_t>(
      new InteractionState{getKeepAliveToken(evb)});
}

void PooledRequestChannel::terminateInteraction(int64_t id) {
  std::unique_ptr<InteractionState> state(
      reinterpret_cast<InteractionState*>(id));
  std::move(state->keepAlive).add([id, implPtr = impl_](auto&& keepAlive) {
    auto* channel = implPtr->get(*keepAlive);
    if (channel) {
      channel->terminateInteraction(id);
    }
    // channel is only null if nothing was ever sent on that evb,
    // in which case server doesn't know about this interaction
  });
}

PooledRequestChannel::Impl& PooledRequestChannel::impl(folly::EventBase& evb) {
  DCHECK(evb.inRunningEventBaseThread());

  return impl_->getOrCreateFn(evb, [this, &evb] { return implCreator_(evb); });
}
} // namespace thrift
} // namespace apache
