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

#include <memory>

#include <folly/io/IOBuf.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/httpserver/ScopedHTTPServer.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/testutil/CoreTestFixture.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>
#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>
#include <thrift/lib/cpp2/transport/http2/common/testutil/ChannelTestFixture.h>
#include <thrift/lib/cpp2/transport/http2/common/testutil/FakeProcessors.h>

namespace apache {
namespace thrift {

using folly::IOBuf;
using std::string;
using std::unordered_map;

class SingleRpcChannelTest
    : public ChannelTestFixture,
      public testing::WithParamInterface<string::size_type> {};

TEST_P(SingleRpcChannelTest, VaryingChunkSizes) {
  apache::thrift::server::ServerConfigsMock server;
  EchoProcessor processor(
      server, "extrakey", "extravalue", "<eom>", eventBase_.get());
  unordered_map<string, string> inputHeaders;
  inputHeaders["key1"] = "value1";
  inputHeaders["key2"] = "value2";
  string inputPayload = "single stream payload";
  unordered_map<string, string>* outputHeaders;
  IOBuf* outputPayload;
  sendAndReceiveStream(
      &processor,
      inputHeaders,
      inputPayload,
      GetParam(),
      outputHeaders,
      outputPayload);
  EXPECT_EQ(3, outputHeaders->size());
  EXPECT_EQ("value1", outputHeaders->at("key1"));
  EXPECT_EQ("value2", outputHeaders->at("key2"));
  EXPECT_EQ("extravalue", outputHeaders->at("extrakey"));
  EXPECT_EQ("single stream payload<eom>", toString(outputPayload));
}

INSTANTIATE_TEST_CASE_P(
    AllChunkSizes,
    SingleRpcChannelTest,
    testing::Values(0, 1, 2, 4, 10));

TEST_F(ChannelTestFixture, SingleRpcChannelErrorEmptyBody) {
  apache::thrift::server::ServerConfigsMock server;
  EchoProcessor processor(
      server, "extrakey", "extravalue", "<eom>", eventBase_.get());
  unordered_map<string, string> inputHeaders;
  inputHeaders["key1"] = "value1";
  string inputPayload = "";
  unordered_map<string, string>* outputHeaders;
  IOBuf* outputPayload;
  sendAndReceiveStream(
      &processor,
      inputHeaders,
      inputPayload,
      0,
      outputHeaders,
      outputPayload,
      true);
  EXPECT_EQ(0, outputHeaders->size());
  TApplicationException tae;
  EXPECT_TRUE(CoreTestFixture::deserializeException(outputPayload, &tae));
  EXPECT_EQ(TApplicationException::UNKNOWN, tae.getType());
  EXPECT_EQ("Proxygen stream has no body", tae.getMessage());
}

TEST_F(ChannelTestFixture, SingleRpcChannelErrorNoEnvelope) {
  apache::thrift::server::ServerConfigsMock server;
  EchoProcessor processor(
      server, "extrakey", "extravalue", "<eom>", eventBase_.get());
  unordered_map<string, string> inputHeaders;
  inputHeaders["key1"] = "value1";
  string inputPayload = "notempty";
  unordered_map<string, string>* outputHeaders;
  IOBuf* outputPayload;
  sendAndReceiveStream(
      &processor,
      inputHeaders,
      inputPayload,
      0,
      outputHeaders,
      outputPayload,
      true);
  EXPECT_EQ(0, outputHeaders->size());
  TApplicationException tae;
  EXPECT_TRUE(CoreTestFixture::deserializeException(outputPayload, &tae));
  EXPECT_EQ(TApplicationException::UNKNOWN, tae.getType());
  EXPECT_EQ("Invalid envelope: see logs for error", tae.getMessage());
}

struct RequestState {
  bool sent{false};
  bool reply{false};
  bool error{false};
  ClientReceiveState receiveState;
};

class TestRequestCallback : public apache::thrift::RequestCallback {
 public:
  explicit TestRequestCallback(folly::Promise<RequestState> promise)
      : promise_(std::move(promise)) {}

  void requestSent() final {
    rstate_.sent = true;
  }
  void replyReceived(ClientReceiveState&& state) final {
    rstate_.reply = true;
    rstate_.receiveState = std::move(state);
    promise_.setValue(std::move(rstate_));
  }
  void requestError(ClientReceiveState&& state) final {
    rstate_.error = true;
    rstate_.receiveState = std::move(state);
    promise_.setValue(std::move(rstate_));
  }

