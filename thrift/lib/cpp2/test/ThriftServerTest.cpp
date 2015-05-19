/*
 * Copyright 2014 Facebook, Inc.
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

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncServerSocket.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

#include <folly/experimental/fibers/FiberManagerMap.h>
#include <folly/wangle/concurrent/GlobalExecutor.h>

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
    callback->result(std::move(sp));
  }

  void async_eb_eventBaseAsync(std::unique_ptr<StringCob> callback) {
    std::unique_ptr<std::string> hello(new std::string("hello world"));
    callback->result(std::move(hello));
  }

  void async_tm_notCalledBack(std::unique_ptr<
                              apache::thrift::HandlerCallback<void>> cb) {
  }
};

std::shared_ptr<ThriftServer> getServer(
  bool useSimpleThreadManager = true,
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe = nullptr) {

  std::shared_ptr<ThriftServer> server(new ThriftServer);
  if (useSimpleThreadManager) {
    std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory(
        new apache::thrift::concurrency::PosixThreadFactory);
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager(
        apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(
            1, 5, false, 2));
    threadManager->threadFactory(threadFactory);
    threadManager->start();
    server->setThreadManager(threadManager);
  } else if (exe) {
    server->setThreadManager(exe);
  }
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

void AsyncCpp2Test(bool enable_security) {

  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  if (enable_security) {
    client_channel->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    client_channel->setSaslClient(std::unique_ptr<SaslClient>(
      new StubSaslClient(socket->getEventBase())
    ));
  }
  TestServiceAsyncClient client(std::move(client_channel));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);
  client.sendResponse([&](ClientReceiveState&& state) {
    std::string response;
    try {
      TestServiceAsyncClient::recv_sendResponse(
          response, state);
    } catch(const std::exception& ex) {
    }
    EXPECT_EQ(response, "test64");
  }, 64);
  base.loop();
}

TEST(ThriftServer, InsecureAsyncCpp2Test) {
  AsyncCpp2Test(false);
}

TEST(ThriftServer, SecureAsyncCpp2Test) {
  AsyncCpp2Test(true);
}

TEST(ThriftServer, SyncClientTest) {
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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

TEST(ThriftServer, GetLoadTest) {
  auto serv = getServer();
  ScopedServerThread sst(serv);
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  auto header_channel = boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel());
  header_channel->getHeader()->setHeader("load", "thrift.active_requests");
  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
  auto headers = header_channel->getHeader()->getHeaders();
  auto load = headers.find("load");
  EXPECT_NE(load, headers.end());
  EXPECT_EQ(load->second, "0");

  serv->setGetLoad([&](std::string counter){
    EXPECT_EQ(counter, "thrift.active_requests");
    return 1;
  });

  header_channel->getHeader()->setHeader("load", "thrift.active_requests");
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
  headers = header_channel->getHeader()->getHeaders();
  load = headers.find("load");
  EXPECT_NE(load, headers.end());
  EXPECT_EQ(load->second, "1");
}

TEST(ThriftServer, SerializationInEventBaseTest) {
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
    sleep(1); // Wait for server to timeout also - otherwise other tests fail
    return;
  }
  ADD_FAILURE();
}

TEST(ThriftServer, OverloadTest) {
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
      auto header = boost::polymorphic_downcast<HeaderClientChannel*>(
          client.getChannel())->getHeader();
      auto headers = header->getHeaders();
      if (headers.size() > 0) {
        EXPECT_EQ(headers["ex"], kQueueOverloadedErrorCode);
        exception_headers++;
      }
      auto ew = TestServiceAsyncClient::recv_wrapped_sendResponse(response,
                                                                  state);
      if (ew) {
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
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
      TAsyncSocket::newSocket(&base, *st.getAddress()));
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
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

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
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  // Set the client to compact
  auto header = boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->getHeader();
  header->setTransform(
    apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  header->setMinCompressBytes(1);

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
  auto server = getServer();
  ScopedServerThread sst(server);
  TEventBase base;

  auto getClient = [&base, &sst] () {
    std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, *sst.getAddress()));

    return std::make_shared<TestServiceAsyncClient>(
      std::unique_ptr<HeaderClientChannel,
                      apache::thrift::async::TDelayedDestruction::Destructor>(
                        new HeaderClientChannel(socket)));
  };

  int cbCtor = 0;
  int cbCall = 0;

  auto callback = [&cbCall, &cbCtor] (
      std::shared_ptr<TestServiceAsyncClient> client,
      bool& timeout) {
    cbCtor++;
    return std::unique_ptr<RequestCallback>(
      new FunctionReplyCallback(
        [&cbCall, client, &timeout] (ClientReceiveState&& state) {
          cbCall++;
          if (state.exception()) {
            timeout = true;
            try {
              std::rethrow_exception(state.exception());
            } catch (const TTransportException& e) {
              EXPECT_EQ(int(TTransportException::TIMED_OUT),
                        int(e.getType()));

            }
            return;
          }
          try {
            std::string resp;
            client->recv_sendResponse(resp, state);
          } catch (const TApplicationException& e) {
            timeout = true;
            EXPECT_EQ(int(TApplicationException::TIMEOUT),
                      int(e.getType()));
            return;
          }
          timeout = false;
        }));
  };

  // Set the timeout to be 9 milliseconds, but the call will take 10 ms.
  // The server should send a timeout after 9 milliseconds
  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(9));
  auto client1 = getClient();
  bool timeout1;
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_TRUE(timeout1);
  usleep(10000);

  // This time we set the timeout to be 11 millseconds.  The server
  // should not time out
  options.setTimeout(std::chrono::milliseconds(11));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_FALSE(timeout1);
  usleep(10000);

  // This time we set server timeout to be 1 millsecond.  However, the
  // task should start processing within that millisecond, so we should
  // not see an exception because the client timeout should be used after
  // processing is started
  server->setTaskExpireTime(std::chrono::milliseconds(1));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  usleep(10000);

  // The server timeout stays at 1 ms, but we put the client timeout at
  // 9 ms.  We should timeout even though the server starts processing within
  // 1ms.
  options.setTimeout(std::chrono::milliseconds(9));
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
  std::shared_ptr<ThriftServer> server = getServer();
  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *st.getAddress()));

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
    *st.getAddress());
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
      // Comparing substring because the rest contains ips and ports
      std::string expected = "transport is closed in write()";
      std::string actual = std::string(ex.what()).substr(0, expected.size());
      EXPECT_EQ(expected, actual);
    } catch (...) {
      ADD_FAILURE();
    }
  }
};

TEST(ThriftServer, BadSendTest) {
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  client.sendResponse(
    std::unique_ptr<RequestCallback>(new Callback), 64);

  socket->shutdownWriteNow();
  base.loop();

  std::string response;
  EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
}

TEST(ThriftServer, ResetStateTest) {
  TEventBase base;

  // Create a server socket and bind, don't listen.  This gets us a
  // port to test with which is guaranteed to fail.
  auto ssock = std::unique_ptr<
    TAsyncServerSocket,
    apache::thrift::async::TDelayedDestruction::Destructor>(
      new TAsyncServerSocket);
  ssock->bind(0);
  EXPECT_FALSE(ssock->getAddresses().empty());

  // We do this loop a bunch of times, because the bug which caused
  // the assertion failure was a lost race, which doesn't happen
  // reliably.
  for (int i = 0; i < 1000; ++i) {
    std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&base, ssock->getAddresses()[0]));

    // Create a client.
    TestServiceAsyncClient client(
      std::unique_ptr<HeaderClientChannel,
      apache::thrift::async::TDelayedDestruction::Destructor>(
        new HeaderClientChannel(socket)));

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

      // Now do it again with exception_wrappers.
      auto ew = TestServiceAsyncClient::recv_wrapped_sendResponse(response,
                                                                  state);
      if (ew) {
        EXPECT_TRUE(
          ew.is_compatible_with<apache::thrift::TApplicationException>());
        EXPECT_EQ(ERROR, *expected_);
      } else {
        EXPECT_EQ(NONE, *expected_);
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

  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
      std::unique_ptr<HeaderClientChannel,
                      apache::thrift::async::TDelayedDestruction::Destructor>(
          new HeaderClientChannel(socket)));

  auto server = std::dynamic_pointer_cast<ThriftServer>(
      sst.getServer().lock());
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
    TAsyncSocket::newSocket(&base, *st.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

TEST(ThriftServer, FreeCallbackTest) {
  ScopedServerThread sst(getServer());
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));

  try {
    client.sync_notCalledBack(options);
  } catch (...) {
    // Expect timeout
    return;
  }
  ADD_FAILURE();
}

class TestServerEventHandler
    : public server::TServerEventHandler
    , public TProcessorEventHandler
    , public TProcessorEventHandlerFactory
    , public std::enable_shared_from_this<TestServerEventHandler> {
 public:

  std::shared_ptr<TProcessorEventHandler> getEventHandler() {
    return shared_from_this();
  }

  void check() {
    EXPECT_EQ(8, count);
  }
  void preServe(const folly::SocketAddress* addr) {
    EXPECT_EQ(0, count++);
  }
  void newConnection(TConnectionContext* ctx) {
    EXPECT_EQ(1, count++);
  }
  void connectionDestroyed(TConnectionContext* ctx) {
    EXPECT_EQ(7, count++);
  }

  void* getContext(const char* fn_name,
                   TConnectionContext* c) {
    EXPECT_EQ(2, count++);
    return nullptr;
  }
  void freeContext(void* ctx, const char* fn_name) {
    EXPECT_EQ(6, count++);
  }
  void preRead(void* ctx, const char* fn_name) {
    EXPECT_EQ(3, count++);

  }
  void onReadData(void* ctx, const char* fn_name,
                          const SerializedMessage& msg) {
    EXPECT_EQ(4, count++);
  }

  void postRead(void* ctx, const char* fn_name, uint32_t bytes) {
    EXPECT_EQ(5, count++);
  }


 private:
  std::atomic<int> count{0};
};

TEST(ThriftServer, CallbackOrderingTest) {
  auto server = getServer();
  auto serverHandler = std::make_shared<TestServerEventHandler>();


  TProcessorBase::addProcessorEventHandlerFactory(serverHandler);
  server->setServerEventHandler(serverHandler);

  ScopedServerThread sst(server);
  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  client.noResponse([](ClientReceiveState&& state){}, 10000);
  base.tryRunAfterDelay([&](){
    socket->closeNow();
  }, 1);
  base.tryRunAfterDelay([&](){
    base.terminateLoopSoon();
  }, 20);
  base.loopForever();
  serverHandler->check();
  TProcessorBase::removeProcessorEventHandlerFactory(serverHandler);
}

class ReadCallbackTest : public TAsyncTransport::ReadCallback {
 public:
  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
  }
  virtual void readDataAvailable(size_t len) noexcept {
  }
  virtual void readEOF() noexcept {
    eof = true;
  }

  virtual void readError(const transport::TTransportException& ex) noexcept {
    eof = true;
  }

  bool eof = false;
};

TEST(ThriftServer, ShutdownSocketSetTest) {
  auto server = getServer();
  ScopedServerThread sst(server);
  TEventBase base;
  ReadCallbackTest cb;

  std::shared_ptr<TAsyncSocket> socket2(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));
  socket2->setReadCallback(&cb);

  base.tryRunAfterDelay([&](){
      server->immediateShutdown(true);
    }, 10);
  base.tryRunAfterDelay([&](){
      base.terminateLoopSoon();
    }, 30);
  base.loopForever();
  EXPECT_EQ(cb.eof, true);
}

TEST(ThriftServer, ShutdownDegenarateServer) {
  auto server = getServer(false);
  server->setMaxRequests(1);
  server->setNWorkerThreads(1);
  ScopedServerThread sst(server);
}

TEST(ThriftServer, ModifyingIOThreadCountLive) {
  auto server = getServer();
  auto iothreadpool = std::make_shared<folly::wangle::IOThreadPoolExecutor>(0);
  server->setIOThreadPool(iothreadpool);

  ScopedServerThread sst(server);
  // If there are no worker threads, generally the server event base
  // will stop loop()ing.  Create a timeout event to make sure
  // it continues to loop for the duration of the test.
  server->getServeEventBase()->runInEventBaseThread([&](){
    server->getServeEventBase()->tryRunAfterDelay([](){}, 5000);
  });

  server->getServeEventBase()->runInEventBaseThreadAndWait([=](){
    iothreadpool->setNumThreads(0);
  });

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(100);

  // This should fail as soon as it connects:
  // since AsyncServerSocket has no accept callbacks installed,
  // it should close the connection right away.
  ASSERT_ANY_THROW(
  client.sync_sendResponse(response, 64));

  server->getServeEventBase()->runInEventBaseThreadAndWait([=](){
    iothreadpool->setNumThreads(30);
  });

  std::shared_ptr<TAsyncSocket> socket2(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  // Can't reuse client since the channel has gone bad
  TestServiceAsyncClient client2(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket2)));

  client2.sync_sendResponse(response, 64);
}

TEST(ThriftServer, ThriftServerSizeLimits) {
  google::FlagSaver flagSaver;
  FLAGS_thrift_cpp2_protocol_reader_string_limit = 1024 * 1024;

  ScopedServerThread sst(getServer());
  TEventBase eb;

  TestServiceAsyncClient client(
      HeaderClientChannel::newChannel(
        TAsyncSocket::newSocket(
          &eb, *sst.getAddress())));

  std::string response;

  try {
    // make a largest possible input which should not throw an exception
    std::string smallInput(1 << 19, '1');
    client.sync_echoRequest(response, smallInput);
    SUCCEED();
  } catch(const std::exception& ex) {
    ADD_FAILURE();
  }

  // make an input that is too large by 1 byte
  std::string largeInput(1 << 21, '1');
  EXPECT_THROW(client.sync_echoRequest(response, largeInput), std::exception);
}

class MyExecutor : public folly::Executor {
 public:
  virtual void add(std::function<void()> f) {
    calls++;
    f();
  }

  std::atomic<int> calls{0};
};

TEST(ThriftServer, poolExecutorTest) {
  auto exe = std::make_shared<MyExecutor>();
  ScopedServerThread sst(
    getServer(
      false,
      std::make_shared<apache::thrift::concurrency::ThreadManagerExecutorAdapter>(
        exe)));
  TEventBase eb;

  TestServiceAsyncClient client(
      HeaderClientChannel::newChannel(
        TAsyncSocket::newSocket(
          &eb, *sst.getAddress())));

  std::string response;

  client.sync_echoRequest(response, "test");
  eb.loop();
  EXPECT_EQ(1, exe->calls);
}

class FiberExecutor : public folly::Executor {
 public:
  virtual void add(std::function<void()> f) {
    folly::fibers::getFiberManager(
      *folly::wangle::getIOExecutor()->getEventBase()).add(f);
  }
};

TEST(ThriftServer, fiberExecutorTest) {
  auto exe =
    std::make_shared<apache::thrift::concurrency::ThreadManagerExecutorAdapter>(
      std::make_shared<FiberExecutor>());
  ScopedServerThread sst(getServer(false, exe));
  TEventBase eb;

  TestServiceAsyncClient client(
      HeaderClientChannel::newChannel(
        TAsyncSocket::newSocket(
          &eb, *sst.getAddress())));

  std::string response;

  client.sync_sendResponse(response, 1);
  eb.loop();
  EXPECT_EQ("test1", response);
}

TEST(ThriftServer, setIOThreadPool) {
  auto exe = std::make_shared<folly::wangle::IOThreadPoolExecutor>(1);
  auto server = getServer(false);

  // Set the exe, this used to trip various calls like
  // CHECK(ioThreadPool->numThreads() == 0).
  server->setIOThreadPool(exe);
  EXPECT_EQ(1, server->getNWorkerThreads());
}

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
TEST(ThriftServer, CacheAnnotation) {
  // We aren't parsing anything just want this to compile
  auto testInterface = std::unique_ptr<TestInterface>(new TestInterface);
  ExtendedTestServiceAsyncProcessor processor(testInterface.get());
  EXPECT_FALSE(processor.getCacheKeyTest().hasValue());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
