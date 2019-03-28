/*
 * Copyright 2014-present Facebook, Inc.
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
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/Fiber.h>

namespace apache {
namespace thrift {

void RequestChannel::sendRequestAsync(
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RpcKind kind) {
  auto eb = getEventBase();
  if (!eb || eb->isInEventBaseThread()) {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        // Calling asyncComplete before sending because
        // sendOnewayRequest moves from ctx and clears it.
        ctx->asyncComplete();
        sendOnewayRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        sendRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        sendStreamRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      default:
        folly::assume_unreachable();
        break;
    }

  } else {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          // Calling asyncComplete before sending because
          // sendOnewayRequest moves from ctx and clears it.
          ctx->asyncComplete();
          sendOnewayRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          sendRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          sendStreamRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      default:
        folly::assume_unreachable();
        break;
    }
  }
}

namespace {
class ClientSyncBatonCallback final : public RequestCallback {
 public:
  ClientSyncBatonCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::fibers::Baton& doneBaton)
      : cb_(std::move(cb)), doneBaton_(doneBaton) {}

  void requestSent() override {
    cb_->requestSent();
    if (static_cast<ClientSyncCallback*>(cb_.get())->isOneway()) {
      doneBaton_.post();
    }
  }
  void replyReceived(ClientReceiveState&& rs) override {
    cb_->replyReceived(std::move(rs));
    doneBaton_.post();
  }
  void requestError(ClientReceiveState&& rs) override {
    assert(rs.isException());
    cb_->requestError(std::move(rs));
    doneBaton_.post();
  }

 private:
  std::unique_ptr<RequestCallback> cb_;
  folly::fibers::Baton& doneBaton_;
};
} // namespace

uint32_t RequestChannel::sendRequestSync(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  DCHECK(dynamic_cast<ClientSyncCallback*>(cb.get()));
  apache::thrift::RpcKind kind =
      static_cast<ClientSyncCallback&>(*cb).rpcKind();
  auto eb = getEventBase();
  // We intentionally only support sync_* calls from the EventBase thread.
  eb->checkIsInEventBaseThread();

  folly::fibers::Baton baton;
  uint32_t retval = 0;
  auto scb = std::make_unique<ClientSyncBatonCallback>(std::move(cb), baton);

  folly::exception_wrapper ew;
  baton.wait([&, onFiber = folly::fibers::onFiber()]() {
    try {
      switch (kind) {
        case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
          retval = sendOnewayRequest(
              options,
              std::move(scb),
              std::move(ctx),
              std::move(buf),
              std::move(header));
          break;
        case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          retval = sendRequest(
              options,
              std::move(scb),
              std::move(ctx),
              std::move(buf),
              std::move(header));
          break;
        case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
          retval = sendStreamRequest(
              options,
              std::move(scb),
              std::move(ctx),
              std::move(buf),
              std::move(header));
          break;
        default:
          folly::assume_unreachable();
      }
    } catch (const std::exception& e) {
      ew = folly::exception_wrapper(std::current_exception(), e);
      baton.post();
    } catch (...) {
      ew = folly::exception_wrapper(std::current_exception());
      baton.post();
    }
    if (!onFiber) {
      while (!baton.ready()) {
        eb->drive();
      }
    }
  });
  if (ew) {
    ew.throw_exception();
  }
  return retval;
}

uint32_t RequestChannel::sendStreamRequest(
    RpcOptions&,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf>,
    std::shared_ptr<apache::thrift::transport::THeader>) {
  cb->requestError(ClientReceiveState(
      folly::make_exception_wrapper<transport::TTransportException>(
          "Current channel doesn't support stream RPC"),
      std::move(ctx)));
  return 0;
}

} // namespace thrift
} // namespace apache
