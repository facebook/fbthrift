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

#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>

#include <glog/logging.h>
#include <proxygen/lib/http/codec/HTTP1xCodec.h>
#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <proxygen/lib/http/codec/TransportDirection.h>
#include <proxygen/lib/utils/WheelTimerInstance.h>
#include <thrift/lib/cpp2/transport/http2/client/H2TransactionCallback.h>
#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>
#include <wangle/acceptor/TransportInfo.h>

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncTransport;
using std::string;
using folly::EventBase;
using proxygen::HTTPUpstreamSession;
using proxygen::WheelTimerInstance;

const std::chrono::milliseconds H2ClientConnection::kDefaultTransactionTimeout =
    std::chrono::milliseconds(500);

std::unique_ptr<ClientConnectionIf> H2ClientConnection::newHTTP1xConnection(
    TAsyncTransport::UniquePtr transport,
    const string& httpHost,
    const string& httpUrl) {
  std::unique_ptr<H2ClientConnection> connection(new H2ClientConnection(
      std::move(transport),
      std::make_unique<proxygen::HTTP1xCodec>(
          proxygen::TransportDirection::UPSTREAM)));
  connection->httpHost_ = httpHost;
  connection->httpUrl_ = httpUrl;
  return connection;
}

std::unique_ptr<ClientConnectionIf> H2ClientConnection::newHTTP2Connection(
    TAsyncTransport::UniquePtr transport) {
  return std::unique_ptr<H2ClientConnection>(new H2ClientConnection(
      std::move(transport),
      std::make_unique<proxygen::HTTP2Codec>(
          proxygen::TransportDirection::UPSTREAM)));
}

H2ClientConnection::H2ClientConnection(
    TAsyncTransport::UniquePtr transport,
    std::unique_ptr<proxygen::HTTPCodec> codec)
    : evb_(transport->getEventBase()) {
  auto localAddress = transport->getLocalAddress();
  auto peerAddress = transport->getPeerAddress();

  httpSession_ = new HTTPUpstreamSession(
      WheelTimerInstance(timeout_, evb_),
      std::move(transport),
      localAddress,
      peerAddress,
      std::move(codec),
      wangle::TransportInfo(),
      nullptr);
}

H2ClientConnection::~H2ClientConnection() {
  closeNow();
}

std::shared_ptr<ThriftChannelIf> H2ClientConnection::getChannel() {
  if (!httpSession_) {
    // TODO: deal with this case later.
    return std::shared_ptr<ThriftChannelIf>();
  }
  // Question for JiaJie: Why does this self-destruct?  In your code,
  // how does txn get destroyed?
  auto httpCallback = new H2TransactionCallback();
  auto txn = httpSession_->newTransaction(httpCallback);
  if (!txn) {
    // TODO: deal with this case later.
    return std::shared_ptr<ThriftChannelIf>();
  }
  // TODO: do timeout setting.
  auto channel = std::make_shared<SingleRpcChannel>(txn);
  httpCallback->setChannel(channel);
  return channel;
}

void H2ClientConnection::setMaxPendingRequests(uint32_t num) {
  if (httpSession_) {
    httpSession_->setMaxConcurrentOutgoingStreams(num);
  }
}

EventBase* H2ClientConnection::getEventBase() const {
  return evb_;
}

TAsyncTransport* H2ClientConnection::getTransport() {
  if (httpSession_) {
    return dynamic_cast<TAsyncTransport*>(httpSession_->getTransport());
  } else {
    return nullptr;
  }
}

bool H2ClientConnection::good() {
  auto transport = httpSession_ ? httpSession_->getTransport() : nullptr;
  return transport && transport->good();
}

ClientChannel::SaturationStatus H2ClientConnection::getSaturationStatus() {
  if (httpSession_) {
    return ClientChannel::SaturationStatus(
        httpSession_->getNumOutgoingStreams(),
        httpSession_->getMaxConcurrentOutgoingStreams());
  } else {
    return ClientChannel::SaturationStatus();
  }
}

void H2ClientConnection::attachEventBase(EventBase* evb) {
  DCHECK(evb->isInEventBaseThread());
  if (httpSession_) {
    httpSession_->attachEventBase(evb, timeout_);
  }
  evb_ = evb;
}

void H2ClientConnection::detachEventBase() {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(isDetachable());
  if (httpSession_) {
    httpSession_->detachEventBase();
  }
  evb_ = nullptr;
}

bool H2ClientConnection::isDetachable() {
  return !httpSession_ || httpSession_->isDetachable();
}

bool H2ClientConnection::isSecurityActive() {
  return false;
}

uint32_t H2ClientConnection::getTimeout() {
  return timeout_.count();
}

void H2ClientConnection::setTimeout(uint32_t ms) {
  timeout_ = std::chrono::milliseconds(ms);
  // TODO: need to change timeout in httpSession_.  This functionality
  // is also missing in JiaJie's HTTPClientChannel.
}

void H2ClientConnection::closeNow() {
  if (httpSession_) {
    httpSession_->dropConnection();
    httpSession_ = nullptr;
  }
}

CLIENT_TYPE H2ClientConnection::getClientType() {
  return THRIFT_HTTP_CLIENT_TYPE;
}

} // namespace thrift
} // namespace apache
