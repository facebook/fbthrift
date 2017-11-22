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
#include <rsocket/transports/tcp/TcpDuplexConnection.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

using namespace rsocket;

namespace apache {
namespace thrift {

RSRoutingHandler::RSRoutingHandler(
    apache::thrift::ThriftProcessor* thriftProcessor,
    std::shared_ptr<RSocketStats> stats)
    : thriftProcessor_(thriftProcessor) {
  serviceHandler_ = RSocketServiceHandler::create([&](auto&) {
    return std::make_shared<RSResponder>(
        thriftProcessor_,
        folly::EventBaseManager::get()->getExistingEventBase());
  });

  rsocketServer_ = RSocket::createServer(nullptr, std::move(stats));
  rsocketServer_->setSingleThreadedResponder();
}

RSRoutingHandler::~RSRoutingHandler() {
  if (rsocketServer_) {
    rsocketServer_->shutdownAndWait();
    rsocketServer_.reset();
  }
}

bool RSRoutingHandler::canAcceptConnection(const std::vector<uint8_t>& bytes) {
  return
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
      (bytes[9] == 0x00 && bytes[10] == 0x01 && bytes[11] == 0x00 &&
       bytes[12] == 0x00)

      /*
       * SETUP frames have a frame type value of 0x01.
       * RESUME frames have a frame type value of 0x0D.
       *
       * Since the frame type is specified in only 6 bits, the value is
       * bitshifted by << 2 before stored. Thus, SETUP becomes 0x04, and
       * RESUME becomes 0x34.
       *
       * For more, see:
       * https://github.com/rsocket/rsocket/blob/master/Protocol.md#frame-types
       */
      // setupOrResumeFrame
      && (bytes[7] == 0x04 || bytes[7] == 0x34);
}

bool RSRoutingHandler::canAcceptEncryptedConnection(
    const std::string& protocolName) {
  return protocolName == "rs";
}

void RSRoutingHandler::handleConnection(
    wangle::ConnectionManager*,
    folly::AsyncTransportWrapper::UniquePtr sock,
    folly::SocketAddress const*,
    wangle::TransportInfo const&) {
  auto connection = std::make_unique<TcpDuplexConnection>(
      std::
          unique_ptr<folly::AsyncSocket, folly::DelayedDestruction::Destructor>(
              dynamic_cast<folly::AsyncSocket*>(sock.release())));

  // TODO T21601758: RSocketServer's acceptConnection method takes an eventBase
  // as input, but it does not use it at all. We should get rid of it.
  rsocketServer_->acceptConnection(
      std::move(connection), dummyEventBase_, serviceHandler_);
}
} // namespace thrift
} // namespace apache
