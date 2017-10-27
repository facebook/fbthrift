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
#include <folly/ExceptionWrapper.h>
#include <folly/io/async/Request.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

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
      folly::Baton<>& baton,
      bool oneway)
      : cb_(std::move(cb)), baton_(baton), oneway_(oneway) {}

  void requestSent() override {
    cb_->requestSent();
    if (oneway_) {
      baton_.post();
    }
  }

  void replyReceived(ClientReceiveState&& rs) override {
    DCHECK(!oneway_);
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
  bool oneway_;
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
  auto scb =
      std::make_unique<WaitableRequestCallback>(std::move(cb), baton, oneway);
  int result = sendRequestHelper(
      rpcOptions,
      oneway,
      std::move(scb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      connectionEvb);
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
  DestructorGuard dg(this);
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
  auto metadata = std::make_unique<RequestRpcMetadata>();
  if (!stripEnvelope(metadata.get(), buf)) {
    LOG(FATAL) << "Unexpected problem stripping envelope";
  }
  // The envelope does not say whether or not the call is oneway - so
  // we set it here.
  if (oneway) {
    metadata->kind = RpcKind::SINGLE_REQUEST_NO_RESPONSE;
  } else {
    metadata->kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  }
  metadata->__isset.kind = true;
  DCHECK(static_cast<ProtocolId>(protocolId_) == metadata->protocol);
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    metadata->clientTimeoutMs = rpcOptions.getTimeout().count();
    metadata->__isset.clientTimeoutMs = true;
  }
  if (rpcOptions.getQueueTimeout() > std::chrono::milliseconds(0)) {
    metadata->queueTimeoutMs = rpcOptions.getTimeout().count();
    metadata->__isset.queueTimeoutMs = true;
  }
  if (rpcOptions.getPriority() < concurrency::N_PRIORITIES) {
    metadata->priority = static_cast<RpcPriority>(rpcOptions.getPriority());
    metadata->__isset.priority = true;
  }
  metadata->otherMetadata = header->releaseWriteHeaders();
  auto* eh = header->getExtraWriteHeaders();
  if (eh) {
    metadata->otherMetadata.insert(eh->begin(), eh->end());
  }
  auto& pwh = getPersistentWriteHeaders();
  metadata->otherMetadata.insert(pwh.begin(), pwh.end());
  if (!metadata->otherMetadata.empty()) {
    metadata->__isset.otherMetadata = true;
  }
  auto callback = std::make_unique<ThriftClientCallback>(
      callbackEvb,
      std::move(cb),
      std::move(ctx),
      isSecurityActive(),
      protocolId_);
  connection_->getEventBase()->runInEventBaseThread(
      [evbChannel = channel,
       evbMetadata = std::move(metadata),
       evbBuf = std::move(buf),
       evbCallback = std::move(callback)]() mutable {
        evbChannel->sendThriftRequest(
            std::move(evbMetadata), std::move(evbBuf), std::move(evbCallback));
      });
  return 0;
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
