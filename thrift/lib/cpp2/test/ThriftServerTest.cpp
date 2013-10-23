/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <gtest/gtest.h>
#include "thrift/lib/cpp2/test/gen-cpp2/TestService.h"
#include "thrift/lib/cpp2/test/gen-cpp/TestService.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"

#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"

#include "thrift/lib/cpp2/async/StubSaslClient.h"
#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include "thrift/lib/cpp2/test/TestUtils.h"

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::protocol::TBinaryProtocolT;
using apache::thrift::test::TestServiceClient;

class TestInterface : public TestServiceSvIf {
  void sendResponse(std::string& _return, int64_t size) {
    if (size >= 0) {
      usleep(size);
    }

    EXPECT_NE("", getConnectionContext()->getPeerAddress()->describe());

    _return = "test" + boost::lexical_cast<std::string>(size);
  }

  void noResponse(int64_t size) {
    usleep(size);
  }

  void echoRequest(std::string& _return, std::unique_ptr<std::string> req) {
    _return = *req + "ccccccccccccccccccccccccccccccccccccccccccccc";
  }

  typedef apache::thrift::HandlerCallback<std::unique_ptr<std::string>>
      StringCob;
  void async_tm_serializationTest(std::unique_ptr<StringCob> callback,
                                  bool inEventBase) {
    std::unique_ptr<std::string> sp(new std::string("hello world"));
    auto st = inEventBase ? SerializationThread::EVENT_BASE :
                            SerializationThread::CURRENT;
    callback.release()->resultInThread(std::move(sp), st);
  }

  void async_eb_eventBaseAsync(std::unique_ptr<StringCob> callback) {
    std::unique_ptr<std::string> hello(new std::string("hello world"));
    callback->result(std::move(hello));
  }
};

std::shared_ptr<ThriftServer> getServer() {
  std::shared_ptr<ThriftServer> server(new ThriftServer);
  std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory(
      new apache::thrift::concurrency::PosixThreadFactory);
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    threadManager(
        apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(1,
          5,
          false,
          2));
  threadManager->threadFactory(threadFactory);
  threadManager->start();
  server->setThreadManager(threadManager);
  server->setPort(0);
  server->setSaslEnabled(true);
  server->setSaslServerFactory(
      [] (apache::thrift::async::TEventBase* evb) {
        return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
      }
  );
  server->setInterface(std::unique_ptr<TestInterface>(new TestInterface));
  return server;
}

std::shared_ptr<TestServiceClient> getThrift1Client(uint16_t port) {
  // Create Thrift1 clients
  TSocketAddress address("127.0.0.1", port);
  std::shared_ptr<TSocket> socket = std::make_shared<TSocket>(address);
  socket->open();
  std::shared_ptr<TFramedTransport> transport =
      std::make_shared<TFramedTransport>(socket);
  std::shared_ptr<TBinaryProtocolT<TBufferBase>> protocol =
      std::make_shared<TBinaryProtocolT<TBufferBase>>(transport);
  return std::make_shared<TestServiceClient>(protocol);
}

void AsyncCpp2Test(bool enable_security) {
  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  if (enable_security) {
    client_channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    client_channel->setSaslClient(std::unique_ptr<SaslClient>(
      new StubSaslClient(socket->getEventBase())
    ));
  }
  TestServiceAsyncClient client(std::move(client_channel));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);
  client.sendResponse([](ClientReceiveState&& state) {
                        std::string response;
                        try {
                          TestServiceAsyncClient::recv_sendResponse(
                              response, state);
                        } catch(const std::exception& ex) {
                        }
                        EXPECT_EQ(response, "test64");
                      },
                      64);
  base.loop();
}

TEST(ThriftServer, InsecureAsyncCpp2Test) {
  AsyncCpp2Test(false);
}

TEST(ThriftServer, SecureAsyncCpp2Test) {
  AsyncCpp2Test(true);
}

TEST(ThriftServer, SyncClientTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);
  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));
  try {
    // should timeout
    client.sync_sendResponse(options, response, 10000);
  } catch (const TTransportException& e) {
    EXPECT_EQ(int(TTransportException::TIMED_OUT), int(e.getType()));
    return;
  }
  ADD_FAILURE();
}

