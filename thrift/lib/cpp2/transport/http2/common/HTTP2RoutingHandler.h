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

#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>

#include <proxygen/httpserver/HTTPServerAcceptor.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/lib/http/session/HTTPSession.h>
#include <proxygen/lib/http/session/SimpleController.h>

namespace apache {
namespace thrift {

/*
 * This handler is used to determine if a client is talking HTTP2 and
 * routes creates the handler to route the socket to Proxygen
 */
class HTTP2RoutingHandler : public TransportRoutingHandler {
 public:
  explicit HTTP2RoutingHandler(
      std::unique_ptr<proxygen::HTTPServerOptions> options)
      : options_(std::move(options)) {}
  virtual ~HTTP2RoutingHandler() = default;
  HTTP2RoutingHandler(const HTTP2RoutingHandler&) = delete;

  bool canAcceptConnection(const std::vector<uint8_t>& bytes) override;
  bool canAcceptEncryptedConnection(const std::string& protocolName) override;
  void handleConnection(
      folly::AsyncTransportWrapper::UniquePtr sock,
      folly::SocketAddress* peerAddress,
      wangle::TransportInfo const& tinfo) override;

  void setConnectionManager(
      wangle::ConnectionManager* connectionManager) override {
    connectionManager_ = connectionManager;
  }

 private:
  // ConnectionManager is the object that handles the Server Acceptors.
  // This object is set within the Acceptor once it's determined that this
  // is the appropriate handler to handle the client code.
  wangle::ConnectionManager* connectionManager_;

  // HTTPServerOptions are set outside out HTTP2RoutingHandler.
  // Since one of the internal members of this class is a unique_ptr
  // we need to set this object as a unique_ptr as well in order to properly
  // move it into the class.
  std::unique_ptr<proxygen::HTTPServerOptions> options_;

  // All of these objects are used by Proxygen.
  // We set them as unique_ptrs to own them and avoid going out of scope.
  std::unique_ptr<proxygen::SimpleController> controller_;
  std::unique_ptr<proxygen::HTTPServerAcceptor> serverAcceptor_;
  std::unique_ptr<proxygen::HTTPSession::EmptyInfoCallback> sessionInfoCb_;
};

} // namspace thrift
} // namespace apache
