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

#include <folly/io/async/Request.h>
#include <glog/logging.h>

namespace apache {
namespace thrift {

using namespace apache::thrift::transport;
using folly::EventBase;
using folly::exception_wrapper;
using folly::IOBuf;

const std::chrono::milliseconds ThriftClientCallback::kDefaultTimeout =
    std::chrono::milliseconds(500);

ThriftClientCallback::ThriftClientCallback(
    EventBase* evb,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    bool isSecurityActive,
    uint16_t protoId,
    std::chrono::milliseconds timeout)
    : evb_(evb),
      cb_(std::move(cb)),
      ctx_(std::move(ctx)),
      isSecurityActive_(isSecurityActive),
      protoId_(protoId),
      active_(cb_),
      timeout_(timeout) {}

ThriftClientCallback::~ThriftClientCallback() {
  active_ = false;
  cancelTimeout();
}

void ThriftClientCallback::onThriftRequestSent() {
  DCHECK(evb_->isInEventBaseThread());
  if (active_) {
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestSent();

    if (timeout_.count() > 0) {
      evb_->timer().scheduleTimeout(this, timeout_);
    }
  }
}

void ThriftClientCallback::onThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(metadata);
  DCHECK(evb_->isInEventBaseThread());
  cancelTimeout();
  if (active_) {
    active_ = false;
    auto tHeader = std::make_unique<transport::THeader>();
    tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);
    if (metadata->__isset.otherMetadata) {
      tHeader->setReadHeaders(std::move(metadata->otherMetadata));
    }
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->replyReceived(ClientReceiveState(
        protoId_,
        std::move(payload),
        std::move(tHeader),
        std::move(ctx_),
        isSecurityActive_));
  }
}

void ThriftClientCallback::onThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    Stream<std::unique_ptr<folly::IOBuf>> stream) noexcept {
  DCHECK(metadata);
  DCHECK(evb_->isInEventBaseThread());
  cancelTimeout();
  if (active_) {
    active_ = false;
    auto tHeader = std::make_unique<transport::THeader>();
    tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);
    if (metadata->__isset.otherMetadata) {
      tHeader->setReadHeaders(std::move(metadata->otherMetadata));
    }
    folly::RequestContextScopeGuard rctx(cb_->context_);

    ClientReceiveState crs(
        protoId_,
        ResponseAndSemiStream<
            std::unique_ptr<folly::IOBuf>,
            std::unique_ptr<folly::IOBuf>>{std::move(payload),
                                           std::move(stream)},
        std::move(tHeader),
        std::move(ctx_),
        isSecurityActive_);
    cb_->replyReceived(std::move(crs));
  }
}

void ThriftClientCallback::onError(exception_wrapper ex) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  cancelTimeout();
  if (active_) {
    active_ = false;
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestError(
        ClientReceiveState(std::move(ex), std::move(ctx_), isSecurityActive_));
  }
}

EventBase* ThriftClientCallback::getEventBase() const {
  return evb_;
}

void ThriftClientCallback::timeoutExpired() noexcept {
  if (active_) {
    active_ = false;
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(
            apache::thrift::transport::TTransportException::TIMED_OUT),
        std::move(ctx_),
        isSecurityActive_));
  }
}

void ThriftClientCallback::callbackCanceled() noexcept {
  // nothing!
}

} // namespace thrift
} // namespace apache
