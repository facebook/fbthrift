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

#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>

#include <proxygen/lib/http/session/HTTPDefaultSessionCodecFactory.h>
#include <proxygen/lib/http/session/HTTPDownstreamSession.h>

namespace apache {
namespace thrift {

bool HTTP2RoutingHandler::canAcceptConnection(
    const std::vector<uint8_t>& bytes) {
  /*
   * HTTP/2.0 requests start with the following sequence:
   *   Octal: 0x505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
   *  String: "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
   *
   * For more, see: https://tools.ietf.org/html/rfc7540#section-3.5
   */
  if (bytes[0] == 0x50 && bytes[1] == 0x52 && bytes[2] == 0x49) {
    return true;
  }

  /*
   * HTTP requests start with the following sequence:
   *   Octal: "0x485454502f..."
   *  String: "HTTP/X.X"
   *
   * For more, see: https://tools.ietf.org/html/rfc2616#section-3
   */
  if (bytes[0] == 0x48 && bytes[1] == 0x54 && bytes[2] == 0x54) {
    return true;
  }

  return false;
}

void HTTP2RoutingHandler::handleConnection(
    folly::AsyncTransportWrapper::UniquePtr sock,
    folly::SocketAddress* peerAddress,
    wangle::TransportInfo const& tinfo) {
  // Create the SimpleController
  auto ipConfig = proxygen::HTTPServer::IPConfig(
      *peerAddress, proxygen::HTTPServer::Protocol::HTTP2);
  auto acceptorConfig =
      proxygen::HTTPServerAcceptor::makeConfig(ipConfig, *options_);
  serverAcceptor_ =
      proxygen::HTTPServerAcceptor::make(acceptorConfig, *options_);
  controller_.reset(new proxygen::SimpleController(serverAcceptor_.get()));

  // Get the HTTP2 Codec
  auto codecFactory = proxygen::HTTPDefaultSessionCodecFactory(acceptorConfig);
  auto h2codec =
      codecFactory.getCodec("h2", proxygen::TransportDirection::DOWNSTREAM);

  // Obtain the proper routing address
  folly::SocketAddress localAddress;
  try {
    sock->getLocalAddress(&localAddress);
  } catch (...) {
    VLOG(3) << "couldn't get local address for socket";
    localAddress = folly::SocketAddress("0.0.0.0", 0);
  }
  VLOG(4) << "Created new session for peer " << *peerAddress;

  // Set an empty InfoCallback
  sessionInfoCb_.reset(new proxygen::HTTPSession::EmptyInfoCallback());

  // Create the DownstreamSession
  auto* session = new proxygen::HTTPDownstreamSession(
      proxygen::WheelTimerInstance(std::chrono::milliseconds(5)),
      std::move(sock),
      localAddress,
      *peerAddress,
      controller_.get(),
      std::move(h2codec),
      tinfo,
      sessionInfoCb_.get());
  if (acceptorConfig.maxConcurrentIncomingStreams) {
    session->setMaxConcurrentIncomingStreams(
        acceptorConfig.maxConcurrentIncomingStreams);
  }

  // Set HTTP2 priorities flag on session object.
  session->setHTTP2PrioritiesEnabled(acceptorConfig.HTTP2PrioritiesEnabled);

  // Set flow control parameters.
  session->setFlowControl(
      acceptorConfig.initialReceiveWindow,
      acceptorConfig.receiveStreamWindowSize,
      acceptorConfig.receiveSessionWindowSize);
  if (acceptorConfig.writeBufferLimit > 0) {
    session->setWriteBufferLimit(acceptorConfig.writeBufferLimit);
  }

  // Route the connection.
  connectionManager_->addConnection(session);
  session->startNow();
}

} // namspace thrift
} // namespace apache