 private:
  RequestState rstate_;
  folly::Promise<RequestState> promise_;
};

template <class HandlerType>
std::unique_ptr<proxygen::ScopedHTTPServer> startProxygenServer(
    HandlerType handler) {
  folly::SocketAddress saddr;
  saddr.setFromLocalPort(static_cast<uint16_t>(0));
  proxygen::HTTPServer::IPConfig cfg{saddr,
                                     proxygen::HTTPServer::Protocol::HTTP2};
  auto f =
      std::make_unique<proxygen::ScopedHandlerFactory<HandlerType>>(handler);
  proxygen::HTTPServerOptions options;
  options.threads = 1;
  options.handlerFactories.push_back(std::move(f));

  return proxygen::ScopedHTTPServer::start(cfg, std::move(options));
}

void httpHandler(
    proxygen::HTTPMessage message,
    std::unique_ptr<folly::IOBuf> /* data */,
    proxygen::ResponseBuilder& builder) {
  if (message.getURL() == "internal_error") {
    builder.status(500, "Internal Server Error").body("internal error");
  } else if (message.getURL() == "eof") {
    builder.status(200, "OK");
  } else {
    builder.status(200, "OK").body("(y)");
  }
}

folly::Future<RequestState> sendRequest(
    folly::EventBase& evb,
    apache::thrift::ThriftChannelIf& channel,
    std::string url) {
  folly::Promise<RequestState> promise;
  auto f = promise.getFuture();

  auto cb = std::make_unique<ThriftClientCallback>(
      &evb,
      std::make_unique<TestRequestCallback>(std::move(promise)),
      std::make_unique<ContextStack>("test"),
      detail::compact::PROTOCOL_ID,
      std::chrono::milliseconds{10000});

  // Send a bad request.
  evb.runInEventBaseThread(
      [&channel, url = std::move(url), cb = std::move(cb)]() mutable {
        auto metadata = std::make_unique<RequestRpcMetadata>();
        metadata->set_url(url);
        metadata->set_kind(
            ::apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE);
        channel.sendThriftRequest(
            std::move(metadata), folly::IOBuf::create(1), std::move(cb));
      });

  return f;
}

void validateException(
    RequestState& rstate,
    transport::TTransportException::TTransportExceptionType type,
    const char* expectedError) {
  EXPECT_TRUE(rstate.sent);
  EXPECT_TRUE(rstate.error);
  EXPECT_FALSE(rstate.reply);
  ASSERT_TRUE(rstate.receiveState.isException());
  auto* ex = rstate.receiveState.exception()
                 .get_exception<transport::TTransportException>();
  ASSERT_NE(nullptr, ex);
  EXPECT_EQ(type, ex->getType());
  EXPECT_STREQ(expectedError, ex->what());
}

TEST(SingleRpcChannel, ClientExceptions) {
  // Spin up the server.
  auto server = startProxygenServer(&httpHandler);
  ASSERT_NE(nullptr, server);
  const auto port = server->getPort();
  ASSERT_NE(0, port);

  // Spin up the client channel.
  folly::EventBase evb;
  folly::SocketAddress addr;
  addr.setFromLocalPort(port);

  async::TAsyncSocket::UniquePtr sock(new async::TAsyncSocket(&evb, addr));
  auto conn = H2ClientConnection::newHTTP2Connection(std::move(sock));
  EXPECT_TRUE(conn->good());
  auto channel = conn->getChannel();

  auto rstate = sendRequest(evb, *channel, "internal_error").getVia(&evb);

  // Validate that we get proper exception.
  validateException(
      rstate,
      transport::TTransportException::UNKNOWN,
      "Bad status: 500 Internal Server Error");

  // The connection should be still good!
  EXPECT_TRUE(conn->good());

  // Follow up with a request that results in empty payload.
  channel = conn->getChannel();
  rstate = sendRequest(evb, *channel, "eof").getVia(&evb);

  validateException(
      rstate, transport::TTransportException::END_OF_FILE, "No content");

  // The connection should be still good!
  EXPECT_TRUE(conn->good());

  // Follow up with an OK request.
  channel = conn->getChannel();
  rstate = sendRequest(evb, *channel, "ok").getVia(&evb);

  EXPECT_TRUE(rstate.sent);
  EXPECT_FALSE(rstate.error);
  EXPECT_TRUE(rstate.reply);

  conn->closeNow();
  evb.loopOnce();
}

} // namespace thrift
} // namespace apache
