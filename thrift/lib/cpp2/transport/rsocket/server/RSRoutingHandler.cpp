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

#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/rsocket/server/ManagedRSocketConnection.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

using namespace rsocket;

namespace apache {
namespace thrift {

RSRoutingHandler::RSRoutingHandler(
    apache::thrift::ThriftProcessor* thriftProcessor,
    const apache::thrift::server::ServerConfigs& serverConfigs)
    : thriftProcessor_(thriftProcessor), serverConfigs_(serverConfigs) {}

RSRoutingHandler::~RSRoutingHandler() {
  stopListening();
}

void RSRoutingHandler::stopListening() {
  listening_ = false;
}

bool RSRoutingHandler::canAcceptConnection(const std::vector<uint8_t>& bytes) {
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
        * bitshifted by << 2 before stored. Thus, SETUP becomes 0x04.
        *
        * For more, see:
        * https://github.com/rsocket/rsocket/blob/master/Protocol.md#frame-types
        */
       // setupFrame
       && bytes[7] == 0x04);
}

bool RSRoutingHandler::canAcceptEncryptedConnection(
    const std::string& protocolName) {
  return listening_ && protocolName == "rs";
}

void RSRoutingHandler::handleConnection(
    wangle::ConnectionManager* connectionManager,
    folly::AsyncTransportWrapper::UniquePtr sock,
    folly::SocketAddress const*,
    wangle::TransportInfo const&) {
  if (!listening_) {
    return;
  }

  auto managedConnection =
      new ManagedRSocketConnection(std::move(sock), [&](auto&) {
        // RSResponder will be created per client connection. It will use the
        // current Observer of the server.
        return std::make_shared<RSResponder>(
            thriftProcessor_,
            folly::EventBaseManager::get()->getExistingEventBase(),
            serverConfigs_.getObserver());
      });

  connectionManager->addConnection(managedConnection);

  auto observer = serverConfigs_.getObserver();
  if (observer) {
    observer->connAccepted();
    observer->activeConnections(
        connectionManager->getNumConnections() *
        serverConfigs_.getNumIOWorkerThreads());
  }
}
} // namespace thrift
} // namespace apache
