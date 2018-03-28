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

namespace apache {
namespace thrift {

namespace {
class ClientSyncEventBaseCallback final : public RequestCallback {
 public:
  ClientSyncEventBaseCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::EventBase* eb)
      : cb_(std::move(cb)), eb_(eb) {}

  void requestSent() override {
    cb_->requestSent();
    if (static_cast<ClientSyncCallback*>(cb_.get())->isOneway()) {
      assert(eb_);
      eb_->terminateLoopSoon();
    }
  }
  void replyReceived(ClientReceiveState&& rs) override {
    assert(eb_);
    cb_->replyReceived(std::move(rs));
    eb_->terminateLoopSoon();
  }
  void requestError(ClientReceiveState&& rs) override {
    assert(rs.isException());
    assert(eb_);
    cb_->requestError(std::move(rs));
    eb_->terminateLoopSoon();
  }

 private:
  std::unique_ptr<RequestCallback> cb_;
  folly::EventBase* eb_;
};
} // namespace

uint32_t RequestChannel::sendRequestSync(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  DCHECK(typeid(ClientSyncCallback) == typeid(*cb));
  apache::thrift::RpcKind kind =
      static_cast<ClientSyncCallback&>(*cb).rpcKind();
  auto eb = getEventBase();
  CHECK(eb->isInEventBaseThread());
  auto scb = std::make_unique<ClientSyncEventBaseCallback>(std::move(cb), eb);
  switch (kind) {
    case apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE: {
      auto ret = sendOnewayRequest(
          options,
          std::move(scb),
          std::move(ctx),
          std::move(buf),
          std::move(header));
      eb->loopForever();
      return ret;
    }
    case apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE: {
      auto ret = sendRequest(
          options,
          std::move(scb),
          std::move(ctx),
          std::move(buf),
          std::move(header));
      eb->loopForever();
      return ret;
    }
    case apache::thrift::RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE: {
      auto ret = sendStreamRequest(
          options,
          std::move(scb),
          std::move(ctx),
          std::move(buf),
          std::move(header));
      eb->loopForever();
      return ret;
    }
    default:
      break;
  }
  scb->requestError(ClientReceiveState(
      folly::make_exception_wrapper<transport::TTransportException>(
          "Unsupported RpcKind value"),
      std::move(ctx),
      false));
  eb->loopForever();
  return 0;
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
      std::move(ctx),
      false));
  return 0;
}
} // namespace thrift
} // namespace apache
