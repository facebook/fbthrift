/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#pragma once

#include <folly/ExceptionWrapper.h>

namespace apache::thrift {

template <class RequestGuardType>
class GuardedRequestClientCallback : public RequestClientCallback {
 public:
  GuardedRequestClientCallback(RequestClientCallback::Ptr cb)
      : cb_(std::move(cb)) {}

  bool isInlineSafe() const override { return cb_->isInlineSafe(); }

  bool isSync() const override { return cb_->isSync(); }

  folly::Executor::KeepAlive<> getExecutor() const override {
    return cb_->getExecutor();
  }

  void onResponse(ClientReceiveState&& state) noexcept override {
    cb_.release()->onResponse(std::move(state));
    delete this;
  }

  void onResponseError(folly::exception_wrapper ex) noexcept override {
    cb_.release()->onResponseError(std::move(ex));
    delete this;
  }

 private:
  RequestClientCallback::Ptr cb_;
  RequestGuardType guard_;
};

template <class RequestGuardType, class ChannelGuardType>
void GuardedRequestChannel<RequestGuardType, ChannelGuardType>::
    setCloseCallback(CloseCallback* callback) {
  impl_->setCloseCallback(std::move(callback));
}

template <class RequestGuardType, class ChannelGuardType>
folly::EventBase*
GuardedRequestChannel<RequestGuardType, ChannelGuardType>::getEventBase()
    const {
  return impl_->getEventBase();
}

template <class RequestGuardType, class ChannelGuardType>
uint16_t
GuardedRequestChannel<RequestGuardType, ChannelGuardType>::getProtocolId() {
  return impl_->getProtocolId();
}

template <class RequestGuardType, class ChannelGuardType>
void GuardedRequestChannel<RequestGuardType, ChannelGuardType>::
    sendRequestResponse(
        RpcOptions&& rpcOptions,
        MethodMetadata&& methodMetadata,
        SerializedRequest&& serializedRequest,
        std::shared_ptr<transport::THeader> header,
        RequestClientCallback::Ptr cb) {
  auto wrappedCb = RequestClientCallback::Ptr(
      new GuardedRequestClientCallback<RequestGuardType>(std::move(cb)));

  impl_->sendRequestResponse(
      std::move(rpcOptions),
      std::move(methodMetadata),
      std::move(serializedRequest),
      std::move(header),
      std::move(wrappedCb));
}

} // namespace apache::thrift
