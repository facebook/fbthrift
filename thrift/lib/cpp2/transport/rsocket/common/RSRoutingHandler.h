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

#include <rsocket/RSocketServer.h>
#include <rsocket/framing/FrameTransport.h>
#include <rsocket/rsocket.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>

namespace apache {
namespace thrift {

class RSRoutingHandler : public TransportRoutingHandler {
 public:
  explicit RSRoutingHandler(std::shared_ptr<ThriftServer> thriftServer);
  virtual ~RSRoutingHandler() = default;
  RSRoutingHandler(const RSRoutingHandler&) = delete;
  RSRoutingHandler& operator=(const RSRoutingHandler&) = delete;

  bool canAcceptEncryptedConnection(const std::string&) override {
    return false;
  }

  bool canAcceptConnection(const std::vector<uint8_t>& bytes) override;
  void handleConnection(
      wangle::ConnectionManager*,
      folly::AsyncTransportWrapper::UniquePtr sock,
      folly::SocketAddress* peerAddress,
      wangle::TransportInfo const& tinfo) override;

 private:
  std::shared_ptr<ThriftServer> thriftServer_;
  std::unique_ptr<rsocket::RSocketServer> rsocketServer_;
  ThriftProcessor* thriftProcessor_;
};
}
}
