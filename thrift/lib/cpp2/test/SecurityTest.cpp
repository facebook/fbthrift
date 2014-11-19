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
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DuplexService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DuplexClient.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>

#include <thrift/lib/cpp2/async/GssSaslClient.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <atomic>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace folly;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

class TestServiceInterface : public TestServiceSvIf {
public:
  void async_tm_sendResponse(
      unique_ptr<HandlerCallback<unique_ptr<std::string>>> callback,
      int64_t size) override {
    EXPECT_NE(callback->getConnectionContext()->
                getSaslServer()->getClientIdentity(), "");
    callback.release()->resultInThread(folly::to<std::string>(size));
  }
};

std::shared_ptr<ThriftServer> getServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(folly::make_unique<TestServiceInterface>());
  server->setSaslEnabled(true);
  return server;
}

void enableSecurity(HeaderClientChannel* channel) {
  char hostname[256];
  EXPECT_EQ(gethostname(hostname, 255), 0);

  // Construct the service identity prefix from hostname.
  // Service identity should be sys.<hostname_scheme_prefix>@hostname.
  std::string hostnameStr(hostname);
  size_t end = hostnameStr.find_first_not_of("abcdefghijklmnopqrstuvwxyz");
  std::string serviceIdentityPrefix = std::string("sys.") +
    hostnameStr.substr(0, end) + std::string("@");

  std::string clientIdentity = std::string("host/") + hostname;
  std::string serviceIdentity = serviceIdentityPrefix + hostname;

  channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);

  auto saslClient = folly::make_unique<GssSaslClient>(channel->getEventBase());
  saslClient->setClientIdentity(clientIdentity);
  saslClient->setServiceIdentity(serviceIdentity);
  saslClient->setSaslThreadManager(make_shared<SaslThreadManager>(
      make_shared<SecurityLogger>()));
  saslClient->setCredentialsCacheManager(
      make_shared<krb5::Krb5CredentialsCacheManager>());
  channel->setSaslClient(std::move(saslClient));
}

HeaderClientChannel::Ptr getClientChannel(TEventBase* eb, uint16_t port) {
  auto socket = TAsyncSocket::newSocket(eb, "127.0.0.1", port);
  auto channel = HeaderClientChannel::newChannel(socket);

  enableSecurity(channel.get());

  return std::move(channel);
}

ScopedServerThread sst(getServer());

class Countdown {
public:
  Countdown(int count, std::function<void()> f)
    : count_(count), f_(f)
  {}
  void down() {
    if (--count_ == 0) {
      f_();
    }
  }
private:
  int count_;
  std::function<void()> f_;
};

void runTest(std::function<void(HeaderClientChannel* channel)> setup) {
  TEventBase base;

  auto port = sst.getAddress()->getPort();
  auto channel = getClientChannel(&base, port);
  setup(channel.get());
  TestServiceAsyncClient client(std::move(channel));
  Countdown c(3, [&base](){base.terminateLoopSoon();});

  client.sendResponse([&base,&client,&c](ClientReceiveState&& state) {
    EXPECT_FALSE(state.isException());
    EXPECT_TRUE(state.isSecurityActive());
    std::string res;
    try {
      TestServiceAsyncClient::recv_sendResponse(res, state);
    } catch(const std::exception&) {
      EXPECT_TRUE(false);
    }
    EXPECT_EQ(res, "10");
    c.down();
  }, 10);


  // fail on time out
  base.runAfterDelay([] {EXPECT_TRUE(false);}, 5000);

  base.runAfterDelay([&client,&base,&c] {
    client.sendResponse([&base,&c](ClientReceiveState&& state) {
      EXPECT_FALSE(state.isException());
      EXPECT_TRUE(state.isSecurityActive());
      std::string res;
      try {
        TestServiceAsyncClient::recv_sendResponse(res, state);
      } catch(const std::exception&) {
        EXPECT_TRUE(false);
      }
      EXPECT_EQ(res, "10");
      c.down();
    }, 10);
    client.sendResponse([&base,&c](ClientReceiveState&& state) {
      EXPECT_FALSE(state.isException());
      EXPECT_TRUE(state.isSecurityActive());
      std::string res;
      try {
        TestServiceAsyncClient::recv_sendResponse(res, state);
      } catch(const std::exception&) {
        EXPECT_TRUE(false);
      }
      EXPECT_EQ(res, "10");
      c.down();
    }, 10);
  }, 1);

  base.loopForever();
}


TEST(Security, Basic) {
  runTest([](HeaderClientChannel* channel) {});
}

TEST(Security, CompressionZlib) {
  runTest([](HeaderClientChannel* channel) {
    auto header = channel->getHeader();
    header->setTransform(transport::THeader::ZLIB_TRANSFORM);
  });
}

TEST(Security, CompressionSnappy) {
  runTest([](HeaderClientChannel* channel) {
    auto header = channel->getHeader();
    header->setTransform(transport::THeader::SNAPPY_TRANSFORM);
  });
}

TEST(Security, DISABLED_CompressionQlz) {
  runTest([](HeaderClientChannel* channel) {
    auto header = channel->getHeader();
    header->setTransform(transport::THeader::QLZ_TRANSFORM);
  });
}

