/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/TransportRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/SetupFrameHandler.h>

namespace apache {
namespace thrift {

class Cpp2Worker;

class RocketRoutingHandler : public TransportRoutingHandler {
 public:
  RocketRoutingHandler();
  ~RocketRoutingHandler() override;
  RocketRoutingHandler(const RocketRoutingHandler&) = delete;
  RocketRoutingHandler& operator=(const RocketRoutingHandler&) = delete;

  void stopListening() override;
  bool canAcceptConnection(const std::vector<uint8_t>& bytes) override;
  bool canAcceptEncryptedConnection(const std::string& protocolName) override;
  void handleConnection(
      wangle::ConnectionManager* connectionManager,
      folly::AsyncTransport::UniquePtr sock,
      folly::SocketAddress const* peerAddress,
      wangle::TransportInfo const& tinfo,
      std::shared_ptr<Cpp2Worker> worker) override;

  void addSetupFrameHandler(std::unique_ptr<rocket::SetupFrameHandler> handler);
  const std::vector<std::unique_ptr<rocket::SetupFrameHandler>>&
  getSetupFrameHandlers() const;

 private:
  std::atomic<bool> listening_{true};
  std::vector<std::unique_ptr<rocket::SetupFrameHandler>> setupFrameHandlers_;
};
} // namespace thrift
} // namespace apache
