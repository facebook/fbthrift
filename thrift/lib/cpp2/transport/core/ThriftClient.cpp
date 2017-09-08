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

#include <thrift/lib/cpp2/transport/core/ThriftClient.h>

#include <folly/Baton.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using apache::thrift::transport::THeader;
using apache::thrift::transport::TTransportException;
using folly::EventBase;
using folly::IOBuf;
using folly::RequestContext;

namespace {

/**
 * Used as the callback for sendRequestSync.  It delegates to the
 * wrapped callback and posts on the baton object to allow the
 * synchronous call to continue.
 */
class WaitableRequestCallback final : public RequestCallback {
 public:
  WaitableRequestCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::Baton<>& baton)
      : cb_(std::move(cb)), baton_(baton) {}

  void requestSent() override {
    cb_->requestSent();
  }

  void replyReceived(ClientReceiveState&& rs) override {
    cb_->replyReceived(std::move(rs));
    baton_.post();
  }

  void requestError(ClientReceiveState&& rs) override {
    DCHECK(rs.isException());
    cb_->requestError(std::move(rs));
    baton_.post();
  }

 private:
  std::unique_ptr<RequestCallback> cb_;
  folly::Baton<>& baton_;
};

} // namespace

ThriftClient::ThriftClient(
    std::shared_ptr<ClientConnectionIf> connection,
    folly::EventBase* callbackEvb)
    : connection_(connection), callbackEvb_(callbackEvb) {}

ThriftClient::ThriftClient(std::shared_ptr<ClientConnectionIf> connection)
    : ThriftClient(connection, connection->getEventBase()) {}

void ThriftClient::setProtocolId(uint16_t protocolId) {
  protocolId_ = protocolId;
}

uint32_t ThriftClient::sendRequestSync(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  // Synchronous calls may be made from any thread except the one used
  // by the underlying connection.  That thread is used to handle the
  // callback and to release this thread that will be waiting on
  // "baton".
  EventBase* connectionEvb = connection_->getEventBase();
  DCHECK(!connectionEvb->inRunningEventBaseThread());
  folly::Baton<> baton;
  DCHECK(typeid(ClientSyncCallback) == typeid(*cb));
  bool oneway = static_cast<ClientSyncCallback&>(*cb).isOneway();
  auto scb = std::make_unique<WaitableRequestCallback>(std::move(cb), baton);
  int result;
  if (oneway) {
    result = sendRequestHelper(
        rpcOptions,
        true,
        std::move(scb),
        std::move(ctx),
        std::move(buf),
        std::move(header),
        connectionEvb);
  } else {
    result = sendRequestHelper(
        rpcOptions,
        false,
        std::move(scb),
        std::move(ctx),
        std::move(buf),
        std::move(header),
        connectionEvb);
  }
  baton.wait();
  return result;
}

uint32_t ThriftClient::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  return sendRequestHelper(
      rpcOptions,
      false,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      callbackEvb_);
}

uint32_t ThriftClient::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  sendRequestHelper(
      rpcOptions,
      true,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      callbackEvb_);
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t ThriftClient::sendRequestHelper(
    RpcOptions& rpcOptions,
    bool oneway,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header,
    EventBase* callbackEvb) {
  // TODO: We'll deal with properties such as timeouts later.  Timeouts
  // are of multiple kinds and can be set in different ways - so need to
  // understand this first.
  DestructorGuard dg(this); //// understand what this and
                            //// RequestContextScopeGuard are doing!!!!
  cb->context_ = RequestContext::saveContext();
  std::shared_ptr<ThriftChannelIf> channel;
  try {
    channel = connection_->getChannel();
  } catch (TTransportException& te) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(std::move(te)),
        std::move(ctx),
        isSecurityActive()));
    return 0;
  }
  std::unique_ptr<FunctionInfo> finfo = std::make_unique<FunctionInfo>();
  // TODO: add function name.  We don't use it right now.  The payload
  // already contains the function name - so that's where the name is
  // obtained right now.
  finfo->name = "";
  if (oneway) {
    finfo->kind = SINGLE_REQUEST_NO_RESPONSE;
  } else {
    finfo->kind = SINGLE_REQUEST_SINGLE_RESPONSE;
  }
  finfo->seqId = 0; // not used.
  finfo->protocol = static_cast<PROTOCOL_TYPES>(protocolId_);
  std::unique_ptr<map<string, string>> headers;
  if (channel->supportsHeaders()) {
    headers = buildHeaders(header.get(), rpcOptions);
  }
  std::unique_ptr<ThriftClientCallback> callback;
  if (!oneway) {
    callback = std::make_unique<ThriftClientCallback>(
        callbackEvb,
        std::move(cb),
        std::move(ctx),
        isSecurityActive(),
        protocolId_);
  }
  connection_->getEventBase()->runInEventBaseThread([
    evbChannel = channel,
    evbFinfo = std::move(finfo),
    evbHeaders = std::move(headers),
    evbBuf = std::move(buf),
    evbCallback = std::move(callback),
    oneway,
    evbCb = std::move(cb)
  ]() mutable {
    evbChannel->sendThriftRequest(
        std::move(evbFinfo),
        std::move(evbHeaders),
        std::move(evbBuf),
        std::move(evbCallback));
    if (oneway) {
      // TODO: We only invoke requestSent for oneway calls.  I don't think it
      // use used for any other kind of call.  Verify.
      evbCb->requestSent();
    }
  });
  return 0;
}

std::unique_ptr<map<string, string>> ThriftClient::buildHeaders(
    THeader* header,
    RpcOptions& rpcOptions) {
  header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
  header->forceClientType(THRIFT_HTTP_CLIENT_TYPE);
  addRpcOptionHeaders(header, rpcOptions);
  auto headers = std::make_unique<map<string, string>>();
  auto pwh = getPersistentWriteHeaders();
  headers->insert(pwh.begin(), pwh.end());
  {
    auto wh = header->releaseWriteHeaders();
    headers->insert(wh.begin(), wh.end());
  }
  auto eh = header->getExtraWriteHeaders();
  if (eh) {
    headers->insert(eh->begin(), eh->end());
  }
  return headers;
}

EventBase* ThriftClient::getEventBase() const {
  return callbackEvb_;
}

uint16_t ThriftClient::getProtocolId() {
  return protocolId_;
}

void ThriftClient::setCloseCallback(CloseCallback* /*cb*/) {
  // TBD
}

TAsyncTransport* ThriftClient::getTransport() {
  return connection_->getTransport();
}

bool ThriftClient::good() {
  return connection_->good();
}

ClientChannel::SaturationStatus ThriftClient::getSaturationStatus() {
  return connection_->getSaturationStatus();
}

void ThriftClient::attachEventBase(folly::EventBase* eventBase) {
  connection_->attachEventBase(eventBase);
}

void ThriftClient::detachEventBase() {
  connection_->detachEventBase();
}

bool ThriftClient::isDetachable() {
  return connection_->isDetachable();
}

bool ThriftClient::isSecurityActive() {
  return connection_->isSecurityActive();
}

uint32_t ThriftClient::getTimeout() {
  return connection_->getTimeout();
}

void ThriftClient::setTimeout(uint32_t ms) {
  return connection_->setTimeout(ms);
}

void ThriftClient::closeNow() {
  connection_->closeNow();
}

CLIENT_TYPE ThriftClient::getClientType() {
  return connection_->getClientType();
}

} // namespace thrift
} // namespace apache