TEST(ThriftServer, SerializationInEventBaseTest) {
  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto channel =
      std::unique_ptr<HeaderClientChannel,
                      apache::thrift::async::TDelayedDestruction::Destructor>(
                          new HeaderClientChannel(socket));
  channel->setTimeout(10000);

  TestServiceAsyncClient client(std::move(channel));

  std::string response;
  client.sync_serializationTest(response, true);
  EXPECT_EQ("hello world", response);
}

TEST(ThriftServer, HandlerInEventBaseTest) {
  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto channel =
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                        new HeaderClientChannel(socket));
  channel->setTimeout(10000);

  TestServiceAsyncClient client(std::move(channel));

  std::string response;
  client.sync_eventBaseAsync(response);
  EXPECT_EQ("hello world", response);

}

TEST(ThriftServer, LargeSendTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  std::string request;
  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(5000);
  request.reserve(0x3fffffff);
  for (uint32_t i = 0; i < 0x3fffffd0 / 30; i++) {
    request += "cccccccccccccccccccccccccccccc";
  }

  try {
    // should timeout
    client.sync_echoRequest(response, request);
  } catch (const TTransportException& e) {
    EXPECT_EQ(int(TTransportException::TIMED_OUT), int(e.getType()));
    return;
  }
  ADD_FAILURE();
}

TEST(ThriftServer, OverloadTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(500);

  auto tval = 10000;
  int too_full = 0;
  int exception_headers = 0;
  auto lambda = [&](ClientReceiveState&& state) {
      std::string response;
      try {
        auto header = boost::polymorphic_downcast<HeaderClientChannel*>(
          client.getChannel())->getHeader();
        auto headers = header->getHeaders();
        if (headers.size() > 0) {
          EXPECT_EQ(headers["ex"], kOverloadedErrorCode);
          exception_headers++;
        }
        TestServiceAsyncClient::recv_sendResponse(response, state);
      } catch (apache::thrift::TApplicationException& e) {
        usleep(tval); // Wait for large task to finish
        too_full++;
      }
  };

  // Fill up the server's request buffer
  client.sendResponse(lambda, tval);
  client.sendResponse(lambda, 0);
  client.sendResponse(lambda, 0);
  client.sendResponse(lambda, 0);
  base.loop();

  // We expect one 'too full' exception (queue size is 2, one being worked on)
  // And three timeouts
  EXPECT_EQ(too_full, 1);
  EXPECT_EQ(exception_headers, 1);
}

TEST(ThriftServer, OnewaySyncClientTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  client.sync_noResponse(0);
}

TEST(ThriftServer, OnewayClientConnectionCloseTest) {
  static std::atomic<bool> done(false);

  class OnewayTestInterface: public TestServiceSvIf {
    void noResponse(int64_t size) {
        usleep(size);
        done = true;
    }
  };

  std::shared_ptr<ThriftServer> cpp2Server = getServer();
  cpp2Server->setInterface(std::unique_ptr<OnewayTestInterface>(
      new OnewayTestInterface));
  apache::thrift::util::ScopedServerThread st(cpp2Server);

  {
    TEventBase base;
    std::shared_ptr<TAsyncSocket> socket(
          TAsyncSocket::newSocket(&base, "127.0.0.1",
              st.getAddress()->getPort()));
    TestServiceAsyncClient client(
        std::unique_ptr<HeaderClientChannel,
            apache::thrift::async::TDelayedDestruction::Destructor>(
            new HeaderClientChannel(socket)));

    client.sync_noResponse(10000);
  } // client out of scope

  usleep(50000);
  EXPECT_TRUE(done);
}

TEST(ThriftServer, CompactClientTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  // Set the client to compact
  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->getHeader()->setProtocolId(
      ::apache::thrift::protocol::T_COMPACT_PROTOCOL);

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
}

