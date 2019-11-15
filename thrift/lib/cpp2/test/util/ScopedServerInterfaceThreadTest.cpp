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
#include <folly/stop_watch.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
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
 void async_tm_add(unique_ptr<HandlerCallback<int64_t>> cb,
                   int64_t a,
                   int64_t b) override {
    cb->result(a + b);
  }
};

TEST(ScopedServerInterfaceThread, nada) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());
}

TEST(ScopedServerInterfaceThread, example) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());

  EventBase eb;
  SimpleServiceAsyncClient cli(
    HeaderClientChannel::newChannel(
      TAsyncSocket::newSocket(
        &eb, ssit.getAddress())));

  EXPECT_EQ(6, cli.sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());

  EventBase eb;
  auto cli = ssit.newClient<SimpleServiceAsyncClient>(&eb);

  EXPECT_EQ(6, cli->sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient_ref) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());

  EventBase eb;
  auto cli = ssit.newClient<SimpleServiceAsyncClient>(eb);  // ref

  EXPECT_EQ(6, cli->sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient_SemiFuture) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  auto cli = ssit.newClient<SimpleServiceAsyncClient>();

  EXPECT_EQ(6, cli->semifuture_add(-3, 9).get());
}

TEST(ScopedServerInterfaceThread, getThriftServer) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());
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
  SimpleServiceAsyncClient cli(
    HeaderClientChannel::newChannel(
      TAsyncSocket::newSocket(
        &eb, ssit.getAddress())));

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
      return Channel::newChannel(std::move(socket));
    });
  }
};

class SlowSimpleServiceImpl : public virtual SimpleServiceSvIf {
 public:
  ~SlowSimpleServiceImpl() override {}
  folly::Future<int64_t> future_add(int64_t a, int64_t b) override {
    requestSem_.post();
    return folly::futures::sleepUnsafe(std::chrono::milliseconds(a + b))
        .thenValue([=](auto&&) { return a + b; });
  }

  void waitForRequest() {
    requestSem_.wait();
  }

 private:
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

  void waitForRequest() {
    requestSem_.wait();
  }

 private:
  folly::LifoSem requestSem_;
};

using TestTypes = ::testing::Types<
    ChannelAndService<HeaderClientChannel, SlowSimpleServiceImpl>,
    ChannelAndService<HeaderClientChannel, SlowSimpleServiceImplSemiFuture>>;
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
