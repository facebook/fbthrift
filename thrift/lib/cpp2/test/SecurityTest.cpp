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

HeaderClientChannel::Ptr getClientChannel(TEventBase* eb, uint16_t port) {
  char hostname[256];
  EXPECT_EQ(gethostname(hostname, 255), 0);
  std::string clientIdentity = std::string("host/") + hostname;
  std::string serviceIdentity = std::string("host@") + hostname;

  auto socket = TAsyncSocket::newSocket(eb, "127.0.0.1", port);
  auto channel = HeaderClientChannel::newChannel(socket);
  channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);

  auto saslClient = folly::make_unique<GssSaslClient>(eb);
  saslClient->setClientIdentity(clientIdentity);
  saslClient->setServiceIdentity(serviceIdentity);
  saslClient->setSaslThreadManager(make_shared<SaslThreadManager>(
      make_shared<SecurityLogger>()));
  saslClient->setCredentialsCacheManager(
      make_shared<krb5::Krb5CredentialsCacheManager>());
  channel->setSaslClient(std::move(saslClient));

  return std::move(channel);
}

ScopedServerThread sst(getServer());

void runTest(std::function<void(HeaderClientChannel* channel)> setup) {
  TEventBase base;

  auto port = sst.getAddress()->getPort();
  auto channel = getClientChannel(&base, port);
  setup(channel.get());
  TestServiceAsyncClient client(std::move(channel));

  client.sendResponse([&base](ClientReceiveState&& state) {
    EXPECT_FALSE(state.isException());
    EXPECT_TRUE(state.isSecurityActive());
    std::string res;
    TestServiceAsyncClient::recv_sendResponse(res, state);
    EXPECT_EQ(res, "10");
    base.terminateLoopSoon();
  }, 10);

  // fail on time out
  base.runAfterDelay([] {EXPECT_TRUE(false);}, 5000);

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

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