TEST(Security, ProtocolBinary) {
  runTest([](HeaderClientChannel* channel) {
    auto header = channel->getHeader();
    header->setProtocolId(protocol::T_BINARY_PROTOCOL);
  });
}

TEST(Security, ProtocolCompact) {
  runTest([](HeaderClientChannel* channel) {
    auto header = channel->getHeader();
    header->setProtocolId(protocol::T_COMPACT_PROTOCOL);
  });
}


class DuplexClientInterface : public DuplexClientSvIf {
public:
  DuplexClientInterface(int32_t first, int32_t count, bool& success)
    : expectIndex_(first), lastIndex_(first + count), success_(success)
  {}

  void async_tm_update(unique_ptr<HandlerCallback<int32_t>> callback,
                       int32_t currentIndex) override {
    auto callbackp = callback.release();
    EXPECT_EQ(currentIndex, expectIndex_);
    expectIndex_++;
    TEventBase *eb = callbackp->getEventBase();
    callbackp->resultInThread(currentIndex);
    if (expectIndex_ == lastIndex_) {
      success_ = true;
      eb->runInEventBaseThread([eb] { eb->terminateLoopSoon(); });
    }
  }
private:
  int32_t expectIndex_;
  int32_t lastIndex_;
  bool& success_;
};

class Updater {
public:
  Updater(DuplexClientAsyncClient* client,
          TEventBase* eb,
          int32_t startIndex,
          int32_t numUpdates,
          int32_t interval)
    : client_(client)
    , eb_(eb)
    , startIndex_(startIndex)
    , numUpdates_(numUpdates)
    , interval_(interval)
  {}

  void update() {
    int32_t si = startIndex_;
    client_->update([si](ClientReceiveState&& state) {
      EXPECT_FALSE(state.isException());
      EXPECT_TRUE(state.isSecurityActive());
      try {
        int32_t res = DuplexClientAsyncClient::recv_update(state);
        EXPECT_EQ(res, si);
      } catch (const std::exception&) {
        EXPECT_TRUE(false);
      }
    }, startIndex_);
    startIndex_++;
    numUpdates_--;
    if (numUpdates_ > 0) {
      Updater updater(*this);
      eb_->runAfterDelay([updater]() mutable {
        updater.update();
      }, interval_);
    }
  }
private:
  DuplexClientAsyncClient* client_;
  TEventBase* eb_;
  int32_t startIndex_;
  int32_t numUpdates_;
  int32_t interval_;
};

class DuplexServiceInterface : public DuplexServiceSvIf {
  void async_tm_registerForUpdates(unique_ptr<HandlerCallback<bool>> callback,
                                   int32_t startIndex,
                                   int32_t numUpdates,
                                   int32_t interval) override {

    EXPECT_NE(callback->getConnectionContext()->
                getSaslServer()->getClientIdentity(), "");

    auto callbackp = callback.release();
    auto ctx = callbackp->getConnectionContext()->getConnectionContext();
    CHECK(ctx != nullptr);
    auto client = ctx->getDuplexClient<DuplexClientAsyncClient>();
    auto eb = callbackp->getEventBase();
    CHECK(eb != nullptr);
    if (numUpdates > 0) {
      Updater updater(client, eb, startIndex, numUpdates, interval);
      eb->runInEventBaseThread([updater]() mutable {updater.update();});
    };
    callbackp->resultInThread(true);
  }

  void async_tm_regularMethod(unique_ptr<HandlerCallback<int32_t>> callback,
                              int32_t val) override {
    EXPECT_NE(callback->getConnectionContext()->
                getSaslServer()->getClientIdentity(), "");
    callback.release()->resultInThread(val * 2);
  }
};

std::shared_ptr<ThriftServer> getDuplexServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(folly::make_unique<DuplexServiceInterface>());
  server->setDuplex(true);
  server->setSaslEnabled(true);
  return server;
}

ScopedServerThread duplexsst(getDuplexServer());

TEST(Security, Duplex) {
  enum {START=1, COUNT=3, INTERVAL=1};
  TEventBase base;

  auto port = duplexsst.getAddress()->getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto duplexChannel =
      std::make_shared<DuplexChannel>(DuplexChannel::Who::CLIENT, socket);
  enableSecurity(duplexChannel->getClientChannel().get());
  DuplexServiceAsyncClient client(duplexChannel->getClientChannel());

  bool success = false;
  ThriftServer clients_server(duplexChannel->getServerChannel());
  clients_server.setInterface(std::make_shared<DuplexClientInterface>(
      START, COUNT, success));
  clients_server.serve();

  client.registerForUpdates([](ClientReceiveState&& state) {
    EXPECT_FALSE(state.isException());
    EXPECT_TRUE(state.isSecurityActive());
    try {
      bool res = DuplexServiceAsyncClient::recv_registerForUpdates(state);
      EXPECT_TRUE(res);
    } catch (const std::exception&) {
      EXPECT_TRUE(false);
    }
  }, START, COUNT, INTERVAL);

  // fail on time out
  base.runAfterDelay([] {EXPECT_TRUE(false);}, 5000);

  base.loopForever();

  EXPECT_TRUE(success);
}

int main(int argc, char** argv) {
  setenv("KRB5_CONFIG", "/etc/krb5-thrift.conf", 0);
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
