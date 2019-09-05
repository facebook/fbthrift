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
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <folly/executors/InlineExecutor.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/futures/Future.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <rsocket/RSocket.h>
#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/ServerSocketConfig.h>
#include <yarpl/Flowable.h>
#include <yarpl/Single.h>

#include <thrift/lib/cpp2/async/ServerSinkBridge.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/async/StreamPublisher.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/transport/core/TryUtil.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/framing/test/Util.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>
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

  std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>
  handleRequestChannel(
      rsocket::Payload request,
      std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>
          requestStream,
      uint32_t streamId) final;

 private:
  folly::ScopedEventBaseThread th_;
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
    while (requested-- > 0 && i <= n) {
      // Note that first payload (i == 0) is not counted against requested
      // number of payloads to generate.
      subscriber.onNext(makePayload<P>(
          !i ? CompactSerializer::serialize<std::string>(ResponseRpcMetadata{})
             : CompactSerializer::serialize<std::string>(
                   StreamPayloadMetadata{}),
          folly::to<std::string>(i)));
      ++i;
    }
    if (i == n + 1) {
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

std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>
RsocketTestServerResponder::handleRequestChannel(
    rsocket::Payload request,
    std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>> input,
    uint32_t /* streamId */) {
  auto data = folly::StringPiece(request.data->coalesce());

  auto streamAndPublisher =
      apache::thrift::StreamPublisher<rsocket::Payload>::create(
          th_.getEventBase(), [] {}, 1000);
  streamAndPublisher.second.next(rsocket::Payload("FirstResponse"));
  if (data.removePrefix("upload:")) {
    std::shared_ptr<int> d = std::make_shared<int>(0);
    input->subscribe(
        [d](rsocket::Payload p) {
          std::string data = p.cloneDataToString();
          if (data.empty()) {
            return;
          }
          EXPECT_EQ(*d, folly::to<int>(data));
          (*d)++;
        },
        [](folly::exception_wrapper) {},
        [publisher = std::move(streamAndPublisher.second), d]() mutable {
          publisher.next(rsocket::Payload(folly::to<std::string>(*d)));
          std::move(publisher).complete();
        },
        10);
  }
  return toFlowable(std::move(streamAndPublisher.first));
}

} // namespace

rocket::SetupFrame RocketTestClient::makeTestSetupFrame(
    MetadataOpaqueMap<std::string, std::string> md) {
  RequestSetupMetadata meta;
  meta.opaque_ref() = {};
  *meta.opaque_ref() = std::move(md);
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
        [&](folly::Try<folly::Try<Payload>>&& r) {
          response = collapseTry(std::move(r));
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
        [&](folly::Try<folly::Try<void>>&& r) {
          response = collapseTry(std::move(r));
          baton.post();
        });
  });

  baton.wait();
  return response;
}

