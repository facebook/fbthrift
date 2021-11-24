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

folly::Executor::KeepAlive<folly::EventBase> PooledRequestChannel::getEvb(
    const RpcOptions& options) {
  if (options.getInteractionId()) {
    return reinterpret_cast<InteractionState*>(options.getInteractionId())
        ->keepAlive;
  }

  auto evb = getEventBase_();
  if (!evb) {
    throw std::logic_error("IO executor already destroyed.");
  }
  return evb;
}

uint16_t PooledRequestChannel::getProtocolId() {
  if (auto value = protocolId_.load(std::memory_order_relaxed)) {
    return value;
  }

  auto evb = getEvb({});
  evb->runImmediatelyOrRunInEventBaseThreadAndWait([&] {
    protocolId_.store(impl(*evb).getProtocolId(), std::memory_order_relaxed);
  });

  return protocolId_.load(std::memory_order_relaxed);
}

template <typename SendFunc>
void PooledRequestChannel::sendRequestImpl(
    SendFunc&& sendFunc, folly::Executor::KeepAlive<folly::EventBase>&& evb) {
  std::move(evb).add(
      [this, sendFunc = std::forward<SendFunc>(sendFunc)](
          auto&& keepAlive) mutable { sendFunc(impl(*keepAlive)); });
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
    RpcOptions&& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<false>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  auto evb = getEvb(options);
  sendRequestImpl(
      [options = std::move(options),
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestResponse(
            std::move(options),
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      std::move(evb));
}

void PooledRequestChannel::sendRequestNoResponse(
    RpcOptions&& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  if (!cob->isInlineSafe()) {
    cob = RequestClientCallback::Ptr(new ExecutorRequestCallback<true>(
        std::move(cob), getKeepAliveToken(callbackExecutor_)));
  }
  auto evb = getEvb(options);
  sendRequestImpl(
      [options = std::move(options),
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob = std::move(cob)](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestNoResponse(
            std::move(options),
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            std::move(cob));
      },
      std::move(evb));
}

void PooledRequestChannel::sendRequestStream(
    RpcOptions&& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    StreamClientCallback* cob) {
  auto evb = getEvb(options);
  sendRequestImpl(
      [options = std::move(options),
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestStream(
            std::move(options),
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            cob);
      },
      std::move(evb));
}

void PooledRequestChannel::sendRequestSink(
    RpcOptions&& options,
    apache::thrift::MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* cob) {
  auto evb = getEvb(options);
  sendRequestImpl(
      [options = std::move(options),
       methodMetadata = std::move(methodMetadata),
       request = std::move(request),
       header = std::move(header),
       cob](Impl& channel) mutable {
        maybeCreateInteraction(options, channel);
        channel.sendRequestSink(
            std::move(options),
            std::move(methodMetadata),
            std::move(request),
            std::move(header),
            cob);
      },
      std::move(evb));
}

InteractionId PooledRequestChannel::createInteraction(
    ManagedStringView&& name) {
  CHECK(!name.view().empty());
  return createInteractionId(reinterpret_cast<int64_t>(
      new InteractionState{getEvb({}), std::move(name), {}}));
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

PooledRequestChannel::EventBaseProvider PooledRequestChannel::wrapWeakPtr(
    std::weak_ptr<folly::IOExecutor> executor) {
  return [executor = std::move(
              executor)]() -> folly::Executor::KeepAlive<folly::EventBase> {
    if (auto ka = executor.lock()) {
      return {ka->getEventBase()};
    }
    return {};
  };
}

PooledRequestChannel::EventBaseProvider
PooledRequestChannel::globalExecutorProvider(size_t numThreads) {
  auto executor = folly::getGlobalIOExecutor();
  if (!executor) {
    throw std::logic_error("IO executor already destroyed.");
  }
  std::vector<folly::EventBase*> ebs;
  ebs.reserve(numThreads);
  for (size_t i = 0; i < numThreads; i++) {
    ebs.push_back(executor->getEventBase());
  }
  return
      [ebs = std::move(ebs),
       idx = std::make_unique<std::atomic<uint64_t>>(0),
       numThreads]() mutable -> folly::Executor::KeepAlive<folly::EventBase> {
        if (auto ka = folly::getGlobalIOExecutor()) {
          return {ebs.at(
              idx->fetch_add(1, std::memory_order_relaxed) % numThreads)};
        }
        return {};
      };
}
} // namespace thrift
} // namespace apache
