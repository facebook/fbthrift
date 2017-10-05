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

#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>

#include <glog/logging.h>

namespace apache {
namespace thrift {

using folly::EventBase;
using folly::IOBuf;
using folly::exception_wrapper;

ThriftClientCallback::ThriftClientCallback(
    EventBase* evb,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    bool isSecurityActive,
    uint16_t protoId)
    : evb_(evb),
      cb_(std::move(cb)),
      ctx_(std::move(ctx)),
      isSecurityActive_(isSecurityActive),
      protoId_(protoId) {}

void ThriftClientCallback::onThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(metadata);
  DCHECK(evb_->isInEventBaseThread());
  if (cb_) {
    auto tHeader = std::make_unique<transport::THeader>();
    tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);
    if (metadata->__isset.otherMetadata) {
      tHeader->setReadHeaders(std::move(metadata->otherMetadata));
    }
    cb_->replyReceived(ClientReceiveState(
        protoId_,
        std::move(payload),
        std::move(tHeader),
        std::move(ctx_),
        isSecurityActive_));
    cb_ = nullptr;
  }
}

void ThriftClientCallback::onError(exception_wrapper ex) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  if (cb_) {
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestError(
        ClientReceiveState(std::move(ex), std::move(ctx_), isSecurityActive_));
    cb_ = nullptr;
  }
}

void ThriftClientCallback::cancel(exception_wrapper /*ex*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

EventBase* ThriftClientCallback::getEventBase() const {
  return evb_;
}

} // namespace thrift
} // namespace apache
