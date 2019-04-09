/*
 * Copyright 2017-present Facebook, Inc.
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
#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>

#include <folly/io/async/AsyncSocketException.h>

namespace apache {
namespace thrift {

class RetryingRequestChannel::RequestCallback
    : public apache::thrift::RequestCallback {
 public:
  RequestCallback(
      folly::Executor::KeepAlive<> ka,
      RetryingRequestChannel::ImplPtr impl,
      int retriesLeft,
      apache::thrift::RpcOptions options,
      std::unique_ptr<apache::thrift::RequestCallback> cob,
      std::unique_ptr<apache::thrift::ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header)
      : impl_(std::move(impl)),
        retriesLeft_(retriesLeft),
        options_(options),
        cob_(std::move(cob)),
        ctx_(std::move(ctx)),
        buf_(std::move(buf)),
        header_(std::move(header)) {
    if (retriesLeft_) {
      ka_ = std::move(ka);
    }
  }

  void requestSent() override {
    cob_->requestSent();
  }

  void replyReceived(apache::thrift::ClientReceiveState&& state) override {
    if (shouldRetry(state)) {
      retry();
    } else {
      cob_->replyReceived(std::move(state));
    }
  }

  void requestError(apache::thrift::ClientReceiveState&& state) override {
    if (shouldRetry(state)) {
      retry();
    } else {
      cob_->requestError(std::move(state));
    }
  }

 private:
  bool shouldRetry(apache::thrift::ClientReceiveState& state) {
    if (!state.isException()) {
      return false;
    }
    if (!state.exception()
             .is_compatible_with<
                 apache::thrift::transport::TTransportException>()) {
      return false;
    }
    return retriesLeft_ > 0;
  }

  void retry() {
    auto& impl = *impl_;
    auto fakeCtx =
        std::make_unique<apache::thrift::ContextStack>(ctx_->getMethod());
    auto cob = std::make_unique<RequestCallback>(
        std::move(ka_),
        std::move(impl_),
        retriesLeft_ - 1,
        options_,
        std::move(cob_),
        std::move(ctx_),
        buf_->clone(),
        header_);

    impl.sendRequest(
        options_,
        std::move(cob),
        std::move(fakeCtx),
        std::move(buf_),
        std::move(header_));
  }

  folly::Executor::KeepAlive<> ka_;
  RetryingRequestChannel::ImplPtr impl_;
  int retriesLeft_;
  apache::thrift::RpcOptions options_;
  std::unique_ptr<apache::thrift::RequestCallback> cob_;
  std::unique_ptr<apache::thrift::ContextStack> ctx_;
  std::unique_ptr<folly::IOBuf> buf_;
  std::shared_ptr<apache::thrift::transport::THeader> header_;
};

uint32_t RetryingRequestChannel::sendRequest(
    apache::thrift::RpcOptions& options,
    std::unique_ptr<apache::thrift::RequestCallback> cob,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  auto fakeCtx =
      std::make_unique<apache::thrift::ContextStack>(ctx->getMethod());
  cob = std::make_unique<RequestCallback>(
      folly::getKeepAliveToken(evb_),
      impl_,
      numRetries_,
      options,
      std::move(cob),
      std::move(ctx),
      buf->clone(),
      header);

  return impl_->sendRequest(
      options,
      std::move(cob),
      std::move(fakeCtx),
      std::move(buf),
      std::move(header));
}
} // namespace thrift
} // namespace apache
