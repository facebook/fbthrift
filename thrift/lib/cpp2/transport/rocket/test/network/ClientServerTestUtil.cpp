/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/test/network/ClientServerTestUtil.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/futures/helpers.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>

#include <rsocket/RSocket.h>
#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/ServerSocketConfig.h>
#include <yarpl/Flowable.h>
#include <yarpl/Single.h>

#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerStreamSubscriber.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

using namespace rsocket;
using namespace yarpl::flowable;
using namespace yarpl::single;

namespace apache {
namespace thrift {
namespace rocket {
namespace test {

namespace {
class RsocketTestServerResponder : public rsocket::RSocketResponder {
 public:
  std::shared_ptr<Single<rsocket::Payload>> handleRequestResponse(
      rsocket::Payload request,
      uint32_t) final;

  std::shared_ptr<Flowable<rsocket::Payload>> handleRequestStream(
      rsocket::Payload request,
      uint32_t) final;
};

std::pair<std::unique_ptr<folly::IOBuf>, std::unique_ptr<folly::IOBuf>>
makeTestResponse(
    std::unique_ptr<folly::IOBuf> requestMetadata,
    std::unique_ptr<folly::IOBuf> requestData) {
  std::pair<std::unique_ptr<folly::IOBuf>, std::unique_ptr<folly::IOBuf>>
      response;

  folly::StringPiece data(requestData->coalesce());
  constexpr folly::StringPiece kMetadataEchoPrefix{"metadata_echo:"};
  constexpr folly::StringPiece kDataEchoPrefix{"data_echo:"};

  folly::Optional<rsocket::Payload> responsePayload;
  if (data.removePrefix("sleep_ms:")) {
    // Sleep, then echo back request.
    std::chrono::milliseconds sleepFor(folly::to<uint32_t>(data));
    std::this_thread::sleep_for(sleepFor); // sleep override
  } else if (data.removePrefix("error:")) {
    // Reply with a specific kind of error.
  } else if (data.startsWith(kMetadataEchoPrefix)) {
    // Reply with echoed metadata in the response payload.
    auto responseMetadata = requestData->clone();
    responseMetadata->trimStart(kMetadataEchoPrefix.size());
    response =
        std::make_pair(std::move(responseMetadata), std::move(requestData));
  } else if (data.startsWith(kDataEchoPrefix)) {
    // Reply with echoed data in the response payload.
    auto responseData = requestData->clone();
    responseData->trimStart(kDataEchoPrefix.size());
    response =
        std::make_pair(std::move(requestMetadata), std::move(responseData));
  }

  // If response payload is not set at this point, simply echo back what client
  // sent.
  if (!response.first && !response.second) {
    response =
        std::make_pair(std::move(requestMetadata), std::move(requestData));
  }

  return response;
}

template <class P>
P makePayload(folly::StringPiece metadata, folly::StringPiece data);

template <>
rsocket::Payload makePayload<rsocket::Payload>(
    folly::StringPiece metadata,
    folly::StringPiece data) {
  return rsocket::Payload(data, metadata);
}

template <>
apache::thrift::rocket::Payload makePayload<apache::thrift::rocket::Payload>(
    folly::StringPiece metadata,
    folly::StringPiece data) {
  return apache::thrift::rocket::Payload::makeFromMetadataAndData(
      metadata, data);
}

template <class P>
std::shared_ptr<yarpl::flowable::Flowable<P>> makeTestFlowable(
    folly::StringPiece data) {
  size_t n = 500;
  if (data.removePrefix("generate:")) {
    n = folly::to<size_t>(data);
  }

  auto gen = [n, i = static_cast<size_t>(0)](
                 auto& subscriber, int64_t requested) mutable {
    while (requested-- > 0 && i < n) {
      subscriber.onNext(makePayload<P>(
          folly::to<std::string>("metadata:", i), folly::to<std::string>(i)));
      ++i;
    }
    if (i == n) {
      subscriber.onComplete();
    }
  };
  return yarpl::flowable::Flowable<P>::create(std::move(gen));
}

std::shared_ptr<Single<rsocket::Payload>>
RsocketTestServerResponder::handleRequestResponse(
    rsocket::Payload request,
    uint32_t /* streamId */) {
  DCHECK(request.data);
  auto data = folly::StringPiece(request.data->coalesce());

  if (data.removePrefix("error:application")) {
    return Single<rsocket::Payload>::create([](auto&& subscriber) mutable {
      subscriber->onSubscribe(SingleSubscriptions::empty());
      subscriber->onError(folly::make_exception_wrapper<std::runtime_error>(
          "Application error occurred"));
    });
  }

  auto response =
      makeTestResponse(std::move(request.metadata), std::move(request.data));
  rsocket::Payload responsePayload(
      std::move(response.second), std::move(response.first));

  return Single<rsocket::Payload>::create(
      [responsePayload =
           std::move(responsePayload)](auto&& subscriber) mutable {
        subscriber->onSubscribe(SingleSubscriptions::empty());
        subscriber->onSuccess(std::move(responsePayload));
      });
}

std::shared_ptr<Flowable<rsocket::Payload>>
RsocketTestServerResponder::handleRequestStream(
    rsocket::Payload request,
    uint32_t /* streamId */) {
  DCHECK(request.data);
  auto data = folly::StringPiece(request.data->coalesce());

  if (data.removePrefix("error:application")) {
    return Flowable<rsocket::Payload>::create(
        [](Subscriber<rsocket::Payload>& subscriber,
           int64_t /* requested */) mutable {
          subscriber.onError(folly::make_exception_wrapper<std::runtime_error>(
              "Application error occurred"));
        });
  }
  return makeTestFlowable<rsocket::Payload>(data);
}
} // namespace

rocket::SetupFrame RocketTestClient::makeTestSetupFrame() {
  RequestSetupMetadata meta;
  meta.userSetupParams_ref() = "setup_data";
  CompactProtocolWriter compactProtocolWriter;
  folly::IOBufQueue paramQueue;
  compactProtocolWriter.setOutput(&paramQueue);
  meta.write(&compactProtocolWriter);

  // Serialize RocketClient's major/minor version (which is separate from the
  // rsocket protocol major/minor version) into setup metadata.
  auto buf = folly::IOBuf::createCombined(
      sizeof(int32_t) + meta.serializedSize(&compactProtocolWriter));
  folly::IOBufQueue queue;
  queue.append(std::move(buf));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  // Serialize RocketClient's major/minor version (which is separate from the
  // rsocket protocol major/minor version) into setup metadata.
  appender.writeBE<uint16_t>(0); // Thrift RocketClient major version
  appender.writeBE<uint16_t>(1); // Thrift RocketClient minor version
  // Append serialized setup parameters to setup frame metadata
  appender.insert(paramQueue.move());
  return rocket::SetupFrame(
      rocket::Payload::makeFromMetadataAndData(queue.move(), {}));
}

RsocketTestServer::RsocketTestServer() {
  TcpConnectionAcceptor::Options opts;
  opts.address = folly::SocketAddress("::1", 0 /* bind to any port */);
  opts.threads = 2;

  rsocketServer_ = RSocket::createServer(
      std::make_unique<TcpConnectionAcceptor>(std::move(opts)));
  // Start accepting connections
  rsocketServer_->start([](const rsocket::SetupParameters&) {
    return std::make_shared<RsocketTestServerResponder>();
  });
}

RsocketTestServer::~RsocketTestServer() {
  shutdown();
}

uint16_t RsocketTestServer::getListeningPort() const {
  auto oport = rsocketServer_->listeningPort();
  DCHECK(oport);
  return *oport;
}

void RsocketTestServer::shutdown() {
  rsocketServer_.reset();
}

RocketTestClient::RocketTestClient(const folly::SocketAddress& serverAddr)
    : evb_(*evbThread_.getEventBase()),
      fm_(folly::fibers::getFiberManager(evb_)),
      serverAddr_(serverAddr) {
  connect();
}

RocketTestClient::~RocketTestClient() {
  disconnect();
}

folly::Try<Payload> RocketTestClient::sendRequestResponseSync(
    Payload request,
    std::chrono::milliseconds timeout,
    RocketClientWriteCallback* writeCallback) {
  folly::Try<Payload> response;
  folly::fibers::Baton baton;

  evb_.runInEventBaseThread([&] {
    fm_.addTaskFinally(
        [&] {
          return client_->sendRequestResponseSync(
              std::move(request), timeout, writeCallback);
        },
        [&](folly::Try<Payload>&& r) {
          response = std::move(r);
          baton.post();
        });
  });

  baton.wait();
  return response;
}

folly::Try<void> RocketTestClient::sendRequestFnfSync(
    Payload request,
    RocketClientWriteCallback* writeCallback) {
  folly::Try<void> response;
  folly::fibers::Baton baton;

  evb_.runInEventBaseThread([&] {
    fm_.addTaskFinally(
        [&] {
          return client_->sendRequestFnfSync(std::move(request), writeCallback);
        },
        [&](folly::Try<void>&& r) {
          response = std::move(r);
          baton.post();
        });
  });

  baton.wait();
  return response;
}

folly::Try<SemiStream<Payload>> RocketTestClient::sendRequestStreamSync(
    Payload request) {
  folly::Try<SemiStream<Payload>> stream;

  evb_.runInEventBaseThreadAndWait([&] {
    stream = folly::makeTryWith([&] {
      return SemiStream<Payload>(
          toStream<Payload>(client_->createStream(std::move(request)), &evb_));
    });
  });

  return stream;
}

void RocketTestClient::reconnect() {
  disconnect();
  connect();
}

void RocketTestClient::connect() {
  evb_.runInEventBaseThreadAndWait([this] {
    folly::AsyncSocket::UniquePtr socket(
        new folly::AsyncSocket(&evb_, serverAddr_));
    client_ = RocketClient::create(
        evb_,
        std::move(socket),
        std::make_unique<rocket::SetupFrame>(makeTestSetupFrame()));
  });
}

void RocketTestClient::disconnect() {
  evb_.runInEventBaseThread([client = std::move(client_)] {
    if (client) {
      client->closeNow(folly::make_exception_wrapper<std::runtime_error>(
          "RocketTestClient disconnecting"));
    }
  });
}

namespace {
class RocketTestServerAcceptor final : public wangle::Acceptor {
 public:
  explicit RocketTestServerAcceptor(
      std::shared_ptr<RocketServerHandler> frameHandler,
      std::promise<void> shutdownPromise)
      : Acceptor(wangle::ServerSocketConfig{}),
        frameHandler_(std::move(frameHandler)),
        shutdownPromise_(std::move(shutdownPromise)) {}

