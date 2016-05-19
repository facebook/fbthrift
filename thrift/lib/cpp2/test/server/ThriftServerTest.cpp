/*
 * Copyright 2015 Facebook, Inc.
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
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/Memory.h>
#include <thrift/lib/cpp/transport/THeader.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>
#include <thrift/lib/cpp2/test/util/TestInterface.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>
#include <thrift/lib/cpp2/test/util/TestHeaderClientChannelFactory.h>

#include <folly/fibers/FiberManagerMap.h>
#include <wangle/concurrent/GlobalExecutor.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::protocol::TBinaryProtocolT;
using apache::thrift::test::TestServiceClient;

DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);

std::shared_ptr<TestServiceClient> getThrift1Client(
    const folly::SocketAddress& address) {
  // Create Thrift1 clients
  std::shared_ptr<TSocket> socket = std::make_shared<TSocket>(address);
  socket->open();
  std::shared_ptr<TFramedTransport> transport =
      std::make_shared<TFramedTransport>(socket);
  std::shared_ptr<TBinaryProtocolT<TBufferBase>> protocol =
      std::make_shared<TBinaryProtocolT<TBufferBase>>(transport);
  return std::make_shared<TestServiceClient>(protocol);
}

TEST(ThriftServer, OnewayClientConnectionCloseTest) {
  static std::atomic<bool> done(false);

  class OnewayTestInterface : public TestServiceSvIf {
    void noResponse(int64_t size) override {
      usleep(size);
      done = true;
    }
  };

  TestThriftServerFactory<OnewayTestInterface> factory2;
  ScopedServerThread st(factory2.create());

  {
    folly::EventBase base;
    std::shared_ptr<TAsyncSocket> socket(
        TAsyncSocket::newSocket(&base, *st.getAddress()));
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

    client.sync_noResponse(10000);
  } // client out of scope

  usleep(50000);
  EXPECT_TRUE(done);
}

TEST(ThriftServer, CompressionClientTest) {
  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto channel =
      boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel());
  channel->setTransform(apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  channel->setMinCompressBytes(1);

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");

  auto trans = channel->getWriteTransforms();
  EXPECT_EQ(trans.size(), 1);
  for (auto& tran : trans) {
    EXPECT_EQ(tran, apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  }
}

TEST(ThriftServer, CompressionServerTest) {
  TestThriftServerFactory<TestInterface> factory;
  factory.minCompressBytes(100);
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto channel =
      boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel());
  channel->setTransform(apache::thrift::transport::THeader::ZLIB_TRANSFORM);

  std::string request(55, 'a');
  std::string response;
  // The response is slightly more than 100 bytes before compression
  // and less than 100 bytes after compression
  client.sync_echoRequest(response, request);
  EXPECT_EQ(response.size(), 100);
}

TEST(ThriftServer, HeaderTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto serv = factory.create();
  ScopedServerThread sst(serv);
  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  RpcOptions options;
  // Set it as a header directly so the client channel won't set a
  // timeout and the test won't throw TTransportException
  options.setWriteHeader(
      apache::thrift::transport::THeader::CLIENT_TIMEOUT_HEADER,
      folly::to<std::string>(10));
  try {
    client.sync_processHeader(options);
    ADD_FAILURE() << "should timeout";
  } catch (const TApplicationException& e) {
    EXPECT_EQ(e.getType(),
              TApplicationException::TApplicationExceptionType::TIMEOUT);
  }
}

TEST(ThriftServer, LoadHeaderTest) {
  class Callback : public RequestCallback {
   public:
    explicit Callback(bool isLoadExpected)
        : isLoadExpected_(isLoadExpected) {}

   private:
    void requestSent() override {}

    void replyReceived(ClientReceiveState&& state) override {
      const auto& headers = state.header()->getHeaders();
      auto loadIter = headers.find(Cpp2Connection::loadHeader);
      ASSERT_EQ(isLoadExpected_, loadIter != headers.end());
      if (isLoadExpected_) {
        auto load = loadIter->second;
        EXPECT_NE("", load);
      }
    }
    void requestError(ClientReceiveState&&) override {
      ADD_FAILURE() << "The response should not be an error";
    }
    bool isLoadExpected_;
  };

  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  client->voidResponse(folly::make_unique<Callback>(false));

  RpcOptions emptyLoadOptions;
  emptyLoadOptions.setWriteHeader(Cpp2Connection::loadHeader, "");
  client->voidResponse(emptyLoadOptions, folly::make_unique<Callback>(true));

  RpcOptions customLoadOptions;
  customLoadOptions.setWriteHeader(Cpp2Connection::loadHeader, "foo");
  client->voidResponse(customLoadOptions, folly::make_unique<Callback>(true));

  base.loop();
}

TEST(ThriftServer, ClientTimeoutTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  ScopedServerThread sst(server);
  folly::EventBase base;

  auto getClient = [&base, &sst]() {
    std::shared_ptr<TAsyncSocket> socket(
        TAsyncSocket::newSocket(&base, *sst.getAddress()));

    return std::make_shared<TestServiceAsyncClient>(
        HeaderClientChannel::newChannel(socket));
  };

  int cbCtor = 0;
  int cbCall = 0;

  auto callback = [&cbCall, &cbCtor](
      std::shared_ptr<TestServiceAsyncClient> client, bool& timeout) {
    cbCtor++;
    return std::unique_ptr<RequestCallback>(new FunctionReplyCallback(
        [&cbCall, client, &timeout](ClientReceiveState&& state) {
          cbCall++;
          if (state.exception()) {
            timeout = true;
            try {
              std::rethrow_exception(state.exception());
            } catch (const TTransportException& e) {
              EXPECT_EQ(int(TTransportException::TIMED_OUT), int(e.getType()));
            }
            return;
          }
          try {
            std::string resp;
            client->recv_sendResponse(resp, state);
          } catch (const TApplicationException& e) {
            timeout = true;
            EXPECT_EQ(int(TApplicationException::TIMEOUT), int(e.getType()));
            EXPECT_TRUE(state.header()->getFlags() &
                HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
            return;
          }
          timeout = false;
        }));
  };

  // Set the timeout to be 5 milliseconds, but the call will take 10 ms.
  // The server should send a timeout after 5 milliseconds
  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(5));
  auto client1 = getClient();
  bool timeout1;
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_TRUE(timeout1);
  usleep(10000);

  // This time we set the timeout to be 100 millseconds.  The server
  // should not time out
  options.setTimeout(std::chrono::milliseconds(100));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_FALSE(timeout1);
  usleep(10000);

  // This time we set server timeout to be 5 millseconds.  However, the
  // task should start processing within that millisecond, so we should
  // not see an exception because the client timeout should be used after
  // processing is started
  server->setTaskExpireTime(std::chrono::milliseconds(5));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  usleep(10000);

  // The server timeout stays at 5 ms, but we put the client timeout at
  // 5 ms.  We should timeout even though the server starts processing within
  // 5ms.
  options.setTimeout(std::chrono::milliseconds(5));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_TRUE(timeout1);
  usleep(50000);

  // And finally, with the server timeout at 50 ms, we send 2 requests at
  // once.  Because the first request will take more than 50 ms to finish
  // processing (the server only has 1 worker thread), the second request
  // won't start processing until after 50ms, and will timeout, despite the
  // very high client timeout.
  // We don't know which one will timeout (race conditions) so we just check
  // the xor
  auto client2 = getClient();
  bool timeout2;
  server->setTaskExpireTime(std::chrono::milliseconds(50));
  options.setTimeout(std::chrono::milliseconds(110));
  client1->sendResponse(options, callback(client1, timeout1), 100000);
  client2->sendResponse(options, callback(client2, timeout2), 100000);
  base.loop();
  EXPECT_TRUE(timeout1 || timeout2);
  EXPECT_FALSE(timeout1 && timeout2);

  EXPECT_EQ(cbCall, cbCtor);
}

TEST(ThriftServer, ConnectionIdleTimeoutTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *st.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

TEST(ThriftServer, Thrift1OnewayRequestTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto cpp2Server = factory.create();
  cpp2Server->setNWorkerThreads(1);
  cpp2Server->setIsOverloaded([](const THeader*) { return true; });
  apache::thrift::util::ScopedServerThread st(cpp2Server);

  std::shared_ptr<TestServiceClient> client =
      getThrift1Client(*st.getAddress());
  std::string response;
  // Send a oneway request. Server doesn't send error back
  client->noResponse(1);
  // Send a twoway request. Server sends overloading error back
  try {
    client->sendResponse(response, 0);
  } catch (apache::thrift::TApplicationException& ex) {
    EXPECT_STREQ(ex.what(), "loadshedding request");
  } catch (...) {
    ADD_FAILURE();
  }

  cpp2Server->setIsOverloaded([](const THeader*) { return false; });
  // Send another twoway request. Client should receive a response
  // with correct seqId
  client->sendResponse(response, 0);
}

namespace {
class Callback : public RequestCallback {
  void requestSent() override { ADD_FAILURE(); }
  void replyReceived(ClientReceiveState&& state) override { ADD_FAILURE(); }
  void requestError(ClientReceiveState&& state) override {
    try {
      std::rethrow_exception(state.exception());
    } catch (const apache::thrift::transport::TTransportException& ex) {
      // Verify we got a write and not a read error
      // Comparing substring because the rest contains ips and ports
      std::string expected = "transport is closed in write()";
      std::string actual = std::string(ex.what()).substr(0, expected.size());
      EXPECT_EQ(expected, actual);
    } catch (...) {
      ADD_FAILURE();
    }
  }
};
}

TEST(ThriftServer, BadSendTest) {
  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  client.sendResponse(std::unique_ptr<RequestCallback>(new Callback), 64);

  socket->shutdownWriteNow();
  base.loop();

  std::string response;
  EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
}

TEST(ThriftServer, ResetStateTest) {
  folly::EventBase base;

  // Create a server socket and bind, don't listen.  This gets us a
  // port to test with which is guaranteed to fail.
  auto ssock = std::unique_ptr<folly::AsyncServerSocket,
                               folly::DelayedDestruction::Destructor>(
      new folly::AsyncServerSocket);
  ssock->bind(0);
  EXPECT_FALSE(ssock->getAddresses().empty());

  // We do this loop a bunch of times, because the bug which caused
  // the assertion failure was a lost race, which doesn't happen
  // reliably.
  for (int i = 0; i < 1000; ++i) {
    std::shared_ptr<TAsyncSocket> socket(
        TAsyncSocket::newSocket(&base, ssock->getAddresses()[0]));

    // Create a client.
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

    std::string response;
    // This will fail, because there's no server.
    EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
    // On a failed client object, this should also throw an exception.
    // In the past, this would generate an assertion failure and
    // crash.
    EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
  }
}

TEST(ThriftServer, FailureInjection) {
  enum ExpectedFailure { NONE = 0, ERROR, TIMEOUT, DISCONNECT, END };

  std::atomic<ExpectedFailure> expected(NONE);

  using apache::thrift::transport::TTransportException;

  class Callback : public RequestCallback {
   public:
    explicit Callback(const std::atomic<ExpectedFailure>* expected)
        : expected_(expected) {}

   private:
    void requestSent() override {}

    void replyReceived(ClientReceiveState&& state) override {
      std::string response;
      try {
        TestServiceAsyncClient::recv_sendResponse(response, state);
        EXPECT_EQ(NONE, *expected_);
      } catch (const apache::thrift::TApplicationException& ex) {
        const auto& headers = state.header()->getHeaders();
        EXPECT_TRUE(headers.find("ex") != headers.end() &&
                    headers.find("ex")->second == kInjectedFailureErrorCode);
        EXPECT_EQ(ERROR, *expected_);
      } catch (...) {
        ADD_FAILURE() << "Unexpected exception thrown";
      }

      // Now do it again with exception_wrappers.
      auto ew =
          TestServiceAsyncClient::recv_wrapped_sendResponse(response, state);
      if (ew) {
        EXPECT_TRUE(
            ew.is_compatible_with<apache::thrift::TApplicationException>());
        EXPECT_EQ(ERROR, *expected_);
      } else {
        EXPECT_EQ(NONE, *expected_);
      }
    }

    void requestError(ClientReceiveState&& state) override {
      try {
        std::rethrow_exception(state.exception());
      } catch (const TTransportException& ex) {
        if (ex.getType() == TTransportException::TIMED_OUT) {
          EXPECT_EQ(TIMEOUT, *expected_);
        } else {
          EXPECT_EQ(DISCONNECT, *expected_);
        }
      } catch (...) {
        ADD_FAILURE() << "Unexpected exception thrown";
      }
    }

    const std::atomic<ExpectedFailure>* expected_;
  };

  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto server = std::dynamic_pointer_cast<ThriftServer>(sst.getServer().lock());
  CHECK(server);
  SCOPE_EXIT { server->setFailureInjection(ThriftServer::FailureInjection()); };

  RpcOptions rpcOptions;
  rpcOptions.setTimeout(std::chrono::milliseconds(100));
  for (int i = 0; i < END; ++i) {
    auto exp = static_cast<ExpectedFailure>(i);
    ThriftServer::FailureInjection fi;

    switch (exp) {
      case NONE:
        break;
      case ERROR:
        fi.errorFraction = 1;
        break;
      case TIMEOUT:
        fi.dropFraction = 1;
        break;
      case DISCONNECT:
        fi.disconnectFraction = 1;
        break;
      case END:
        LOG(FATAL) << "unreached";
        break;
    }

    server->setFailureInjection(std::move(fi));

    expected = exp;

    auto callback = folly::make_unique<Callback>(&expected);
    client.sendResponse(rpcOptions, std::move(callback), 1);
    base.loop();
  }
}

TEST(ThriftServer, useExistingSocketAndExit) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  folly::AsyncServerSocket::UniquePtr serverSocket(
      new folly::AsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));
  // In the past, this would cause a SEGV
}

TEST(ThriftServer, useExistingSocketAndConnectionIdleTimeout) {
  // This is ConnectionIdleTimeoutTest, but with an existing socket
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  folly::AsyncServerSocket::UniquePtr serverSocket(
      new folly::AsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));

  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  folly::EventBase base;
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *st.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

namespace {
class ReadCallbackTest : public TAsyncTransport::ReadCallback {
 public:
  void getReadBuffer(void**, size_t*) override {}
  void readDataAvailable(size_t) noexcept override {}
  void readEOF() noexcept override { eof = true; }

  void readError(const transport::TTransportException&) noexcept override {
    eof = true;
  }

  bool eof = false;
};
}

TEST(ThriftServer, ShutdownSocketSetTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  ScopedServerThread sst(server);
  folly::EventBase base;
  ReadCallbackTest cb;

  std::shared_ptr<TAsyncSocket> socket2(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));
  socket2->setReadCallback(&cb);

  base.tryRunAfterDelay([&]() { server->immediateShutdown(true); }, 10);
  base.tryRunAfterDelay([&]() { base.terminateLoopSoon(); }, 30);
  base.loopForever();
  EXPECT_EQ(cb.eof, true);
}

TEST(ThriftServer, ShutdownDegenarateServer) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  server->setMaxRequests(1);
  server->setNWorkerThreads(1);
  ScopedServerThread sst(server);
}

TEST(ThriftServer, ModifyingIOThreadCountLive) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  auto iothreadpool = std::make_shared<wangle::IOThreadPoolExecutor>(0);
  server->setIOThreadPool(iothreadpool);

  ScopedServerThread sst(server);
  // If there are no worker threads, generally the server event base
  // will stop loop()ing.  Create a timeout event to make sure
  // it continues to loop for the duration of the test.
  server->getServeEventBase()->runInEventBaseThread(
      [&]() { server->getServeEventBase()->tryRunAfterDelay([]() {}, 5000); });

  server->getServeEventBase()->runInEventBaseThreadAndWait(
      [=]() { iothreadpool->setNumThreads(0); });

  folly::EventBase base;

  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;

  boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel())
      ->setTimeout(100);

  // This should fail as soon as it connects:
  // since AsyncServerSocket has no accept callbacks installed,
  // it should close the connection right away.
  ASSERT_ANY_THROW(client.sync_sendResponse(response, 64));

  server->getServeEventBase()->runInEventBaseThreadAndWait(
      [=]() { iothreadpool->setNumThreads(30); });

  std::shared_ptr<TAsyncSocket> socket2(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

  // Can't reuse client since the channel has gone bad
  TestServiceAsyncClient client2(HeaderClientChannel::newChannel(socket2));

  client2.sync_sendResponse(response, 64);
}

TEST(ThriftServer, setIOThreadPool) {
  auto exe = std::make_shared<wangle::IOThreadPoolExecutor>(1);
  TestThriftServerFactory<TestInterface> factory;
  factory.useSimpleThreadManager(false);
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());

  // Set the exe, this used to trip various calls like
  // CHECK(ioThreadPool->numThreads() == 0).
  server->setIOThreadPool(exe);
  EXPECT_EQ(1, server->getNWorkerThreads());
}

namespace {
class ExtendedTestServiceAsyncProcessor : public TestServiceAsyncProcessor {
 public:
  explicit ExtendedTestServiceAsyncProcessor(TestServiceSvIf* serviceInterface)
      : TestServiceAsyncProcessor(serviceInterface) {}

  folly::Optional<std::string> getCacheKeyTest() {
    folly::IOBuf emptyBuffer;
    return getCacheKey(
        &emptyBuffer,
        apache::thrift::protocol::PROTOCOL_TYPES::T_BINARY_PROTOCOL);
  }
};
}

TEST(ThriftServer, CacheAnnotation) {
  // We aren't parsing anything just want this to compile
  auto testInterface = std::unique_ptr<TestInterface>(new TestInterface);
  ExtendedTestServiceAsyncProcessor processor(testInterface.get());
  EXPECT_FALSE(processor.getCacheKeyTest().hasValue());
}
