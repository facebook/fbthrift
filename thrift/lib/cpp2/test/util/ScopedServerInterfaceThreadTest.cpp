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

#include <folly/executors/GlobalExecutor.h>

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>
#include <folly/stop_watch.h>
#include <folly/test/TestUtils.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/util/gen-cpp2/SimpleService.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
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

  apache::thrift::SinkConsumer<int64_t, bool> slowReturnSink(int64_t sleepMs) {
    return apache::thrift::SinkConsumer<int64_t, bool>{
        [&, sleepMs](folly::coro::AsyncGenerator<int64_t&&> gen)
            -> folly::coro::Task<bool> {
          while (auto item = co_await gen.next()) {
          }
          // sink complete
          requestSem_.post();
          co_await folly::coro::sleep(std::chrono::milliseconds(sleepMs));
          co_return true;
        },
        10};
  }

  void waitForSinkComplete() {
    requestSem_.wait();
  }

 private:
  folly::LifoSem requestSem_;
};

TEST(ScopedServerInterfaceThread, nada) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());
}

TEST(ScopedServerInterfaceThread, example) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  EventBase eb;
  SimpleServiceAsyncClient cli(HeaderClientChannel::newChannel(
      folly::AsyncSocket::newSocket(&eb, ssit.getAddress())));

  EXPECT_EQ(6, cli.sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  EventBase eb;
  auto cli = ssit.newClient<SimpleServiceAsyncClient>(&eb);

  EXPECT_EQ(6, cli->sync_add(-3, 9));
}

TEST(ScopedServerInterfaceThread, newClient_SemiFuture) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());

  auto cli = ssit.newClient<SimpleServiceAsyncClient>();

  EXPECT_EQ(6, cli->semifuture_add(-3, 9).get());
}

TEST(ScopedServerInterfaceThread, newRemoteClient) {
  struct Handler : SimpleServiceSvIf {
    struct State {
      size_t requests = 0;
    };
    std::atomic<size_t> conns{0};
    void async_tm_add(
        unique_ptr<HandlerCallback<int64_t>> cb,
        int64_t a,
        int64_t b) override {
      auto r = cb->getConnectionContext();
      auto eb = cb->getEventBase();
      eb->runInEventBaseThread([cb = std::move(cb), r, a, b] {
        auto c = r->getConnectionContext();
        auto s = static_cast<State*>(c->getUserData());
        if (s == nullptr) {
          s = new State();
          c->setUserData(s, [](void* _) { delete static_cast<State*>(_); });
        }
        cb->result(++s->requests + a + b);
      });
    }
  };

  ScopedServerInterfaceThread ssit(make_shared<Handler>());

  auto cli = ssit.newStickyClient<SimpleServiceAsyncClient>();

  EXPECT_EQ(7, cli->semifuture_add(-3, 9).get());
  EXPECT_EQ(8, cli->semifuture_add(-3, 9).get());
  EXPECT_EQ(9, cli->semifuture_add(-3, 9).get());
}

TEST(ScopedServerInterfaceThread, getThriftServer) {
  ScopedServerInterfaceThread ssit(make_shared<SimpleServiceImpl>());
  auto& ts = ssit.getThriftServer();
  EXPECT_EQ(1, ts.getNumCPUWorkerThreads());
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
      folly::AsyncSocket::newSocket(&eb, ssit.getAddress())));

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