folly::Try<SemiStream<Payload>> RocketTestClient::sendRequestStreamSync(
    Payload request) {
  constexpr std::chrono::milliseconds kFirstResponseTimeout{500};
  constexpr std::chrono::milliseconds kChunkTimeout{500};

  class TestStreamClientCallback final
      : public yarpl::flowable::Flowable<Payload>,
        public StreamClientCallback {
   private:
    class Subscription final : public yarpl::flowable::Subscription {
     public:
      explicit Subscription(std::shared_ptr<TestStreamClientCallback> flowable)
          : flowable_(std::move(flowable)) {}

      void request(int64_t n) override {
        if (!flowable_) {
          return;
        }
        n = std::min<int64_t>(
            std::max<int64_t>(0, n), std::numeric_limits<int32_t>::max());
        flowable_->serverCallback_->onStreamRequestN(n);
      }

      void cancel() override {
        flowable_->serverCallback_->onStreamCancel();
        flowable_.reset();
      }

     private:
      std::shared_ptr<TestStreamClientCallback> flowable_;
    };

   public:
    TestStreamClientCallback(
        std::chrono::milliseconds chunkTimeout,
        folly::Promise<SemiStream<Payload>> p)
        : chunkTimeout_(chunkTimeout), p_(std::move(p)) {}

    void init() {
      self_ = this->ref_from_this(this);
    }

    // Flowable interface
    void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<Payload>>
                       subscriber) override {
      subscriber_ = std::move(subscriber);
      auto subscription =
          std::make_shared<Subscription>(this->ref_from_this(this));
      subscriber_->onSubscribe(std::move(subscription));
      if (pendingComplete_) {
        onStreamComplete();
      }
      if (pendingError_) {
        onStreamError(std::move(pendingError_));
      }
    }

    // ClientCallback interface
    void onFirstResponse(
        FirstResponsePayload&& firstPayload,
        folly::EventBase* evb,
        StreamServerCallback* serverCallback) override {
      serverCallback_ = serverCallback;
      auto self = std::move(self_);
      self = self->timeout(*evb, chunkTimeout_, chunkTimeout_, [] {
        return transport::TTransportException(
            transport::TTransportException::TTransportExceptionType::TIMED_OUT);
      });
      (void)firstPayload;
      p_.setValue(toStream(std::move(self), evb));
    }

    void onFirstResponseError(folly::exception_wrapper ew) override {
      p_.setException(std::move(ew));
      self_.reset();
    }

    void onStreamNext(StreamPayload&& payload) override {
      subscriber_->onNext(Payload::makeFromData(std::move(payload.payload)));
    }

    void onStreamError(folly::exception_wrapper ew) override {
      if (!subscriber_) {
        pendingError_ = std::move(ew);
        return;
      }
      subscriber_->onError(std::move(ew));
    }

    void onStreamComplete() override {
      if (subscriber_) {
        subscriber_->onComplete();
      } else {
        pendingComplete_ = true;
      }
    }

   private:
    std::chrono::milliseconds chunkTimeout_;
    folly::Promise<SemiStream<Payload>> p_;

    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> subscriber_;
    StreamServerCallback* serverCallback_{nullptr};
    std::shared_ptr<yarpl::flowable::Flowable<Payload>> self_;

    bool pendingComplete_{false};
    folly::exception_wrapper pendingError_;
  };

  folly::Promise<SemiStream<Payload>> p;
  auto sf = p.getSemiFuture();

  auto clientCallback =
      std::make_shared<TestStreamClientCallback>(kChunkTimeout, std::move(p));
  clientCallback->init();

  evb_.runInEventBaseThread([&] {
    fm_.addTask([&] {
      client_->sendRequestStream(
          std::move(request), kFirstResponseTimeout, clientCallback.get());
    });
  });

  return folly::makeTryWith([&] {
    return std::move(sf).via(&folly::InlineExecutor::instance()).get();
  });
}

void RocketTestClient::sendRequestChannel(
    ChannelClientCallback* callback,
    Payload request) {
  evb_.runInEventBaseThread(
      [this, request = std::move(request), callback]() mutable {
        fm_.addTask([this, request = std::move(request), callback]() mutable {
          constexpr std::chrono::milliseconds kFirstResponseTimeout{500};
          client_->sendRequestChannel(
              std::move(request), kFirstResponseTimeout, callback);
        });
      });
}

void RocketTestClient::sendRequestSink(
    SinkClientCallback* callback,
    Payload request) {
  evb_.runInEventBaseThread(
      [this, request = std::move(request), callback]() mutable {
        fm_.addTask([this, request = std::move(request), callback]() mutable {
          constexpr std::chrono::milliseconds kFirstResponseTimeout{500};
          client_->sendRequestSink(
              std::move(request), kFirstResponseTimeout, callback);
        });
      });
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
    auto* connection = new RocketServerConnection(
        std::move(socket), frameHandler_, std::chrono::milliseconds::zero());
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
} // namespace

class RocketTestServer::RocketTestServerHandler : public RocketServerHandler {
 public:
  explicit RocketTestServerHandler(folly::EventBase& ioEvb) : ioEvb_(ioEvb) {}
  void handleSetupFrame(SetupFrame&& frame, RocketServerFrameContext&&) final {
    folly::io::Cursor cursor(frame.payload().buffer());
    // Validate Thrift major/minor version
    int16_t majorVersion;
    int16_t minorVersion;
    const bool success = cursor.tryReadBE<int16_t>(majorVersion) &&
        cursor.tryReadBE<int16_t>(minorVersion);
    EXPECT_TRUE(success);
    EXPECT_EQ(0, majorVersion);
    EXPECT_EQ(1, minorVersion);
    // Validate RequestSetupMetadata
    CompactProtocolReader reader;
    reader.setInput(cursor);
    RequestSetupMetadata meta;
    meta.read(&reader);
    EXPECT_EQ(reader.getCursorPosition(), frame.payload().metadataSize());
    EXPECT_EQ(expectedSetupMetadata_, meta.opaque_ref().value_or({}));
  }

