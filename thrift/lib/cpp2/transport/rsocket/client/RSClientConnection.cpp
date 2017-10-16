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
#include <rsocket/transports/tcp/TcpConnectionFactory.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h>

namespace apache {
namespace thrift {

using namespace rsocket;

static constexpr std::chrono::milliseconds kDefaultTimeout =
    std::chrono::milliseconds(5000);

RSClientConnection::RSClientConnection(
    folly::AsyncSocket::UniquePtr socket,
    folly::EventBase* evb,
    bool isSecure)
    : evb_(evb), timeout_(kDefaultTimeout), isSecure_(isSecure) {
  rsClient_ = RSocket::createClientFromConnection(
      TcpConnectionFactory::createDuplexConnectionFromSocket(std::move(socket)),
      *evb);
  rsRequester_ = rsClient_->getRequester();
}

std::shared_ptr<ThriftChannelIf> RSClientConnection::getChannel() {
  return std::make_shared<RSClientThriftChannel>(rsRequester_);
}

void RSClientConnection::setMaxPendingRequests(uint32_t) {
  LOG(FATAL) << "not implemented";
}

folly::EventBase* RSClientConnection::getEventBase() const {
  return evb_;
}

apache::thrift::async::TAsyncTransport* RSClientConnection::getTransport() {
  LOG(FATAL) << "not implemented";
}

bool RSClientConnection::good() {
  // TODO: Temporary implementation.
  return true;
}

ClientChannel::SaturationStatus RSClientConnection::getSaturationStatus() {
  LOG(FATAL) << "not implemented";
}

void RSClientConnection::attachEventBase(folly::EventBase*) {
  DLOG(FATAL) << "not implemented, just ignore";
}

void RSClientConnection::detachEventBase() {
  DLOG(FATAL) << "not implemented, just ignore";
}

bool RSClientConnection::isDetachable() {
  return false;
}

bool RSClientConnection::isSecurityActive() {
  return isSecure_;
}

uint32_t RSClientConnection::getTimeout() {
  return timeout_.count();
}

void RSClientConnection::setTimeout(uint32_t ms) {
  // TODO: update rsClient_'s timeout
  timeout_ = std::chrono::milliseconds(ms);
}

void RSClientConnection::closeNow() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  if (rsClient_) {
    rsClient_->disconnect();
    rsClient_.reset();
  }
}

CLIENT_TYPE RSClientConnection::getClientType() {
  // TODO: Should we use this value?
  return THRIFT_HTTP_CLIENT_TYPE;
}
} // namespace thrift
} // namespace apache
