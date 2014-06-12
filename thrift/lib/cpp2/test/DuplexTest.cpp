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
#include "thrift/lib/cpp2/async/RequestChannel.h"
#include "thrift/lib/cpp2/async/FutureRequest.h"
#include "thrift/lib/cpp2/test/gen-cpp2/DuplexService.h"
#include "thrift/lib/cpp2/test/gen-cpp2/DuplexClient.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/DuplexChannel.h"

#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"

#include "thrift/lib/cpp2/async/StubSaslClient.h"
#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <atomic>

#include "thrift/lib/cpp2/test/TestUtils.h"

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::async;
using namespace folly;
using std::shared_ptr;
using std::unique_ptr;

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
      int32_t res = DuplexClientAsyncClient::recv_update(state);
      EXPECT_EQ(res, si);
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
};

std::shared_ptr<ThriftServer> getServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(folly::make_unique<DuplexServiceInterface>());
  server->setDuplex(true);
  server->setSaslEnabled(true);
  server->setSaslServerFactory(
    [] (apache::thrift::async::TEventBase* evb) {
      return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
    }
  );
  return server;
}

TEST(Duplex, DuplexTest) {
  enum {START=1, COUNT=10, INTERVAL=5};
  TEventBase base;

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto duplexChannel =
      std::make_shared<DuplexChannel>(DuplexChannel::Who::CLIENT, socket);
  DuplexServiceAsyncClient client(duplexChannel->getClientChannel());

  bool success = false;
  ThriftServer clients_server(duplexChannel->getServerChannel());
  clients_server.setInterface(std::make_shared<DuplexClientInterface>(
      START, COUNT, success));
  clients_server.serve();

  client.registerForUpdates([](ClientReceiveState&& state) {
    EXPECT_FALSE(state.isException());
    bool res = DuplexServiceAsyncClient::recv_registerForUpdates(state);
    EXPECT_TRUE(res);
  }, START, COUNT, INTERVAL);

  // fail on time out
  base.runAfterDelay([] {EXPECT_TRUE(false);}, 5000);

  base.loopForever();

  EXPECT_TRUE(success);
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