  void handleRequestResponseFrame(
      RequestResponseFrame&& frame,
      RocketServerFrameContext&& context) final {
    auto dam = splitMetadataAndData(frame.payload());
    auto payload = std::move(frame.payload());
    auto dataPiece = getRange(*dam.second);

    if (dataPiece.removePrefix("error:application")) {
      return context.sendError(RocketException(
          ErrorCode::APPLICATION_ERROR, "Application error occurred"));
    }

    auto response =
        makeTestResponse(std::move(dam.first), std::move(dam.second));
    auto responsePayload = Payload::makeFromMetadataAndData(
        std::move(response.first), std::move(response.second));
    return context.sendPayload(
        std::move(responsePayload), Flags::none().next(true).complete(true));
  }

  void handleRequestFnfFrame(RequestFnfFrame&&, RocketServerFrameContext&&)
      final {}

  void handleRequestStreamFrame(
      RequestStreamFrame&& frame,
      StreamClientCallback* clientCallback) final {
    class TestRocketStreamServerCallback final : public StreamServerCallback {
     public:
      TestRocketStreamServerCallback(
          StreamClientCallback* clientCallback,
          size_t n)
          : clientCallback_(clientCallback), n_(n) {}

      void onStreamRequestN(uint64_t tokens) override {
        while (tokens-- && i_++ < n_) {
          clientCallback_->onStreamNext(
              StreamPayload{folly::IOBuf::copyBuffer(std::to_string(i_)), {}});
        }
        if (i_ == n_) {
          clientCallback_->onStreamComplete();
          delete this;
        }
      }

      void onStreamCancel() override {
        delete this;
      }

     private:
      StreamClientCallback* const clientCallback_;
      size_t i_{0};
      const size_t n_;
    };

    folly::StringPiece data(std::move(frame.payload()).data()->coalesce());
    if (data.removePrefix("error:application")) {
      clientCallback->onStreamError(
          folly::make_exception_wrapper<RocketException>(
              ErrorCode::APPLICATION_ERROR, "Application error occurred"));
      delete clientCallback;
      return;
    }

    const size_t n =
        data.removePrefix("generate:") ? folly::to<size_t>(data) : 500;
    auto* serverCallback =
        new TestRocketStreamServerCallback(clientCallback, n);
    clientCallback->onFirstResponse(
        FirstResponsePayload{folly::IOBuf::copyBuffer(std::to_string(0)), {}},
        nullptr /* evb */,
        serverCallback);
  }

  void handleRequestChannelFrame(
      RequestChannelFrame&&,
      SinkClientCallback* clientCallback) final {
    apache::thrift::detail::SinkConsumerImpl impl{
        [](folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> asyncGen)
            -> folly::coro::Task<folly::Try<StreamPayload>> {
          int current = 0;
          while (auto item = co_await asyncGen.next()) {
            auto payload = (*item).value();
            auto data = folly::to<int32_t>(
                folly::StringPiece(payload.payload->coalesce()));
            EXPECT_EQ(current++, data);
          }
          co_return folly::Try<StreamPayload>(StreamPayload(
              folly::IOBuf::copyBuffer(folly::to<std::string>(current)), {}));
        },
        10,
        {}};
    auto serverCallback = apache::thrift::detail::ServerSinkBridge::create(
        std::move(impl), ioEvb_, clientCallback);

    clientCallback->onFirstResponse(
        FirstResponsePayload{folly::IOBuf::copyBuffer(std::to_string(0)), {}},
        nullptr /* evb */,
        serverCallback.get());
    folly::coro::co_invoke(
        [serverCallback =
             std::move(serverCallback)]() mutable -> folly::coro::Task<void> {
          co_return co_await serverCallback->start();
        })
        .scheduleOn(threadManagerThread_.getEventBase())
        .start();
  }

  void setExpectedSetupMetadata(
      MetadataOpaqueMap<std::string, std::string> md) {
    expectedSetupMetadata_ = std::move(md);
  }

 private:
  MetadataOpaqueMap<std::string, std::string> expectedSetupMetadata_{
      {"rando_key", "setup_data"}};
  folly::EventBase& ioEvb_;
  folly::ScopedEventBaseThread threadManagerThread_;
};

RocketTestServer::RocketTestServer()
    : evb_(*ioThread_.getEventBase()),
      listeningSocket_(new folly::AsyncServerSocket(&evb_)),
      handler_(std::make_shared<RocketTestServerHandler>(evb_)) {
  std::promise<void> shutdownPromise;
  shutdownFuture_ = shutdownPromise.get_future();

  acceptor_ = std::make_unique<RocketTestServerAcceptor>(
      handler_, std::move(shutdownPromise));
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

void RocketTestServer::setExpectedSetupMetadata(
    MetadataOpaqueMap<std::string, std::string> md) {
  handler_->setExpectedSetupMetadata(std::move(md));
}

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
