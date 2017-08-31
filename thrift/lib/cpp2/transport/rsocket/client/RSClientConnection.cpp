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

#include <rsocket/transports/tcp/TcpConnectionFactory.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h>

namespace apache {
namespace thrift {

using namespace rsocket;

RSClientConnection::RSClientConnection(
    folly::EventBase& evb,
    folly::SocketAddress address)
    : evb_(evb) {
  rsClient_ =
      RSocket::createConnectedClient(
          std::make_unique<TcpConnectionFactory>(evb, std::move(address)))
          .get();
  rsRequester_ = rsClient_->getRequester();
}

std::shared_ptr<ThriftChannelIf> RSClientConnection::getChannel() {
  return std::make_shared<RSClientThriftChannel>(rsRequester_);
}

void RSClientConnection::setMaxPendingRequests(uint32_t) {
  LOG(FATAL) << "not implemented";
}

folly::EventBase* RSClientConnection::getEventBase() const {
  return &evb_;
}

apache::thrift::async::TAsyncTransport* RSClientConnection::getTransport() {
  LOG(FATAL) << "not implemented";
}

bool RSClientConnection::good() {
  LOG(FATAL) << "not implemented";
}

ClientChannel::SaturationStatus RSClientConnection::getSaturationStatus() {
  LOG(FATAL) << "not implemented";
}

void RSClientConnection::attachEventBase(folly::EventBase*) {
  LOG(FATAL) << "not implemented";
}

void RSClientConnection::detachEventBase() {
  LOG(FATAL) << "not implemented";
}

bool RSClientConnection::isDetachable() {
  LOG(FATAL) << "not implemented";
}

bool RSClientConnection::isSecurityActive() {
  return false; // ?
}

uint32_t RSClientConnection::getTimeout() {
  LOG(FATAL) << "not implemented";
}

void RSClientConnection::setTimeout(uint32_t) {
  LOG(FATAL) << "not implemented";
}

void RSClientConnection::closeNow() {
  LOG(FATAL) << "not implemented";
}

CLIENT_TYPE RSClientConnection::getClientType() {
  LOG(FATAL) << "not implemented";
}
}
}
