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
#include <rsocket/transports/tcp/TcpDuplexConnection.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

#include <unordered_map>

namespace apache {
namespace thrift {

using namespace rsocket;
using apache::thrift::transport::TTransportException;

class RSConnectionStatus : public rsocket::RSocketConnectionEvents {
 public:
  void setCloseCallback(ThriftClient* client, CloseCallback* ccb) {
    if (ccb == nullptr) {
      closeCallbacks_.erase(client);
    } else {
      closeCallbacks_[client] = ccb;
    }
  }

  bool isConnected() const {
    return isConnected_;
  }

 private:
  void onConnected() override {
    isConnected_ = true;
  }

  void onDisconnected(const folly::exception_wrapper&) override {
    closed();
  }

  void onClosed(const folly::exception_wrapper&) override {
    closed();
  }

  void closed() {
    if (isConnected_) {
      isConnected_ = false;
      for (auto& cb : closeCallbacks_) {
        cb.second->channelClosed();
      }
      closeCallbacks_.clear();
    }
  }

  bool isConnected_{false};

  // A map of all registered CloseCallback objects keyed by the
  // ThriftClient objects that registered the callback.
  std::unordered_map<ThriftClient*, CloseCallback*> closeCallbacks_;
};

RSClientConnection::RSClientConnection(
    apache::thrift::async::TAsyncTransport::UniquePtr socket,
    bool isSecure)
    : evb_(socket->getEventBase()),
      isSecure_(isSecure),
      connectionStatus_(std::make_shared<RSConnectionStatus>()) {
  rsRequester_ =
      std::make_shared<RSRequester>(std::move(socket), evb_, connectionStatus_);

  channel_ =
      std::make_shared<RSClientThriftChannel>(rsRequester_, counters_, evb_);
}

RSClientConnection::~RSClientConnection() {
  if (rsRequester_) {
    evb_->runInEventBaseThread(
        [rsRequester = std::move(rsRequester_)]() { rsRequester->closeNow(); });
  }
}

std::shared_ptr<ThriftChannelIf> RSClientConnection::getChannel(
    RequestRpcMetadata*) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (!channel_ || !connectionStatus_->isConnected()) {
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
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  if (rsRequester_) {
    DuplexConnection* connection = rsRequester_->getConnection();
    if (!connection) {
      LOG_EVERY_N(ERROR, 100)
          << "Connection is already closed. May be protocol mismatch x 100";
      channel_.reset();
      rsRequester_.reset();
      return nullptr;
    }
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

void RSClientConnection::attachEventBase(folly::EventBase* evb) {
  DCHECK(evb->isInEventBaseThread());
  auto transport = getTransport();
  if (transport) {
    transport->attachEventBase(evb);
  }
  if (channel_) {
    channel_->attachEventBase(evb);
  }
  if (rsRequester_) {
    rsRequester_->attachEventBase(evb);
  }
  evb_ = evb;
}

void RSClientConnection::detachEventBase() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  auto transport = getTransport();
  if (transport) {
    transport->detachEventBase();
  }
  if (channel_) {
    channel_->detachEventBase();
  }
  if (rsRequester_) {
    rsRequester_->detachEventBase();
  }
  evb_ = nullptr;
}

bool RSClientConnection::isDetachable() {
  auto transport = getTransport();
  bool result = evb_ == nullptr || transport == nullptr ||
      channel_ == nullptr || rsRequester_ == nullptr ||
      (counters_.getPendingRequests() == 0 && transport->isDetachable() &&
       channel_->isDetachable() && rsRequester_->isDetachable());
  return result;
}

bool RSClientConnection::isSecurityActive() {
  return isSecure_;
}

uint32_t RSClientConnection::getTimeout() {
  // TODO: Need to inspect this functionality for RSocket
  return timeout_.count();
}

void RSClientConnection::setTimeout(uint32_t timeoutMs) {
  // TODO: Need to inspect this functionality for RSocket
  timeout_ = std::chrono::milliseconds(timeoutMs);
}

void RSClientConnection::closeNow() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (rsRequester_) {
    channel_.reset();
    rsRequester_->closeNow();
    rsRequester_.reset();
  }
}

CLIENT_TYPE RSClientConnection::getClientType() {
  // TODO: Should we use this value?
  return THRIFT_HTTP_CLIENT_TYPE;
}

void RSClientConnection::setCloseCallback(
    ThriftClient* client,
    CloseCallback* cb) {
  connectionStatus_->setCloseCallback(client, cb);
}

} // namespace thrift
} // namespace apache