TEST(ThriftServer, CompressionClientTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  // Set the client to compact
  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->getHeader()->setTransform(
      apache::thrift::transport::THeader::ZLIB_TRANSFORM);

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");

  auto trans = boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->getHeader()->getTransforms();
  EXPECT_EQ(trans.size(), 1);
  for (auto& tran : trans) {
    EXPECT_EQ(tran, apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  }
}

TEST(ThriftServer, ClientTimeoutTest) {

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));
  std::unique_ptr<RequestCallback> callback(
      new FunctionReplyCallback(
          [](ClientReceiveState&& state) {
             std::string response;
             if (state.exception()) {
               try {
                 std::rethrow_exception(state.exception());
               } catch (const TTransportException& e) {
                 EXPECT_EQ(int(TTransportException::TIMED_OUT),
                           int(e.getType()));
                 return;
               }
             }
             ADD_FAILURE();
           }));
  client.sendResponse(options, std::move(callback), 10000);
  base.loop();
}

TEST(ThriftServer, ConnectionIdleTimeoutTest) {
  std::shared_ptr<ThriftServer> server = getServer();
  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", st.getAddress()->getPort()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

TEST(ThriftServer, Thrift1OnewayRequestTest) {
  std::shared_ptr<ThriftServer> cpp2Server = getServer();
  cpp2Server->setNWorkerThreads(1);
  cpp2Server->setIsOverloaded([]() {
    return true;
  });
  apache::thrift::util::ScopedServerThread st(cpp2Server);

  std::shared_ptr<TestServiceClient> client = getThrift1Client(
      st.getAddress()->getPort());
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

  cpp2Server->setIsOverloaded([]() {
    return false;
  });
  // Send another twoway request. Client should receive a response
  // with correct seqId
  client->sendResponse(response, 0);
}

class Callback : public RequestCallback {
  void requestSent() {
    ADD_FAILURE();
  }
  void replyReceived(ClientReceiveState&& state) {
    ADD_FAILURE();
  }
  void requestError(ClientReceiveState&& state) {
    try {
      std::rethrow_exception(state.exception());
    } catch(const apache::thrift::transport::TTransportException& ex) {
      // Verify we got a write and not a read error
      EXPECT_STREQ(ex.what(), "write() called with socket in invalid state");
    } catch (...) {
      ADD_FAILURE();
    }
  }
};

TEST(ThriftServer, BadSendTest) {
  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  client.sendResponse(
    std::unique_ptr<RequestCallback>(new Callback), 64);

  socket->shutdownWriteNow();
  base.loop();
}

TEST(ThriftServer, FailureInjection) {
  enum ExpectedFailure {
    NONE = 0,
    ERROR,
    TIMEOUT,
    DISCONNECT,
    END
  };

  std::atomic<ExpectedFailure> expected(NONE);

  using apache::thrift::transport::TTransportException;

  class Callback : public RequestCallback {
   public:
    explicit Callback(const std::atomic<ExpectedFailure>* expected)
      : expected_(expected) { }

   private:
    void requestSent() {
    }

    void replyReceived(ClientReceiveState&& state) {
      std::string response;
      try {
        TestServiceAsyncClient::recv_sendResponse(response, state);
        EXPECT_EQ(NONE, *expected_);
      } catch (const apache::thrift::TApplicationException& ex) {
        EXPECT_EQ(ERROR, *expected_);
      } catch (...) {
        ADD_FAILURE() << "Unexpected exception thrown";
      }
    }

    void requestError(ClientReceiveState&& state) {
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

  auto port = Server::get(getServer)->getAddress().getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
      std::unique_ptr<HeaderClientChannel,
                      apache::thrift::async::TDelayedDestruction::Destructor>(
          new HeaderClientChannel(socket)));

  auto server = Server::get(getServer);
  CHECK(server);
  SCOPE_EXIT {
    server->setFailureInjection(ThriftServer::FailureInjection());
  };

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
  auto server = getServer();
  TAsyncServerSocket::UniquePtr serverSocket(new TAsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));
  // In the past, this would cause a SEGV
}

TEST(ThriftServer, useExistingSocketAndConnectionIdleTimeout) {
  // This is ConnectionIdleTimeoutTest, but with an existing socket
  auto server = getServer();
  TAsyncServerSocket::UniquePtr serverSocket(new TAsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));

  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", st.getAddress()->getPort()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