  ~RocketTestServerAcceptor() override {
    EXPECT_EQ(0, connections_);
  }

  void onNewConnection(
      folly::AsyncTransportWrapper::UniquePtr socket,
      const folly::SocketAddress*,
      const std::string&,
      wangle::SecureTransportType,
      const wangle::TransportInfo&) override {
    auto* connection =
        new RocketServerConnection(std::move(socket), frameHandler_);
    getConnectionManager()->addConnection(connection);
  }

  void onConnectionsDrained() override {
    shutdownPromise_.set_value();
  }

  void onConnectionAdded(const wangle::ManagedConnection*) override {
    ++connections_;
  }

  void onConnectionRemoved(const wangle::ManagedConnection* conn) override {
    if (expectedRemainingStreams_ != folly::none) {
      if (auto rconn = dynamic_cast<const RocketServerConnection*>(conn)) {
        EXPECT_EQ(expectedRemainingStreams_, rconn->getNumStreams());
      }
    }

    --connections_;
  }

  void setExpectedRemainingStreams(size_t size) {
    expectedRemainingStreams_ = size;
  }

 private:
  const std::shared_ptr<RocketServerHandler> frameHandler_;
  std::promise<void> shutdownPromise_;
  size_t connections_{0};
  folly::Optional<size_t> expectedRemainingStreams_ = folly::none;
};

class RocketTestServerHandler : public RocketServerHandler {
 public:
  void handleSetupFrame(SetupFrame&&, RocketServerFrameContext&&) final {}

