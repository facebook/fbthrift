/*
 * Copyright 2004-present Facebook, Inc.
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

#pragma once

#include <vector>

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncTransport.h>
#include <wangle/acceptor/ConnectionManager.h>
#include <wangle/acceptor/TransportInfo.h>

namespace apache {
namespace thrift {

/*
 * An interface used by ThriftServer to route the
 * socket to different Transports.
 */
class TransportRoutingHandler {
 public:
  TransportRoutingHandler() = default;
  virtual ~TransportRoutingHandler() = default;
  TransportRoutingHandler(const TransportRoutingHandler&) = delete;

  /*
   * Performs a check on the first bytes read from the wire
   * and determines if this protocol is supported by this routing handler
   */
  virtual bool canAcceptConnection(const std::vector<uint8_t>& bytes) = 0;

  /*
   * Determines if the protocol indicated by the protocol name is supported by
   * this routing handler.
   */
  virtual bool canAcceptEncryptedConnection(
      const std::string& protocolName) = 0;

  /*
   * Sets the ConnectionManager that will be used to route the socket
   */
  virtual void setConnectionManager(
      wangle::ConnectionManager* connectionManager) = 0;

  /*
   * Creates the correct session to route the socket to the appropriate
   * protocol handler
   */
  virtual void handleConnection(
      folly::AsyncTransportWrapper::UniquePtr sock,
      folly::SocketAddress* peerAddress,
      wangle::TransportInfo const& tinfo) = 0;
};

} // namspace thrift
} // namespace apache