TEST(ScopedServerInterfaceThread, joinRequestsSinkSlowFinalResponse) {
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto serviceImpl = std::make_shared<SimpleServiceImpl>();
    folly::Optional<ScopedServerInterfaceThread> ssit(
        folly::in_place, serviceImpl);

    auto cli =
        ssit->newClient<SimpleServiceAsyncClient>(nullptr, [](auto socket) {
          auto channel = RocketClientChannel::newChannel(std::move(socket));
          channel->setTimeout(0);
          return channel;
        });

    auto sink = co_await cli->co_slowReturnSink(6000);
    // should not throw
    bool result =
        co_await sink.sink([&](auto) -> folly::coro::AsyncGenerator<int64_t&&> {
          co_yield 1;
          co_yield 2;
        }(folly::makeGuard([&]() {
                             serviceImpl->waitForSinkComplete();
                             serviceImpl.reset();
                             ssit.reset();
                           })));
    EXPECT_TRUE(result);
  }());
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
              auto channel = Channel::newChannel(folly::AsyncSocket::UniquePtr(
                  new folly::AsyncSocket(evb, ssit.getAddress())));
              channel->setTimeout(0);
              return channel;
            })
            .get());
  }

  static bool isHeaderTransport() {
    return std::is_same_v<HeaderClientChannel, Channel>;
  }

  void SetUp() {
    // By default, ThriftServer aborts the process if unable to shutdown
    // on deadline. Since client and server are running in the same process,
    // this also would crash the tests.
    FLAGS_thrift_abort_if_exceeds_shutdown_deadline = false;
  }

 private:
  gflags::FlagSaver flagSaver;
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

    return folly::futures::sleep(std::chrono::milliseconds(a + b))
        .via(folly::getGlobalCPUExecutor())
        .thenValue([=](auto&&) { return a + b; });
  }

  folly::Future<std::unique_ptr<std::string>> future_echoSlow(
      std::unique_ptr<std::string> message,
      int64_t sleepMs) override {
    requestSem_.post();
    return folly::futures::sleep(std::chrono::milliseconds(sleepMs))
        .via(folly::getGlobalCPUExecutor())
        .thenValue([message = std::move(message)](auto&&) mutable {
          return std::move(message);
        });
  }

  folly::Future<apache::thrift::ServerStream<int64_t>> future_emptyStreamSlow(
      int64_t sleepMs) {
    requestSem_.post();
    return folly::futures::sleep(std::chrono::milliseconds(sleepMs))
        .via(folly::getGlobalCPUExecutor())
        .thenValue([](auto&&) {
          return apache::thrift::ServerStream<int64_t>::createEmpty();
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

  folly::SemiFuture<apache::thrift::ServerStream<int64_t>>
  semifuture_emptyStreamSlow(int64_t sleepMs) {
    requestSem_.post();
    return folly::futures::sleep(std::chrono::milliseconds(sleepMs))
        .deferValue([](auto&&) {
          return apache::thrift::ServerStream<int64_t>::createEmpty();
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

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsStreamTaskTimeout) {
  SKIP_IF(this->isHeaderTransport())
      << "Streaming is not implemented for Header transport";

  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl);

  auto cli = this->template newClient<SimpleServiceAsyncClient>(*ssit);

  folly::stop_watch<std::chrono::milliseconds> timer;

  apache::thrift::RpcOptions options;
  options.setTimeout(std::chrono::seconds{1});
  auto future = cli->semifuture_emptyStreamSlow(options, 6000);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  ssit.reset();

  EXPECT_GE(timer.elapsed().count(), 6000);
  EXPECT_ANY_THROW(std::move(future).get());
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

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsStress) {
  SKIP_IF(this->isHeaderTransport())
      << "Clean shutdown is not implemented for Header transport";

  std::string message(10000000, 'a');

  bool stopping{false};
  folly::Function<void()> spamServer;
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl);

  folly::Optional<folly::ScopedEventBaseThread> evbThread(folly::in_place);
  auto evb = evbThread->getEventBase();
  auto cli = this->template newRawClient<SimpleServiceAsyncClient>(evb, *ssit);

  auto future = cli->semifuture_echoSlow(message, 2000);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  constexpr size_t kRequestsPerLoop = 20;
  constexpr size_t kMaxInflightSpamRequests = 1000;

  size_t inflightSpamRequests = 0;

  // Make sure that there're enough in-flight writes so that we see a write
  // error before seeing an EOF.
  spamServer = [&] {
    evb->add([&] {
      if (stopping) {
        return;
      }
      for (size_t i = 0; i < kRequestsPerLoop; ++i) {
        if (inflightSpamRequests >= kMaxInflightSpamRequests) {
          break;
        }
        apache::thrift::RpcOptions rpcOptions;
        ++inflightSpamRequests;
        cli->header_future_add(rpcOptions, 2000, 0)
            .thenTry([&inflightSpamRequests,
                      ka = folly::getKeepAliveToken(evb)](auto&& t) {
              --inflightSpamRequests;
              if (t.hasValue()) {
                auto& header = *t->second;
                const auto& readHeaders = header.getHeaders();
                if (auto exHeader = folly::get_ptr(readHeaders, "ex")) {
                  if (*exHeader != kOverloadedErrorCode &&
                      *exHeader != kQueueOverloadedErrorCode) {
                    FAIL() << "Non-retriable server error: " << *exHeader;
                  }
                }
                EXPECT_EQ(2000, t->first);
                return;
              }
              DCHECK(t.hasException());
              if (!t.exception()
                       .template with_exception<
                           apache::thrift::transport::
                               TTransportException>([](auto&& ex) {
                         if (ex.getType() !=
                             apache::thrift::transport::TTransportException::
                                 NOT_OPEN) {
                           FAIL()
                               << "Non-retriable TTransportException exception: "
                               << ex.what()
                               << ". Exception type: " << ex.getType();
                         }
                       }) &&
                  !t.exception()
                       .template with_exception<
                           apache::thrift::TApplicationException>([](auto&&
                                                                         ex) {
                         if (ex.getType() !=
                             apache::thrift::TApplicationException::
                                 LOADSHEDDING) {
                           FAIL()
                               << "Non-retriable TApplicationException exception: "
                               << ex.what()
                               << ". Exception type: " << ex.getType();
                         }
                       })) {
                FAIL() << "Unexpected exception: "
                       << folly::exceptionStr(t.exception());
              }
            });
      }
      spamServer();
    });
  };
  spamServer();

  ssit.reset();

  EXPECT_EQ(message, std::move(future).get());

  evb->add([&] {
    stopping = true;
    cli.reset();
  });
  evbThread.reset();
}

TYPED_TEST(ScopedServerInterfaceThreadTest, joinRequestsDetachedConnection) {
  auto serviceImpl = this->newService();

  folly::Optional<ScopedServerInterfaceThread> ssit(
      folly::in_place, serviceImpl, "::1");

  folly::ScopedEventBaseThread evbThread;

  auto cli = this->template newRawClient<SimpleServiceAsyncClient>(
      evbThread.getEventBase(), *ssit);
  SCOPE_EXIT {
    folly::via(evbThread.getEventBase(), [cli = std::move(cli)] {});
  };

  folly::stop_watch<std::chrono::milliseconds> timer;

  auto future = cli->semifuture_add(2000, 0);

  serviceImpl->waitForRequest();
  serviceImpl.reset();

  folly::Baton<> blockBaton;

  folly::via(evbThread.getEventBase(), [&] { blockBaton.wait(); });

  ssit.reset();

  EXPECT_GE(timer.elapsed().count(), 2000);
  EXPECT_LE(timer.elapsed().count(), 10000);

  EXPECT_FALSE(future.isReady());

  blockBaton.post();

  EXPECT_EQ(2000, std::move(future).get());
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

TYPED_TEST(ScopedServerInterfaceThreadTest, SetMaxRequestsJoinWrites) {
  SKIP_IF(this->isHeaderTransport())
      << "Joining writes is not implemented for Header transport";

  std::string message(10000000, 'a');

  auto serviceImpl = this->newService();

  ScopedServerInterfaceThread ssit(
      serviceImpl, "::1", 0, [](auto& thriftServer) {
        thriftServer.setMaxRequests(1);
      });

  folly::ScopedEventBaseThread evbThread1, evbThread2;
  auto cli1 = this->template newRawClient<SimpleServiceAsyncClient>(
      evbThread1.getEventBase(), ssit);
  auto cli2 = this->template newRawClient<SimpleServiceAsyncClient>(
      evbThread2.getEventBase(), ssit);
  SCOPE_EXIT {
    folly::via(evbThread1.getEventBase(), [cli1 = std::move(cli1)] {});
    folly::via(evbThread2.getEventBase(), [cli2 = std::move(cli2)] {});
  };

  auto future = cli1->semifuture_echoSlow(message, 1000);

  serviceImpl->waitForRequest();

  folly::stop_watch<std::chrono::milliseconds> timer;

  // Block the receiving thread so that write can't complete on the server side.
  evbThread1.add([] { std::this_thread::sleep_for(std::chrono::seconds{5}); });

  while (true) {
    try {
      EXPECT_EQ(43, cli2->semifuture_add(42, 1).get());
      EXPECT_GE(timer.elapsed(), std::chrono::seconds{5});
      break;
    } catch (...) {
    }
  }

  std::move(future).get();
}
