/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <fizz/client/AsyncFizzClient.h>
#include <fizz/client/FizzClientContext.h>

#include <folly/ExceptionWrapper.h>
#include <folly/ScopeGuard.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/portability/GMock.h>
#include <folly/synchronization/Baton.h>

#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/fast_thrift/security/FizzServerCertConfig.h>
#include <thrift/lib/cpp2/fast_thrift/security/test/TestCert.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/FastThriftServer.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/test/if/gen-cpp2/AlignedParserServer.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/test/if/gen-cpp2/AlignedParserServer.tcc>
#include <thrift/lib/cpp2/fast_thrift/thrift/test/if/gen-cpp2/AlignedParserServerAsyncClient.h>

THRIFT_FLAG_DECLARE_bool(rocket_client_binary_rpc_metadata_encoding);

namespace apache::thrift::fast_thrift::thrift::test::aligned_parser {

using namespace testing;

namespace {

class Handler : public apache::thrift::FastServiceHandler<AlignedParserServer> {
 public:
  void async_tm_consume(
      FastHandlerCallbackPtr<void> callback,
      std::unique_ptr<Request> request) override {
    const folly::IOBuf& data = *request->data().value();
    addressRemainder_.store(
        reinterpret_cast<uintptr_t>(data.data()) % 16,
        std::memory_order_relaxed);
    chainElements_.store(data.countChainElements(), std::memory_order_relaxed);
    dataSize_.store(data.computeChainDataLength(), std::memory_order_relaxed);
    callback->done();
  }

  size_t addressRemainder() const noexcept {
    return addressRemainder_.load(std::memory_order_relaxed);
  }

  size_t chainElements() const noexcept {
    return chainElements_.load(std::memory_order_relaxed);
  }

  size_t dataSize() const noexcept {
    return dataSize_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<size_t> addressRemainder_{16};
  std::atomic<size_t> chainElements_{0};
  std::atomic<size_t> dataSize_{0};
};

FastThriftServerConfig makeServerConfig() {
  FastThriftServerConfig config;
  config.address = folly::SocketAddress("::1", 0);
  config.numIOThreads = 1;
  config.useAlignedParser = true;
  return config;
}

security::FizzServerCertConfig makeTlsConfig() {
  const security::test::TestCert cert = security::test::makeTestCert();
  security::FizzServerCertConfig config;
  config.certPem = cert.certPem;
  config.keyPem = cert.keyPem;
  config.clientAuth = fizz::server::ClientAuthMode::None;
  return config;
}

folly::AsyncTransport::UniquePtr connectFizz(
    folly::EventBase* evb, const folly::SocketAddress& address) {
  constexpr int kConnectTimeoutMs = 5000;

  struct Callback : public folly::AsyncSocket::ConnectCallback,
                    public fizz::client::AsyncFizzClient::HandshakeCallback {
    folly::Baton<>& done;
    folly::exception_wrapper& error;
    fizz::client::AsyncFizzClient::UniquePtr& client;

    Callback(
        folly::Baton<>& done,
        folly::exception_wrapper& error,
        fizz::client::AsyncFizzClient::UniquePtr& client)
        : done(done), error(error), client(client) {}

    void connectSuccess() noexcept override {
      client->connect(
          this,
          /*verifier=*/nullptr,
          /*sni=*/folly::none,
          /*pskIdentity=*/folly::none,
          /*echConfigs=*/folly::none);
    }

    void connectErr(const folly::AsyncSocketException& ex) noexcept override {
      error = folly::exception_wrapper(ex);
      done.post();
    }

    void fizzHandshakeSuccess(
        fizz::client::AsyncFizzClient*) noexcept override {
      done.post();
    }

    void fizzHandshakeError(
        fizz::client::AsyncFizzClient*,
        folly::exception_wrapper ex) noexcept override {
      error = std::move(ex);
      done.post();
    }
  };

  folly::Baton<> done;
  folly::exception_wrapper error;
  fizz::client::AsyncFizzClient::UniquePtr client;
  Callback callback{done, error, client};

  evb->runInEventBaseThreadAndWait([&] {
    folly::AsyncSocket::UniquePtr socket = folly::AsyncSocket::newSocket(evb);
    std::shared_ptr<fizz::client::FizzClientContext> context =
        std::make_shared<fizz::client::FizzClientContext>();
    folly::AsyncSocket* const socketPtr = socket.get();
    client.reset(new fizz::client::AsyncFizzClient(std::move(socket), context));
    socketPtr->connect(&callback, address, kConnectTimeoutMs);
  });

  done.wait();
  if (error) {
    evb->runInEventBaseThreadAndWait([&] { client.reset(); });
    return nullptr;
  }
  return folly::AsyncTransport::UniquePtr(client.release());
}

} // namespace

class FastThriftServerAlignedParserE2ETest : public Test {
 protected:
  void SetUp() override {
    THRIFT_FLAG_SET_MOCK(rocket_client_binary_rpc_metadata_encoding, true);

    handler_ = std::make_shared<Handler>();
    server_ = std::make_unique<FastThriftServer>(makeServerConfig());
    server_->setSSLConfig(makeTlsConfig());
    server_->setInterface(handler_);
    server_->start();
    clientThread_ = std::make_unique<folly::ScopedEventBaseThread>();
  }

  void TearDown() override {
    THRIFT_FLAG_UNMOCK(rocket_client_binary_rpc_metadata_encoding);
    clientThread_.reset();
    if (server_ != nullptr) {
      server_->stop();
    }
    server_.reset();
  }

  std::unique_ptr<apache::thrift::Client<AlignedParserServer>> createClient() {
    folly::EventBase* const evb = clientThread_->getEventBase();
    folly::AsyncTransport::UniquePtr transport =
        connectFizz(evb, server_->getAddress());
    if (transport == nullptr) {
      return nullptr;
    }
    std::unique_ptr<apache::thrift::Client<AlignedParserServer>> client;
    evb->runInEventBaseThreadAndWait([&] {
      apache::thrift::RocketClientChannel::Ptr channel =
          apache::thrift::RocketClientChannel::newChannel(std::move(transport));
      channel->setProtocolId(apache::thrift::protocol::T_BINARY_PROTOCOL);
      client = std::make_unique<apache::thrift::Client<AlignedParserServer>>(
          std::move(channel));
    });
    return client;
  }

  template <typename Client>
  void destroyClientOnEvb(std::unique_ptr<Client>& client) {
    clientThread_->getEventBase()->runInEventBaseThreadAndWait(
        [&] { client.reset(); });
  }

  std::shared_ptr<Handler> handler_;
  std::unique_ptr<FastThriftServer> server_;
  std::unique_ptr<folly::ScopedEventBaseThread> clientThread_;
};

TEST_F(FastThriftServerAlignedParserE2ETest, AlignsRequestData) {
  constexpr size_t kDataSize = 4096;
  Request request;
  request.data() = folly::IOBuf::copyBuffer(std::string(kDataSize, 'd'));
  request.marker() = 42;

  std::unique_ptr<apache::thrift::Client<AlignedParserServer>> client =
      createClient();
  SCOPE_EXIT {
    destroyClientOnEvb(client);
  };
  EXPECT_THAT(client, NotNull());
  if (client == nullptr) {
    return;
  }
  client->semifuture_consume(request).get();

  EXPECT_THAT(handler_->addressRemainder(), Eq(0));
  EXPECT_THAT(handler_->chainElements(), Eq(1));
  EXPECT_THAT(handler_->dataSize(), Eq(kDataSize));
}

} // namespace apache::thrift::fast_thrift::thrift::test::aligned_parser
