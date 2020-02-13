/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <atomic>

#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>
#include <folly/stop_watch.h>
#include <folly/test/TestUtils.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/util/gen-cpp2/SimpleService.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::util::cpp2;

class SimpleServiceImpl : public virtual SimpleServiceSvIf {
 public:
  ~SimpleServiceImpl() override {}
  void async_tm_add(
      unique_ptr<HandlerCallback<int64_t>> cb,
      int64_t a,
      int64_t b) override {
    cb->result(a + b);
  }
};

TEST(ScopedServerInterfaceThread, nada) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());
}

TEST(ScopedServerInterfaceThread, example) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  EventBase eb;
  SimpleServiceAsyncClient cli(HeaderClientChannel::newChannel(
      TAsyncSocket::newSocket(&eb, ssit.getAddress())));

  EXPECT_EQ(6, cli.sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  EventBase eb;
  auto cli = ssit.newClient<SimpleServiceAsyncClient>(&eb);

  EXPECT_EQ(6, cli->sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient_ref) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  EventBase eb;
  auto cli = ssit.newClient<SimpleServiceAsyncClient>(eb); // ref

  EXPECT_EQ(6, cli->sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient_SemiFuture) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  auto cli = ssit.newClient<SimpleServiceAsyncClient>();

  EXPECT_EQ(6, cli->semifuture_add(-3, 9).get());
}

TEST(ScopedServerInterfaceThread, getThriftServer) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());
  auto& ts = ssit.getThriftServer();
  EXPECT_EQ(0, ts.getNumCPUWorkerThreads());
  EXPECT_EQ(1, ts.getNumIOWorkerThreads());
}

TEST(ScopedServerInterfaceThread, ctor_with_thriftserver) {
  auto si = make_shared<SimpleServiceImpl>();
  auto ts = make_shared<ThriftServer>();
  ts->setInterface(si);
  ts->setAddress("::1", 0);
  ts->setNumIOWorkerThreads(1);
  ScopedServerInterfaceThread ssit(ts);
  EXPECT_EQ(uintptr_t(ts.get()), uintptr_t(&ssit.getThriftServer())); // sanity

  EventBase eb;
  SimpleServiceAsyncClient cli(HeaderClientChannel::newChannel(
      TAsyncSocket::newSocket(&eb, ssit.getAddress())));

  EXPECT_EQ(6, cli.sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, configureCbCalled) {
  std::atomic<bool> configCalled{false};
  ScopedServerInterfaceThread ssit(
      make_shared<SimpleServiceImpl>(), "::1", 0, [&](BaseThriftServer&) {
        configCalled = true;
      });
  EXPECT_TRUE(configCalled);
}

template <typename ChannelT, typename ServiceT>
struct ChannelAndService {
  using Channel = ChannelT;
  using Service = ServiceT;
};

template <typename ChannelAndServiceT>
struct ScopedServerInterfaceThreadTest : public testing::Test {
  using Channel = typename ChannelAndServiceT::Channel;
  using Service = typename ChannelAndServiceT::Service;

  std::shared_ptr<Service> newService() {
    return std::make_shared<Service>();
  }

  template <typename AsyncClientT>
  static std::unique_ptr<AsyncClientT> newClient(
      ScopedServerInterfaceThread& ssit) {
    return ssit.newClient<AsyncClientT>(nullptr, [](auto socket) {
      auto channel = Channel::newChannel(std::move(socket));
      channel->setTimeout(0);
      return channel;
    });
  }

  template <typename AsyncClientT>
  static std::unique_ptr<AsyncClientT> newRawClient(
      folly::EventBase* evb,
      ScopedServerInterfaceThread& ssit) {
    return std::make_unique<AsyncClientT>(
        folly::via(
            evb,
            [&] {
              auto channel = Channel::newChannel(async::TAsyncSocket::UniquePtr(
                  new async::TAsyncSocket(evb, ssit.getAddress())));
              channel->setTimeout(0);
              return channel;
            })
            .get());
  }

  static bool isHeaderTransport() {
    return std::is_same_v<HeaderClientChannel, Channel>;
  }
};

class SlowSimpleServiceImpl : public virtual SimpleServiceSvIf {
 public:
  ~SlowSimpleServiceImpl() override {}
  folly::Future<int64_t> future_add(int64_t a, int64_t b) override {
    requestSem_.post();

    if (a + b == 6666) {
      // A hack to avoid crashing when sleep future gets complete on
      // Timekeeper thread shutdown.
      return infiniteFuture().thenValue([res = a + b](auto&&) { return res; });
    }

    return folly::futures::sleepUnsafe(std::chrono::milliseconds(a + b))
        .thenValue([=](auto&&) { return a + b; });
  }

  folly::Future<std::unique_ptr<std::string>> future_echoSlow(
      std::unique_ptr<std::string> message,
      int64_t sleepMs) override {
    requestSem_.post();
    return folly::futures::sleepUnsafe(std::chrono::milliseconds(sleepMs))
        .thenValue([message = std::move(message)](auto&&) mutable {
          return std::move(message);
        });
  }

  void waitForRequest() {
    requestSem_.wait();
  }

 private:
  folly::Future<folly::Unit> infiniteFuture() {
    static folly::Indestructible<folly::SharedPromise<folly::Unit>> promise;
    return promise->getFuture();
  }

  folly::LifoSem requestSem_;
};

class SlowSimpleServiceImplSemiFuture : public virtual SimpleServiceSvIf {
 public:
  ~SlowSimpleServiceImplSemiFuture() override {}
  folly::SemiFuture<int64_t> semifuture_add(int64_t a, int64_t b) override {
    requestSem_.post();
    return folly::futures::sleep(std::chrono::milliseconds(a + b))
        .deferValue([=](auto&&) { return a + b; });
  }

  folly::SemiFuture<std::unique_ptr<std::string>> semifuture_echoSlow(
      std::unique_ptr<std::string> message,
      int64_t sleepMs) override {
    requestSem_.post();
    return folly::futures::sleep(std::chrono::milliseconds(sleepMs))
        .deferValue([message = std::move(message)](auto&&) mutable {
          return std::move(message);
        });
  }

  void waitForRequest() {
    requestSem_.wait();
  }

 private:
  folly::LifoSem requestSem_;
};

using TestTypes = ::testing::Types<
    ChannelAndService<HeaderClientChannel, SlowSimpleServiceImpl>,
    ChannelAndService<HeaderClientChannel, SlowSimpleServiceImplSemiFuture>,
    ChannelAndService<RocketClientChannel, SlowSimpleServiceImpl>,
    ChannelAndService<RocketClientChannel, SlowSimpleServiceImplSemiFuture>>;
TYPED_TEST_CASE(ScopedServerInterfaceThreadTest, TestTypes);

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequests) {
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl);

  auto cli = this->template newClient<SimpleServiceAsyncClient>(*ssit);

  folly::stop_watch<std::chrono::milliseconds> timer;

  auto future = cli->semifuture_add(6000, 0);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  ssit.reset();

  EXPECT_GE(timer.elapsed().count(), 6000);
  EXPECT_EQ(6000, std::move(future).get());
}

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsLargeMessage) {
  SKIP_IF(this->isHeaderTransport())
      << "Clean shutdown is not implemented for Header transport";

  std::string message(10000000, 'a');

  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl);

  auto cli = this->template newClient<SimpleServiceAsyncClient>(*ssit);

  folly::stop_watch<std::chrono::milliseconds> timer;

  auto future = cli->semifuture_echoSlow(message, 2000);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  ssit.reset();

  EXPECT_GE(timer.elapsed().count(), 2000);
  EXPECT_EQ(message, std::move(future).get(std::chrono::seconds(10)));
}

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsTimeout) {
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl, "::1", 0, [](auto& thriftServer) {
        thriftServer.setWorkersJoinTimeout(std::chrono::seconds{1});
      });

  auto cli = this->template newClient<SimpleServiceAsyncClient>(*ssit);

  auto future = cli->semifuture_add(6000, 666);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  ssit.reset();

  try {
    std::move(future).get();
    FAIL() << "Request didn't fail";
  } catch (const apache::thrift::transport::TTransportException& ex) {
    EXPECT_EQ(
        apache::thrift::transport::TTransportException::END_OF_FILE,
        ex.getType())
        << "Unexpected exception: " << folly::exceptionStr(ex);
  }
}