  void handleRequestResponseFrame(
      RequestResponseFrame&& frame,
      RocketServerFrameContext&& context) final {
    auto payload = std::move(frame.payload());
    folly::StringPiece dataPiece(payload.data()->coalesce());

    if (dataPiece.removePrefix("error:application")) {
      return context.sendError(RocketException(
          ErrorCode::APPLICATION_ERROR, "Application error occurred"));
    }

    auto md = std::move(payload).metadata();
    auto data = std::move(payload).data();
    auto response = makeTestResponse(std::move(md), std::move(data));
    auto responsePayload = Payload::makeFromMetadataAndData(
        std::move(response.first), std::move(response.second));
    return context.sendPayload(
        std::move(responsePayload), Flags::none().next(true).complete(true));
  }

  void handleRequestFnfFrame(RequestFnfFrame&&, RocketServerFrameContext&&)
      final {}

  void handleRequestStreamFrame(
      RequestStreamFrame&& frame,
      std::shared_ptr<RocketServerStreamSubscriber> subscriber) final {
    auto payload = std::move(frame.payload());
    folly::StringPiece dataPiece(payload.data()->coalesce());

    if (dataPiece.removePrefix("error:application")) {
      return Flowable<Payload>::error(
                 folly::make_exception_wrapper<RocketException>(
                     ErrorCode::APPLICATION_ERROR,
                     "Application error occurred"))
          ->subscribe(std::move(subscriber));
    }
    makeTestFlowable<rocket::Payload>(dataPiece)->subscribe(
        std::move(subscriber));
  }
};
} // namespace

RocketTestServer::RocketTestServer()
    : evb_(*ioThread_.getEventBase()),
      listeningSocket_(new folly::AsyncServerSocket(&evb_)) {
  std::promise<void> shutdownPromise;
  shutdownFuture_ = shutdownPromise.get_future();
  acceptor_ = std::make_unique<RocketTestServerAcceptor>(
      std::make_shared<RocketTestServerHandler>(), std::move(shutdownPromise));
  start();
}

RocketTestServer::~RocketTestServer() {
  stop();
}

void RocketTestServer::start() {
  folly::via(
      &evb_,
      [this] {
        acceptor_->init(listeningSocket_.get(), &evb_);
        listeningSocket_->bind(0 /* bind to any port */);
        listeningSocket_->listen(128 /* tcpBacklog */);
        listeningSocket_->startAccepting();
      })
      .wait();
}

void RocketTestServer::stop() {
  // Ensure socket and acceptor are destroyed in EventBase thread
  folly::via(&evb_, [listeningSocket = std::move(listeningSocket_)] {});
  // Wait for server to drain connections as gracefully as possible.
  shutdownFuture_.wait();
  folly::via(&evb_, [acceptor = std::move(acceptor_)] {});
}

uint16_t RocketTestServer::getListeningPort() const {
  return listeningSocket_->getAddress().getPort();
}

void RocketTestServer::setExpectedRemainingStreams(size_t n) {
  if (auto acceptor =
          dynamic_cast<RocketTestServerAcceptor*>(acceptor_.get())) {
    acceptor->setExpectedRemainingStreams(n);
  }
}

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
