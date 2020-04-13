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

#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>

#include <folly/io/async/AsyncSocketException.h>

namespace apache {
namespace thrift {

class RetryingRequestChannel::RequestCallback
    : public apache::thrift::RequestClientCallback {
 public:
  RequestCallback(
      folly::Executor::KeepAlive<> ka,
      RetryingRequestChannel::ImplPtr impl,
      int retries,
      const apache::thrift::RpcOptions& options,
      apache::thrift::RequestClientCallback::Ptr cob,
      folly::StringPiece methodName,
      SerializedRequest&& request,
      std::shared_ptr<apache::thrift::transport::THeader> header)
      : impl_(std::move(impl)),
        retriesLeft_(retries),
        options_(options),
        cob_(std::move(cob)),
        methodName_(methodName.str()),
        request_(std::move(request)),
        header_(std::move(header)) {
    if (retriesLeft_) {
      ka_ = std::move(ka);
    }
  }

  void onRequestSent() noexcept override {}

  void onResponse(
      apache::thrift::ClientReceiveState&& state) noexcept override {
    cob_->onRequestSent();
    cob_.release()->onResponse(std::move(state));
    delete this;
  }

  void onResponseError(folly::exception_wrapper ex) noexcept override {
    if (shouldRetry(ex)) {
      retry();
    } else {
      cob_.release()->onResponseError(std::move(ex));
      delete this;
    }
  }

 private:
  bool shouldRetry(folly::exception_wrapper& ex) {
    if (!ex.is_compatible_with<
            apache::thrift::transport::TTransportException>()) {
      return false;
    }
    return retriesLeft_ > 0;
  }

  void retry() {
    if (!--retriesLeft_) {
      ka_.reset();
    }

    impl_->sendRequestResponse(
        options_,
        methodName_,
        SerializedRequest(request_.buffer->clone()),
        header_,
        RequestClientCallback::Ptr(this));
  }

  folly::Executor::KeepAlive<> ka_;
  RetryingRequestChannel::ImplPtr impl_;
  int retriesLeft_;
  apache::thrift::RpcOptions options_;
  RequestClientCallback::Ptr cob_;
  std::string methodName_;
  SerializedRequest request_;
  std::shared_ptr<apache::thrift::transport::THeader> header_;
};

void RetryingRequestChannel::sendRequestResponse(
    const apache::thrift::RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  cob = RequestClientCallback::Ptr(new RequestCallback(
      folly::getKeepAliveToken(evb_),
      impl_,
      numRetries_,
      options,
      std::move(cob),
      methodName,
      SerializedRequest(request.buffer->clone()),
      header));

  return impl_->sendRequestResponse(
      options,
      methodName,
      std::move(request),
      std::move(header),
      std::move(cob));
}
} // namespace thrift
} // namespace apache