TYPED_TEST(ScopedServerInterfaceThreadTest, writeError) {
  auto serviceImpl = this->newService();

  ScopedServerInterfaceThread ssit(serviceImpl);

  folly::ScopedEventBaseThread evbThread;

  auto cli = this->template newRawClient<SimpleServiceAsyncClient>(
      evbThread.getEventBase(), ssit);
  SCOPE_EXIT {
    folly::via(evbThread.getEventBase(), [cli = std::move(cli)] {});
  };

  auto future = cli->semifuture_add(2000, 0);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  folly::via(evbThread.getEventBase(), [&] {
    dynamic_cast<ClientChannel*>(cli->getChannel())
        ->getTransport()
        ->shutdownWrite();
  });

  cli->semifuture_add(2000, 0);

  try {
    std::move(future).get();
    FAIL() << "Request didn't fail";
  } catch (const apache::thrift::transport::TTransportException& ex) {
    EXPECT_NE(
        apache::thrift::transport::TTransportException::NOT_OPEN, ex.getType())
        << "Unexpected exception: " << folly::exceptionStr(ex);
  }
}

TYPED_TEST(ScopedServerInterfaceThreadTest, closeConnection) {
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl, "::1", 0, [](auto& thriftServer) {
        thriftServer.setWorkersJoinTimeout(std::chrono::seconds{1});
      });

  folly::ScopedEventBaseThread evbThread;

  auto cli = this->template newRawClient<SimpleServiceAsyncClient>(
      evbThread.getEventBase(), *ssit);
  SCOPE_EXIT {
    folly::via(evbThread.getEventBase(), [cli = std::move(cli)] {});
  };

  auto future = cli->semifuture_add(6000, 666);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  folly::via(
      evbThread.getEventBase(),
      [&] {
        dynamic_cast<ClientChannel*>(cli->getChannel())
            ->getTransport()
            ->closeNow();
      })
      .get();

  try {
    std::move(future).get();
    FAIL() << "Request didn't fail";
  } catch (const apache::thrift::transport::TTransportException& ex) {
    EXPECT_EQ(
        apache::thrift::transport::TTransportException::END_OF_FILE,
        ex.getType())
        << "Unexpected exception: " << folly::exceptionStr(ex);
  }

  ssit.reset();
}

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsCancel) {
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl);

  auto cli = this->template newClient<SimpleServiceAsyncClient>(*ssit);

  folly::stop_watch<std::chrono::milliseconds> timer;

  std::atomic<bool> stopping{false};
  std::thread schedulerThread([&] {
    ScopedEventBaseThread eb;
    while (!stopping) {
      cli->semifuture_add(2000, 0)
          .via(eb.getEventBase())
          .thenTry([](folly::Try<int64_t> t) {
            if (t.hasException()) {
              LOG(INFO) << folly::exceptionStr(t.exception());
            } else {
              LOG(INFO) << *t;
            }
          });
      this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  });

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  ssit.reset();

  EXPECT_GE(timer.elapsed().count(), 2000);

  EXPECT_LE(timer.elapsed().count(), 20000);

  stopping = true;
  schedulerThread.join();
}
