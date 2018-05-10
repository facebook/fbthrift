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

#include <gflags/gflags.h>
#include <proxygen/httpserver/HTTPServerAcceptor.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerAdaptor.h>
#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <proxygen/lib/http/codec/HTTPSettings.h>
#include <proxygen/lib/http/session/HTTPDefaultSessionCodecFactory.h>
#include <proxygen/lib/http/session/HTTPDownstreamSession.h>
#include <proxygen/lib/http/session/HTTPSession.h>
#include <proxygen/lib/http/session/SimpleController.h>
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelFactory.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandler.h>
#include <wangle/acceptor/ManagedConnection.h>
#include <limits>

DECLARE_uint32(force_channel_version);

DEFINE_uint32(stream_timeout_ms, 1000, "Stream timeout in milliseconds");

namespace apache {
namespace thrift {

using std::chrono::milliseconds;

namespace {
// Class for managing lifetime of objects supporting an HTTP2 session.
class HTTP2RoutingSessionManager : public proxygen::HTTPSession::InfoCallback,
                                   public proxygen::SimpleController {
 public:
  HTTP2RoutingSessionManager(
      std::unique_ptr<proxygen::HTTPServerAcceptor> acceptor,
      ThriftProcessor* processor)
      : proxygen::HTTPSession::InfoCallback(),
        proxygen::SimpleController(acceptor.get()),
        processor_(processor),
        negotiatedChannelVersion_(FLAGS_force_channel_version) {
    acceptor_ = std::move(acceptor);
    if (FLAGS_force_channel_version > 0) {
      // This prevents the inspection of the HTTP2 header for the channel
      // version.
      stableId_ = 0;
    } else {
      stableId_ = std::numeric_limits<proxygen::HTTPCodec::StreamID>::max();
    }
  }

  ~HTTP2RoutingSessionManager() = default;

  proxygen::HTTPDownstreamSession* createSession(
      folly::AsyncTransportWrapper::UniquePtr sock,
      folly::SocketAddress* peerAddress,
      std::unique_ptr<proxygen::HTTPCodec> h2codec,
      wangle::TransportInfo const& tinfo) {
    // Obtain the proper routing address
    folly::SocketAddress localAddress;
    try {
      sock->getLocalAddress(&localAddress);
    } catch (...) {
      VLOG(3) << "couldn't get local address for socket";
      localAddress = folly::SocketAddress("0.0.0.0", 0);
    }
    VLOG(4) << "Created new session for peer " << *peerAddress;

    // Create the DownstreamSession.  Note that "this" occurs twice
    // because it acts as both a controller as well as a info
    // callback.
    auto session = new proxygen::HTTPDownstreamSession(
        proxygen::WheelTimerInstance(milliseconds(FLAGS_stream_timeout_ms)),
        std::move(sock),
        localAddress,
        *peerAddress,
        this,
        std::move(h2codec),
        tinfo,
        this);

    return session;
  }

  // begin HTTPSession::InfoCallback methods

  // We do not override onDestroy() to self destroy because this object
  // doubles as both the InfoCallback and the SimpleController.  The
  // session destructor calls onDestroy() first and then detachSession()
  // so we self destroy at detachSession().

  void onSettings(
      const proxygen::HTTPSessionBase&,
      const proxygen::SettingsList& settings) override {
    if (FLAGS_force_channel_version > 0) {
      // Do not use the negotiated settings.
      return;
    }
    for (auto& setting : settings) {
      if (setting.id == proxygen::SettingsId::THRIFT_CHANNEL_ID_DEPRECATED ||
          setting.id == proxygen::SettingsId::THRIFT_CHANNEL_ID) {
        negotiatedChannelVersion_ =
            std::min(setting.value, kMaxSupportedChannelVersion);
        VLOG(3) << "Peer channel version is " << setting.value << "; "
                << "Negotiated channel version is "
                << negotiatedChannelVersion_;
      }
    }
    if (negotiatedChannelVersion_ == 0) {
      // Did not receive a channel version, assuming legacy peer.
      negotiatedChannelVersion_ = 1;
    }
  }

  // end HTTPSession::InfoCallback methods

  // begin SimpleController methods

  proxygen::HTTPTransactionHandler* getRequestHandler(
      proxygen::HTTPTransaction& txn,
      proxygen::HTTPMessage* msg) override {
    folly::SocketAddress clientAddr, vipAddr;
    txn.getPeerAddress(clientAddr);
    txn.getLocalAddress(vipAddr);
    msg->setClientAddress(clientAddr);
    msg->setDstAddress(vipAddr);

    // This checks that the SETTINGS frame arrives before the first RPC.
    DCHECK(negotiatedChannelVersion_ > 0);

    // Determine channel version for this HTTP2 stream.
    uint32_t version = negotiatedChannelVersion_;
    if (UNLIKELY(txn.getID() < stableId_)) {
      auto val = msg->getHeaders().rawGet(kChannelVersionKey);
      try {
        version = folly::to<int>(val);
      } catch (const std::exception& ex) {
        LOG(WARNING) << "Channel version not set properly in header: " << val;
        // This could be from a legacy client.
        version = 1;
      }
      DCHECK(version == 1 || version == negotiatedChannelVersion_);
      if (version == negotiatedChannelVersion_) {
        stableId_ = txn.getID();
      }
    }

    proxygen::RequestHandler* handler =
        new ThriftRequestHandler(processor_, version);
    return new proxygen::RequestHandlerAdaptor(handler);
  }

  void detachSession(const proxygen::HTTPSessionBase*) override {
    VLOG(4) << "HTTP2RoutingSessionManager::detachSession";
    // Session destroyed, so self destroy.
    delete this;
  }

  // end SimpleController methods

 private:
  // Supporting objects for HTTP2 session managed by the callback.
  std::unique_ptr<proxygen::HTTPServerAcceptor> acceptor_;

  ThriftProcessor* processor_;

  // The negotiated channel version - 0 means negotiation has not
  // taken place yet.  Negotiation is completed when the server
  // receives a header with a non-zero channel version.
  uint32_t negotiatedChannelVersion_;

  // The stream id after which the server can assume that the channel
  // version will be the negotiated version.
  proxygen::HTTPCodec::StreamID stableId_;
};

} // anonymous namespace

void HTTP2RoutingHandler::stopListening() {
  listening_ = false;
}

bool HTTP2RoutingHandler::canAcceptConnection(
    const std::vector<uint8_t>& bytes) {
  return listening_ &&
      /*
       * HTTP/2.0 requests start with the following sequence:
       *   Octal: 0x505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
       *  String: "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
       *
       * For more, see: https://tools.ietf.org/html/rfc7540#section-3.5
       */
      ((bytes[0] == 0x50 && bytes[1] == 0x52 && bytes[2] == 0x49) ||

       /*
        * HTTP requests start with the following sequence:
        *   Octal: "0x485454502f..."
        *  String: "HTTP/X.X"
        *
        * For more, see: https://tools.ietf.org/html/rfc2616#section-3
        */
       (bytes[0] == 0x48 && bytes[1] == 0x54 && bytes[2] == 0x54));
}

bool HTTP2RoutingHandler::canAcceptEncryptedConnection(
    const std::string& protocolName) {
  return listening_ && (protocolName == "h2" || protocolName == "http");
}

void HTTP2RoutingHandler::handleConnection(
    wangle::ConnectionManager* connectionManager,
    folly::AsyncTransportWrapper::UniquePtr sock,
    folly::SocketAddress const* peerAddress,
    wangle::TransportInfo const& tinfo) {
  // Create the DownstreamSession manager.
  auto ipConfig = proxygen::HTTPServer::IPConfig(
      *peerAddress, proxygen::HTTPServer::Protocol::HTTP2);
  auto acceptorConfig =
      proxygen::HTTPServerAcceptor::makeConfig(ipConfig, *options_);
  auto acceptor = proxygen::HTTPServerAcceptor::make(acceptorConfig, *options_);
  auto sessionManager =
      new HTTP2RoutingSessionManager(std::move(acceptor), processor_);
  // Get the HTTP2 Codec
  auto codecFactory = proxygen::HTTPDefaultSessionCodecFactory(acceptorConfig);
  auto h2codec = codecFactory.getCodec(
      "h2",
      proxygen::TransportDirection::DOWNSTREAM,
      // a non empty security protocol is assumed to be TLS
      !sock->getSecurityProtocol().empty());
  // Create the DownstreamSession
  // A const_cast is needed to match wangle and proxygen APIs
  auto session = sessionManager->createSession(
      std::move(sock),
      const_cast<folly::SocketAddress*>(peerAddress),
      std::move(h2codec),
      tinfo);
  // Set HTTP2 priorities flag on session object.
  session->setHTTP2PrioritiesEnabled(acceptorConfig.HTTP2PrioritiesEnabled);
  /*
  if (acceptorConfig.maxConcurrentIncomingStreams) {
    session->setMaxConcurrentIncomingStreams(
        acceptorConfig.maxConcurrentIncomingStreams);
  }
  */
  // TODO: Improve the way max incoming streams is set
  session->setMaxConcurrentIncomingStreams(100000);

  // Set flow control parameters.
  session->setFlowControl(
      acceptorConfig.initialReceiveWindow,
      acceptorConfig.receiveStreamWindowSize,
      acceptorConfig.receiveSessionWindowSize);
  if (acceptorConfig.writeBufferLimit > 0) {
    session->setWriteBufferLimit(acceptorConfig.writeBufferLimit);
  }
  session->setEgressSettings(
      {{proxygen::SettingsId::THRIFT_CHANNEL_ID_DEPRECATED,
        kMaxSupportedChannelVersion},
       {proxygen::SettingsId::THRIFT_CHANNEL_ID, kMaxSupportedChannelVersion}});

  // Route the connection.
  connectionManager->addConnection(session);
  session->startNow();

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
