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
#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>

#include <folly/io/async/AsyncSocketException.h>

namespace apache {
namespace thrift {

class ReconnectingRequestChannel::RequestCallback
    : public apache::thrift::RequestClientCallback {
 public:
  RequestCallback(
      ReconnectingRequestChannel& channel,
      apache::thrift::RequestClientCallback::Ptr cob)
      : channel_(channel), impl_(channel_.impl_), cob_(std::move(cob)) {}

  void onRequestSent() noexcept override {
    cob_->onRequestSent();
  }

  void onResponse(
      apache::thrift::ClientReceiveState&& state) noexcept override {
    cob_.release()->onResponse(std::move(state));
    delete this;
  }

  void onResponseError(folly::exception_wrapper ex) noexcept override {
    handleTransportException(ex);
    cob_.release()->onResponseError(std::move(ex));
    delete this;
  }

 private:
  void handleTransportException(folly::exception_wrapper& ex) {
    if (!ex.is_compatible_with<
            apache::thrift::transport::TTransportException>()) {
      return;
    }
    if (channel_.impl_ != impl_) {
      return;
    }
    channel_.impl_.reset();
  }

  ReconnectingRequestChannel& channel_;
  ReconnectingRequestChannel::ImplPtr impl_;
  apache::thrift::RequestClientCallback::Ptr cob_;
};

void ReconnectingRequestChannel::sendRequestResponse(
    apache::thrift::RpcOptions& options,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    apache::thrift::RequestClientCallback::Ptr cob) {
  cob = apache::thrift::RequestClientCallback::Ptr(
      new RequestCallback(*this, std::move(cob)));

  return impl().sendRequestResponse(
      options, std::move(buf), std::move(header), std::move(cob));
}

ReconnectingRequestChannel::Impl& ReconnectingRequestChannel::impl() {
  if (!impl_) {
    impl_ = implCreator_(evb_);
  }

  return *impl_;
}
} // namespace thrift
} // namespace apache
