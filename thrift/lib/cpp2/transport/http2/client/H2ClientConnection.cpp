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

#include <folly/Likely.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <proxygen/lib/http/codec/HTTP1xCodec.h>
#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <proxygen/lib/http/codec/TransportDirection.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/utils/WheelTimerInstance.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/http2/client/ThriftTransactionHandler.h>
#include <wangle/acceptor/TransportInfo.h>
#include <algorithm>

// This flag is only used on the client side.
DEFINE_uint32(
    max_channel_version,
    3,
    "Maximum channel version to use for negotiation");

DEFINE_uint32(
    force_channel_version,
    0,
    "Set to a positive number to force this as the channel version");

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncTransport;
using apache::thrift::transport::TTransportException;
using folly::EventBase;
using proxygen::HTTPSessionBase;
using proxygen::SettingsList;
using proxygen::HTTPTransaction;
using proxygen::HTTPUpstreamSession;
using proxygen::WheelTimerInstance;
using std::string;

const std::chrono::milliseconds H2ClientConnection::kDefaultTimeout =
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
  return std::move(connection);
}

std::unique_ptr<ClientConnectionIf> H2ClientConnection::newHTTP2Connection(
    TAsyncTransport::UniquePtr transport,
    const string& httpHost,
    const string& httpUrl) {
  std::unique_ptr<H2ClientConnection> connection(new H2ClientConnection(
      std::move(transport),
      std::make_unique<proxygen::HTTP2Codec>(
          proxygen::TransportDirection::UPSTREAM)));
  connection->httpHost_ = httpHost;
  connection->httpUrl_ = httpUrl;
  return std::move(connection);
}

H2ClientConnection::H2ClientConnection(
    TAsyncTransport::UniquePtr transport,
    std::unique_ptr<proxygen::HTTPCodec> codec)
    : evb_(transport->getEventBase()),
      negotiatedChannelVersion_(FLAGS_force_channel_version),
      stable_(FLAGS_force_channel_version != 0) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  auto localAddress = transport->getLocalAddress();
  auto peerAddress = transport->getPeerAddress();
  httpSession_ = new HTTPUpstreamSession(
      WheelTimerInstance(timeout_, evb_),
      std::move(transport),
      localAddress,
      peerAddress,
      std::move(codec),
      wangle::TransportInfo(),
      this);
  // TODO: Improve the way max outging streams is set
  setMaxPendingRequests(100000);
  httpSession_->setEgressSettings(
      {{kChannelSettingId,
        std::min(kMaxSupportedChannelVersion, FLAGS_max_channel_version)}});
}

H2ClientConnection::~H2ClientConnection() {
  closeNow();
}

std::shared_ptr<ThriftChannelIf> H2ClientConnection::getChannel(
    RequestRpcMetadata* metadata) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return channelFactory_.getChannel(
      negotiatedChannelVersion_, this, httpHost_, httpUrl_, metadata);
}

void H2ClientConnection::setMaxPendingRequests(uint32_t num) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (httpSession_) {
    httpSession_->setMaxConcurrentOutgoingStreams(num);
  }
}

EventBase* H2ClientConnection::getEventBase() const {
  return evb_;
}

HTTPTransaction* H2ClientConnection::newTransaction(H2Channel* channel) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (!httpSession_) {
    throw TTransportException(
        TTransportException::NOT_OPEN, "HTTPSession is not open");
  }
  // These objects destroy themselves when done.
  auto handler = new ThriftTransactionHandler();
  auto txn = httpSession_->newTransaction(handler);
  if (!txn) {
    delete handler;
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    throw ex;
  }
  handler->setChannel(
      std::dynamic_pointer_cast<H2Channel>(channel->shared_from_this()));
  return txn;
}

bool H2ClientConnection::isStable() {
  return stable_;
}

void H2ClientConnection::setIsStable() {
  stable_ = true;
}

TAsyncTransport* H2ClientConnection::getTransport() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (httpSession_) {
    return dynamic_cast<TAsyncTransport*>(httpSession_->getTransport());
  } else {
    return nullptr;
  }
}

bool H2ClientConnection::good() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  auto transport = httpSession_ ? httpSession_->getTransport() : nullptr;
  return transport && transport->good();
}

ClientChannel::SaturationStatus H2ClientConnection::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
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
    httpSession_->attachThreadLocals(
        evb,
        nullptr,
        WheelTimerInstance(timeout_, evb),
        nullptr,
        [](proxygen::HTTPCodecFilter*) {},
        nullptr,
        nullptr);
  }
  evb_ = evb;
}

void H2ClientConnection::detachEventBase() {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(isDetachable());
  if (httpSession_) {
    httpSession_->detachThreadLocals();
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
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (httpSession_) {
    httpSession_->dropConnection();
    httpSession_ = nullptr;
  }
}

CLIENT_TYPE H2ClientConnection::getClientType() {
  return THRIFT_HTTP_CLIENT_TYPE;
}

void H2ClientConnection::onDestroy(const HTTPSessionBase& /*session*/) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  httpSession_ = nullptr;
}

void H2ClientConnection::onSettings(
    const HTTPSessionBase&,
    const SettingsList& settings) {
  if (FLAGS_force_channel_version > 0) {
    // Do not use the negotiated settings.
    return;
  }
  for (auto& setting : settings) {
    if (setting.id == kChannelSettingId) {
      negotiatedChannelVersion_ = std::min(
          setting.value,
          std::min(kMaxSupportedChannelVersion, FLAGS_max_channel_version));
      VLOG(2) << "Peer channel version is " << setting.value;
      VLOG(2) << "Negotiated channel version is " << negotiatedChannelVersion_;
    }
  }
  if (negotiatedChannelVersion_ == 0) {
    // Did not receive a channel version, assuming legacy peer.
    negotiatedChannelVersion_ = 1;
  }
}

} // namespace thrift
} // namespace apache
