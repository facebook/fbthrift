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

#include <thrift/lib/cpp2/async/HibernatingRequestChannel.h>

namespace apache {
namespace thrift {

// RequestCallback will keep owning the connection for inflight requests even
// after the timeout.
class HibernatingRequestChannel::RequestCallback
    : public apache::thrift::RequestCallback {
 public:
  RequestCallback(
      std::unique_ptr<apache::thrift::RequestCallback> cob,
      ImplPtr impl)
      : impl_(std::move(impl)), cob_(std::move(cob)) {}

  void requestSent() override {
    cob_->requestSent();
  }

  void replyReceived(ClientReceiveState&& state) override {
    cob_->replyReceived(std::move(state));
  }

  void requestError(ClientReceiveState&& state) override {
    cob_->requestError(std::move(state));
  }

 private:
  HibernatingRequestChannel::ImplPtr impl_;
  std::unique_ptr<apache::thrift::RequestCallback> cob_;
};

uint32_t HibernatingRequestChannel::sendRequest(
    RpcOptions& options,
    std::unique_ptr<apache::thrift::RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  auto implPtr = impl();
  auto& implRef = *implPtr;
  cob = std::make_unique<RequestCallback>(std::move(cob), std::move(implPtr));
  auto ret = implRef.sendRequest(
      options,
      std::move(cob),
      std::move(ctx),
      std::move(buf),
      std::move(header));

  timeout_->scheduleTimeout(waitTime_.count());

  return ret;
}

HibernatingRequestChannel::ImplPtr& HibernatingRequestChannel::impl() {
  if (!impl_) {
    impl_ = implCreator_(evb_);
  }

  return impl_;
}
} // namespace thrift
} // namespace apache
