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

#include <thrift/lib/cpp2/transport/rocket/server/RocketRoutingHandler.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncTransport.h>

#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/ThriftRocketServerHandler.h>

namespace apache {
namespace thrift {

RocketRoutingHandler::RocketRoutingHandler() {}

RocketRoutingHandler::~RocketRoutingHandler() {
  stopListening();
}

void RocketRoutingHandler::stopListening() {
  listening_ = false;
}

bool RocketRoutingHandler::canAcceptConnection(
    const std::vector<uint8_t>& bytes) {
  return listening_ &&
      /*
       * Sample start of an Rsocket frame (version 1.0) in Octal:
       * 0x0000 2800 0000 0004 0000 0100 00....
       * Rsocket frame length - 24 bits
       * StreamId             - 32 bits
       * Frame type           -  6 bits
       * Flags                - 10 bits
       * Major version        - 16 bits
       * Minor version        - 16 bits
       */

      // This only supports Rsocket protocol version 1.0
      ((bytes[9] == 0x00 && bytes[10] == 0x01 && bytes[11] == 0x00 &&
        bytes[12] == 0x00)

       /*
        * SETUP frames have a frame type value of 0x01.
        *
        * Since the frame type is specified in only 6 bits, the value is
        * bitshifted by << 2 before stored. Thus, even though SETUP frame
        * is 0x01, it becomes 0x04. Furthermore, as METADATA flag is set
        * it becomes 0x05.
        *
        * For more, see:
        * https://github.com/rsocket/rsocket/blob/master/Protocol.md#frame-types
        */
       // setupFrame
       && bytes[7] == 0x05);
}

bool RocketRoutingHandler::canAcceptEncryptedConnection(
    const std::string& protocolName) {
  return listening_ && protocolName == "rs";
}

void RocketRoutingHandler::handleConnection(
    wangle::ConnectionManager* connectionManager,
    folly::AsyncTransport::UniquePtr sock,
    folly::SocketAddress const* address,
    wangle::TransportInfo const&,
    std::shared_ptr<Cpp2Worker> worker) {
  if (!listening_) {
    return;
  }

  auto* const sockPtr = sock.get();
  auto* const server = worker->getServer();
  auto* const connection = new rocket::RocketServerConnection(
      std::move(sock),
      std::make_unique<rocket::ThriftRocketServerHandler>(
          worker, *address, sockPtr, setupFrameHandlers_),
      server->getStreamExpireTime(),
      server->getWriteBatchingInterval(),
      server->getWriteBatchingSize());
  // set negotiated compression algorithm on this connection
  auto compression = static_cast<FizzPeeker*>(worker->getFizzPeeker())
                         ->getNegotiatedParameters()
                         .compression;
  if (compression != CompressionAlgorithm::NONE) {
    connection->setNegotiatedCompressionAlgorithm(compression);
  }
  connectionManager->addConnection(connection);

  if (auto* observer = server->getObserver()) {
    observer->connAccepted();
    observer->activeConnections(
        connectionManager->getNumConnections() *
        server->getNumIOWorkerThreads());
  }
}

void RocketRoutingHandler::addSetupFrameHandler(
    std::unique_ptr<rocket::SetupFrameHandler> handler) {
  setupFrameHandlers_.push_back(std::move(handler));
}

const std::vector<std::unique_ptr<rocket::SetupFrameHandler>>&
RocketRoutingHandler::getSetupFrameHandlers() const {
  return setupFrameHandlers_;
}

} // namespace thrift
} // namespace apache
