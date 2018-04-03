/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>

#include <folly/futures/Future.h>

namespace apache {
namespace thrift {

uint16_t PooledRequestChannel::getProtocolId() {
  folly::call_once(protocolIdInitFlag_, [&] {
    auto evb = executor_->getEventBase();
    evb->runInEventBaseThreadAndWait(
        [&] { protocolId_ = impl(*evb).getProtocolId(); });
  });

  return protocolId_;
}

uint32_t PooledRequestChannel::sendRequestImpl(
    RpcKind rpcKind,
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  auto evb = executor_->getEventBase();

  evb->runInEventBaseThread([this,
                             evb,
                             keepAlive = evb->getKeepAliveToken(),
                             options = std::move(options),
                             rpcKind,
                             cob = std::move(cob),
                             ctx = std::move(ctx),
                             buf = std::move(buf),
                             header = std::move(header)]() mutable {
    switch (rpcKind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        impl(*evb).sendOnewayRequest(
            options,
            std::move(cob),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        impl(*evb).sendRequest(
            options,
            std::move(cob),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        impl(*evb).sendStreamRequest(
            options,
            std::move(cob),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      default:
        folly::assume_unreachable();
        break;
    };
  });
  return 0;
}

namespace {
class ExecutorRequestCallback final : public RequestCallback {
 public:
  ExecutorRequestCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::Executor* executor)
      : cb_(std::move(cb)), executor_(executor) {
    CHECK(executor);
  }

  void requestSent() override {
    executor_->add([cb = cb_] { cb->requestSent(); });
  }
  void replyReceived(ClientReceiveState&& rs) override {
    executor_->add([cb = std::move(cb_), rs = std::move(rs)]() mutable {
      cb->replyReceived(std::move(rs));
    });
  }
  void requestError(ClientReceiveState&& rs) override {
    executor_->add([cb = std::move(cb_), rs = std::move(rs)]() mutable {
      cb->requestError(std::move(rs));
    });
  }

 private:
  std::shared_ptr<RequestCallback> cb_;
  folly::Executor* executor_;
};

class SyncRequestCallback final : public RequestCallback {
 public:
  SyncRequestCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::Promise<folly::Unit> promise)
      : cb_(std::move(cb)), promise_(std::move(promise)) {}

  void requestSent() override {
    cb_->requestSent();
    if (static_cast<ClientSyncCallback*>(cb_.get())->isOneway()) {
      promise_.setValue();
    }
  }
  void replyReceived(ClientReceiveState&& rs) override {
    cb_->replyReceived(std::move(rs));
    promise_.setValue();
  }
  void requestError(ClientReceiveState&& rs) override {
    cb_->requestError(std::move(rs));
    promise_.setValue();
  }

 private:
  std::unique_ptr<RequestCallback> cb_;
  folly::Promise<folly::Unit> promise_;
};
} // namespace

uint32_t PooledRequestChannel::sendRequest(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  cob = std::make_unique<ExecutorRequestCallback>(
      std::move(cob), callbackExecutor_);
  sendRequestImpl(
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      options,
      std::move(cob),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

uint32_t PooledRequestChannel::sendOnewayRequest(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  cob = std::make_unique<ExecutorRequestCallback>(
      std::move(cob), callbackExecutor_);
  sendRequestImpl(
      RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      options,
      std::move(cob),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

uint32_t PooledRequestChannel::sendStreamRequest(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  cob = std::make_unique<ExecutorRequestCallback>(
      std::move(cob), callbackExecutor_);
  sendRequestImpl(
      RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      options,
      std::move(cob),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

uint32_t PooledRequestChannel::sendRequestSync(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  folly::Promise<folly::Unit> promise;
  auto future = promise.getSemiFuture();
  DCHECK(typeid(ClientSyncCallback) == typeid(*cob));
  RpcKind kind = static_cast<ClientSyncCallback&>(*cob).rpcKind();
  cob =
      std::make_unique<SyncRequestCallback>(std::move(cob), std::move(promise));
  sendRequestImpl(
      kind,
      options,
      std::move(cob),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  std::move(future).get();
  return 0;
}

PooledRequestChannel::Impl& PooledRequestChannel::impl(folly::EventBase& evb) {
  DCHECK(evb.inRunningEventBaseThread());

  auto creator = [&evb, &implCreator = implCreator_] {
    return implCreator(evb);
  };
  return impl_.getOrCreateFn(evb, creator);
}
} // namespace thrift
} // namespace apache
