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
#include "thrift/lib/cpp2/transport/rsocket/client/RSClientConnection.h"

#include <folly/io/async/EventBase.h>
#include <rsocket/framing/FramedDuplexConnection.h>
#include <rsocket/transports/tcp/TcpConnectionFactory.h>
#include <rsocket/transports/tcp/TcpDuplexConnection.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

namespace apache {
namespace thrift {

using namespace rsocket;
using apache::thrift::transport::TTransportException;

RSClientConnection::RSClientConnection(
    apache::thrift::async::TAsyncTransport::UniquePtr socket,
    folly::EventBase* evb,
    bool isSecure)
    : evb_(evb), isSecure_(isSecure) {
  rsClient_ = RSocket::createClientFromConnection(
      TcpConnectionFactory::createDuplexConnectionFromSocket(std::move(socket)),
      *evb,
      SetupParameters(),
      nullptr, /* ConnectionFactory */
      std::make_shared<RSocketResponder>(),
      std::chrono::milliseconds{0} /* no keepalive timeout */
  );
  rsRequester_ = rsClient_->getRequester();
  channel_ = std::make_shared<RSClientThriftChannel>(rsRequester_, counters_);
}

std::shared_ptr<ThriftChannelIf> RSClientConnection::getChannel() {
  if (!channel_) {
    throw TTransportException(
        TTransportException::NOT_OPEN, "Connection is not open");
  }
  return channel_;
}

void RSClientConnection::setMaxPendingRequests(uint32_t count) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  counters_.setMaxPendingRequests(count);
}

folly::EventBase* RSClientConnection::getEventBase() const {
  return evb_;
}

apache::thrift::async::TAsyncTransport* FOLLY_NULLABLE
RSClientConnection::getTransport() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (rsRequester_) {
    DuplexConnection* connection = rsRequester_->getConnection();
    if (auto framedConnection =
            dynamic_cast<FramedDuplexConnection*>(connection)) {
      connection = framedConnection->getConnection();
    }
    auto* tcpConnection = dynamic_cast<TcpDuplexConnection*>(connection);
    CHECK_NOTNULL(tcpConnection);
    return dynamic_cast<apache::thrift::async::TAsyncTransport*>(
        tcpConnection->getTransport());
  }
  return nullptr;
}

bool RSClientConnection::good() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  auto const socket = getTransport();
  return channel_ && socket && socket->good();
}

ClientChannel::SaturationStatus RSClientConnection::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return ClientChannel::SaturationStatus(
      counters_.getPendingRequests(), counters_.getMaxPendingRequests());
}

void RSClientConnection::attachEventBase(folly::EventBase* /*evb*/) {
  LOG(FATAL) << "RSClientConnection::attachEventBase()";
}

void RSClientConnection::detachEventBase() {
  LOG(FATAL) << "RSClientConnection::detachEventBase()";
}

bool RSClientConnection::isDetachable() {
  return false;
}

bool RSClientConnection::isSecurityActive() {
  return isSecure_;
}

uint32_t RSClientConnection::getTimeout() {
  return counters_.getRequestTimeout().count();
}

void RSClientConnection::setTimeout(uint32_t ms) {
  counters_.setRequestTimeout(std::chrono::milliseconds(ms));
}

void RSClientConnection::closeNow() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (rsClient_) {
    channel_.reset();
    rsClient_->disconnect().get();
    rsRequester_->closeSocket();
    rsRequester_.reset();
    rsClient_.reset();
  }
}

CLIENT_TYPE RSClientConnection::getClientType() {
  // TODO: Should we use this value?
  return THRIFT_HTTP_CLIENT_TYPE;
}
} // namespace thrift
} // namespace apache
