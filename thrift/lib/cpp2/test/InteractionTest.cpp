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

#include <memory>

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/experimental/coro/Task.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Calculator.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Streamer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace ::testing;
using namespace apache::thrift;
using namespace apache::thrift::test;

struct SemiCalculatorHandler : CalculatorSvIf {
  struct SemiAdditionHandler : CalculatorSvIf::AdditionIf {
    int acc_{0};
    Point pacc_;

    void accumulatePrimitive(int32_t a) override { acc_ += a; }
    folly::SemiFuture<folly::Unit> semifuture_noop() override {
      return folly::makeSemiFuture();
    }
    folly::SemiFuture<folly::Unit> semifuture_accumulatePoint(
        std::unique_ptr<::apache::thrift::test::Point> a) override {
      *pacc_.x_ref() += *a->x_ref();
      *pacc_.y_ref() += *a->y_ref();
      return folly::makeSemiFuture();
    }
    int32_t getPrimitive() override { return acc_; }
    folly::SemiFuture<std::unique_ptr<::apache::thrift::test::Point>>
    semifuture_getPoint() override {
      return folly::copy_to_unique_ptr(pacc_);
    }
  };

  std::unique_ptr<AdditionIf> createAddition() override {
    return std::make_unique<SemiAdditionHandler>();
  }
  std::unique_ptr<AdditionFastIf> createAdditionFast() override {
    std::terminate();
  }
  std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
    std::terminate();
  }

  folly::SemiFuture<int32_t> semifuture_addPrimitive(
      int32_t a, int32_t b) override {
    return a + b;
  }
};

TEST(InteractionTest, TerminateUsed) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  auto adder = client.createAddition();
  adder.semifuture_getPrimitive().via(&eb).getVia(&eb);
}

TEST(InteractionTest, TerminateActive) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  auto adder = client.createAddition();
  adder.semifuture_noop().via(&eb).getVia(&eb);
}

TEST(InteractionTest, TerminateUnused) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  client.sync_addPrimitive(0, 0); // sends setup frame
  auto adder = client.createAddition();
}

TEST(InteractionTest, TerminateWithoutSetup) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  auto adder = client.createAddition();
}

TEST(InteractionTest, TerminateUsedPRC) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  adder.semifuture_noop().get();
}

TEST(InteractionTest, TerminateUnusedPRC) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  client->sync_addPrimitive(0, 0); // sends setup frame
  auto adder = client->createAddition();
}

TEST(InteractionTest, TerminateWithoutSetupPRC) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
}

TEST(InteractionTest, IsDetachable) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));
  auto channel = static_cast<ClientChannel*>(client.getChannel());

  bool detached = false;
  channel->setOnDetachable([&] { detached = true; });
  EXPECT_TRUE(channel->isDetachable());

  {
    auto adder = client.createAddition();
    EXPECT_FALSE(channel->isDetachable());

    adder.semifuture_noop().via(&eb).getVia(&eb);
    EXPECT_FALSE(channel->isDetachable());
  }

  client.sync_addPrimitive(0, 0); // drive the EB to send termination
  EXPECT_TRUE(channel->isDetachable());
  EXPECT_TRUE(detached);
}

TEST(InteractionTest, QueueTimeout) {
  struct SlowCalculatorHandler : CalculatorSvIf {
    struct SemiAdditionHandler : CalculatorSvIf::AdditionIf {
      folly::SemiFuture<int32_t> semifuture_getPrimitive() override {
        /* sleep override: testing timeout */
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 0;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<SemiAdditionHandler>();
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }
  };

  ScopedServerInterfaceThread runner{std::make_shared<SlowCalculatorHandler>()};
  runner.getThriftServer().setQueueTimeout(std::chrono::milliseconds(50));
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  auto f1 = adder.semifuture_getPrimitive(),
       f2 = adder.semifuture_getPrimitive();
  EXPECT_FALSE(std::move(f1).getTry().hasException());
  EXPECT_TRUE(std::move(f2).getTry().hasException());
}

struct CalculatorHandler : CalculatorSvIf {
  struct AdditionHandler : CalculatorSvIf::AdditionIf {
    int acc_{0};
    Point pacc_;

#if FOLLY_HAS_COROUTINES
    folly::coro::Task<void> co_accumulatePrimitive(int32_t a) override {
      acc_ += a;
      co_return;
    }
    folly::coro::Task<void> co_noop() override { co_return; }
    folly::coro::Task<void> co_accumulatePoint(
        std::unique_ptr<::apache::thrift::test::Point> a) override {
      *pacc_.x_ref() += *a->x_ref();
      *pacc_.y_ref() += *a->y_ref();
      co_return;
    }
    folly::coro::Task<int32_t> co_getPrimitive() override { co_return acc_; }
    folly::coro::Task<std::unique_ptr<::apache::thrift::test::Point>>
    co_getPoint() override {
      co_return folly::copy_to_unique_ptr(pacc_);
    }
#endif
  };

  std::unique_ptr<AdditionIf> createAddition() override {
    return std::make_unique<AdditionHandler>();
  }
  std::unique_ptr<AdditionFastIf> createAdditionFast() override {
    std::terminate();
  }
  std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
    std::terminate();
  }

  folly::SemiFuture<int32_t> semifuture_addPrimitive(
      int32_t a, int32_t b) override {
    return a + b;
  }
};

TEST(InteractionCodegenTest, Basic) {
  ScopedServerInterfaceThread runner{std::make_shared<CalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    co_await adder.co_accumulatePrimitive(1);
    co_await adder.semifuture_accumulatePrimitive(2);
    co_await adder.co_noop();
    auto acc = co_await adder.co_getPrimitive();
    EXPECT_EQ(acc, 3);

    auto sum = co_await client->co_addPrimitive(20, 22);
    EXPECT_EQ(sum, 42);

    Point p;
    p.x_ref() = 1;
    co_await adder.co_accumulatePoint(p);
    p.y_ref() = 2;
    co_await adder.co_accumulatePoint(p);
    auto pacc = co_await adder.co_getPoint();
    EXPECT_EQ(*pacc.x_ref(), 2);
    EXPECT_EQ(*pacc.y_ref(), 2);
  }());
#endif
}

TEST(InteractionCodegenTest, RpcOptions) {
  ScopedServerInterfaceThread runner{std::make_shared<CalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  RpcOptions rpcOptions;
  auto adder = client->createAddition();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    co_await adder.co_accumulatePrimitive(rpcOptions, 1);
    co_await adder.semifuture_accumulatePrimitive(rpcOptions, 2);
    co_await adder.co_noop(rpcOptions);
    auto acc = co_await adder.co_getPrimitive(rpcOptions);
    EXPECT_EQ(acc, 3);

    auto sum = co_await client->co_addPrimitive(rpcOptions, 20, 22);
    EXPECT_EQ(sum, 42);

    Point p;
    p.x_ref() = 1;
    co_await adder.co_accumulatePoint(rpcOptions, p);
    p.y_ref() = 2;
    co_await adder.co_accumulatePoint(rpcOptions, p);
    auto pacc = co_await adder.co_getPoint(rpcOptions);
    EXPECT_EQ(*pacc.x_ref(), 2);
    EXPECT_EQ(*pacc.y_ref(), 2);
  }());
#endif
}

TEST(InteractionCodegenTest, BasicSemiFuture) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  adder.semifuture_accumulatePrimitive(1).get();
  adder.semifuture_accumulatePrimitive(2).get();
  adder.semifuture_noop().get();
  auto acc = adder.semifuture_getPrimitive().get();
  EXPECT_EQ(acc, 3);

  auto sum = client->semifuture_addPrimitive(20, 22).get();
  EXPECT_EQ(sum, 42);

  Point p;
  p.x_ref() = 1;
  adder.semifuture_accumulatePoint(p).get();
  p.y_ref() = 2;
  adder.semifuture_accumulatePoint(p).get();
  auto pacc = adder.semifuture_getPoint().get();
  EXPECT_EQ(*pacc.x_ref(), 2);
  EXPECT_EQ(*pacc.y_ref(), 2);
}

TEST(InteractionCodegenTest, BasicSync) {
  ScopedServerInterfaceThread runner{std::make_shared<SemiCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  adder.sync_accumulatePrimitive(1);
  adder.sync_accumulatePrimitive(2);
  adder.sync_noop();
  auto acc = adder.sync_getPrimitive();
  EXPECT_EQ(acc, 3);

  auto sum = client->sync_addPrimitive(20, 22);
  EXPECT_EQ(sum, 42);

  Point p;
  p.x_ref() = 1;
  adder.sync_accumulatePoint(p);
  p.y_ref() = 2;
  adder.sync_accumulatePoint(p);
  Point pacc;
  adder.sync_getPoint(pacc);
  EXPECT_EQ(*pacc.x_ref(), 2);
  EXPECT_EQ(*pacc.y_ref(), 2);
}

TEST(InteractionCodegenTest, Error) {
  struct BrokenCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      throw std::runtime_error("Plus key is broken");
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }
  };
  ScopedServerInterfaceThread runner{
      std::make_shared<BrokenCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  const char* kExpectedErr =
      "apache::thrift::TApplicationException:"
      " Interaction constructor failed with std::runtime_error: Plus key is broken";

  auto adder = client->createAddition();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto t = co_await folly::coro::co_awaitTry(adder.co_accumulatePrimitive(1));
    EXPECT_STREQ(t.exception().what().c_str(), kExpectedErr);
    auto t2 = co_await folly::coro::co_awaitTry(adder.co_getPrimitive());
    EXPECT_STREQ(t.exception().what().c_str(), kExpectedErr);
    co_await adder.co_noop();

    auto sum = co_await client->co_addPrimitive(20, 22);
    EXPECT_EQ(sum, 42);
  }());
#endif
}

TEST(InteractionCodegenTest, MethodException) {
  struct ExceptionCalculatorHandler : CalculatorSvIf {
    struct AdditionHandler : CalculatorSvIf::AdditionIf {
      int acc_{0};
#if FOLLY_HAS_COROUTINES
      folly::coro::Task<void> co_accumulatePrimitive(int32_t a) override {
        acc_ += a;
        co_yield folly::coro::co_error(
            std::runtime_error("Not Implemented Yet"));
      }
#endif
    };
    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<AdditionHandler>();
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }
  };

  ScopedServerInterfaceThread runner{
      std::make_shared<ExceptionCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  const char* kExpectedErr =
      "apache::thrift::TApplicationException: std::runtime_error: Not Implemented Yet";

  auto adder = client->createAddition();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto t = co_await folly::coro::co_awaitTry(adder.co_accumulatePrimitive(1));
    EXPECT_STREQ(t.exception().what().c_str(), kExpectedErr);
  }());
#endif
}

TEST(InteractionCodegenTest, SlowConstructor) {
  struct SlowCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      b.wait();
      return std::make_unique<AdditionHandler>();
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::Baton<> b;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  folly::EventBase eb;
#if FOLLY_HAS_COROUTINES
  // only release constructor once interaction methods are queued
  adder.co_accumulatePrimitive(1).scheduleOn(&eb).start();
  adder.co_noop().scheduleOn(&eb).start();
  folly::via(&eb, [&] { handler->b.post(); }).getVia(&eb);
  auto acc = folly::coro::blockingWait(adder.co_getPrimitive());
  EXPECT_EQ(acc, 1);
#endif
}

TEST(InteractionCodegenTest, FastTermination) {
#if FOLLY_HAS_COROUTINES
  struct SlowCalculatorHandler : CalculatorHandler {
    struct SlowAdditionHandler : AdditionHandler {
      folly::coro::Baton &baton_, &destroyed_;
      SlowAdditionHandler(
          folly::coro::Baton& baton, folly::coro::Baton& destroyed)
          : baton_(baton), destroyed_(destroyed) {}
      ~SlowAdditionHandler() override { destroyed_.post(); }

      folly::coro::Task<int32_t> co_getPrimitive() override {
        co_await baton_;
        co_return acc_;
      }
      folly::coro::Task<void> co_noop() override {
        co_await baton_;
        co_return;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<SlowAdditionHandler>(baton, destroyed);
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::coro::Baton baton, destroyed;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = folly::copy_to_unique_ptr(client->createAddition());
  folly::EventBase eb;
  auto semi = adder->co_getPrimitive().scheduleOn(&eb).start();
  adder->co_accumulatePrimitive(1).semi().via(&eb).getVia(&eb);
  adder->co_noop().semi().via(&eb).getVia(&eb);
  adder.reset(); // sends termination while methods in progress
  EXPECT_FALSE(handler->destroyed.ready());
  handler->baton.post();
  EXPECT_EQ(std::move(semi).via(&eb).getVia(&eb), 1);
  folly::coro::blockingWait(handler->destroyed);
#endif
}

TEST(InteractionCodegenTest, ClientCrashDuringInteraction) {
#if FOLLY_HAS_COROUTINES
  struct SlowCalculatorHandler : CalculatorHandler {
    struct SlowAdditionHandler : AdditionHandler {
      folly::coro::Baton &baton_, &destroyed_;
      SlowAdditionHandler(
          folly::coro::Baton& baton, folly::coro::Baton& destroyed)
          : baton_(baton), destroyed_(destroyed) {}
      ~SlowAdditionHandler() override { destroyed_.post(); }

      folly::coro::Task<void> co_noop() override {
        co_await baton_;
        co_return;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<SlowAdditionHandler>(baton, destroyed);
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::coro::Baton baton, destroyed;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  folly::EventBase eb;
  folly::AsyncSocket* sock = new folly::AsyncSocket(&eb, runner.getAddress());
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(sock)));

  auto adder = client.createAddition();
  auto fut = adder.co_noop().semi().via(&eb);
  adder.co_getPrimitive().semi().via(&eb).getVia(&eb);
  sock->closeNow();
  handler->baton.post();
  fut.waitVia(&eb);
  folly::coro::blockingWait(handler->destroyed);
#endif
}

TEST(InteractionCodegenTest, ClientCrashDuringInteractionConstructor) {
#if FOLLY_HAS_COROUTINES
  struct SlowCalculatorHandler : CalculatorHandler {
    struct SlowAdditionHandler : AdditionHandler {
      folly::coro::Baton &baton_, &destroyed_;
      bool& executed_;
      SlowAdditionHandler(
          folly::coro::Baton& baton,
          folly::coro::Baton& destroyed,
          bool& executed)
          : baton_(baton), destroyed_(destroyed), executed_(executed) {}
      ~SlowAdditionHandler() override { destroyed_.post(); }

      folly::coro::Task<void> co_noop() override {
        executed_ = true;
        co_return;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      folly::coro::blockingWait(baton);
      return std::make_unique<SlowAdditionHandler>(baton, destroyed, executed);
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::coro::Baton baton, destroyed;
    bool executed = false;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  runner.getThriftServer().getThreadManager()->addWorker();
  folly::EventBase eb;
  folly::AsyncSocket* sock = new folly::AsyncSocket(&eb, runner.getAddress());
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(sock)));

  auto adder = client.createAddition();
  auto fut = adder.co_noop().semi().via(&eb);
  client.co_addPrimitive(0, 0).semi().via(&eb).getVia(&eb);
  sock->closeNow();
  handler->baton.post();
  fut.waitVia(&eb);
  folly::coro::blockingWait(handler->destroyed);
  EXPECT_TRUE(handler->executed);
#endif
}

TEST(InteractionCodegenTest, ReuseIdDuringConstructor) {
  struct SlowCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      if (first) {
        first = false;
        b1.post();
        b2.wait();
      }
      return std::make_unique<AdditionHandler>();
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::Baton<> b1, b2;
    bool first = true;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  folly::EventBase eb;
  CalculatorAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  {
    auto id = client.getChannel()->registerInteraction("Addition", 1);
    CalculatorAsyncClient::Addition adder(
        client.getChannelShared(), std::move(id));
    adder.semifuture_noop().via(&eb).getVia(&eb);
    handler->b1.wait();
  } // sends termination while constructor is blocked
  eb.loopOnce();

  auto id = client.getChannel()->registerInteraction("Addition", 1);
  CalculatorAsyncClient::Addition adder(
      client.getChannelShared(), std::move(id));

  auto fut = adder.semifuture_accumulatePrimitive(1);
  handler->b2.post();
  std::move(fut).via(&eb).getVia(&eb);
}

TEST(InteractionCodegenTest, ConstructorExceptionPropagated) {
  struct SlowCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      b.wait();
      throw std::runtime_error("Custom exception");
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }

    folly::Baton<> b;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAddition();
  // only release constructor once interaction methods are queued
  auto fut1 = adder.semifuture_accumulatePrimitive(1);
  auto fut2 = adder.semifuture_accumulatePrimitive(1);
  handler->b.post();
  auto fut3 = adder.semifuture_accumulatePrimitive(1);

  EXPECT_THAT(
      std::move(fut1).getTry().exception().what().toStdString(),
      HasSubstr("Custom exception"));
  EXPECT_THAT(
      std::move(fut2).getTry().exception().what().toStdString(),
      HasSubstr("Custom exception"));
  EXPECT_TRUE(std::move(fut3).getTry().hasException());
}

TEST(InteractionCodegenTest, SerialInteraction) {
#if FOLLY_HAS_COROUTINES
  struct SerialCalculatorHandler : CalculatorHandler {
    struct SerialAdditionHandler : CalculatorSvIf::SerialAdditionIf {
      int acc_{0};
      folly::coro::Baton &baton2_, &baton3_;
      SerialAdditionHandler(
          folly::coro::Baton& baton2, folly::coro::Baton& baton3)
          : baton2_(baton2), baton3_(baton3) {}

      folly::coro::Task<void> co_accumulatePrimitive(int a) override {
        co_await baton2_;
        acc_ += a;
      }
      folly::coro::Task<int32_t> co_getPrimitive() override {
        co_await baton3_;
        co_return acc_;
      }
      folly::coro::Task<apache::thrift::ServerStream<int32_t>>
      co_waitForCancel() override {
        co_return []() -> folly::coro::AsyncGenerator<int32_t&&> {
          folly::coro::Baton b;
          folly::CancellationCallback cb{
              co_await folly::coro::co_current_cancellation_token,
              [&] { b.post(); }};
          co_await b;
        }();
      }
    };

    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      folly::coro::blockingWait(baton1);
      return std::make_unique<SerialAdditionHandler>(baton2, baton3);
    }
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      std::terminate();
    }
    std::unique_ptr<AdditionIf> createAddition() override { std::terminate(); }

    folly::coro::Baton baton1, baton2, baton3;
  };
  auto handler = std::make_shared<SerialCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createSerialAddition();
  folly::EventBase eb;
  // keep a stream alive for the rest of the test (0th serial method)
  auto stream = adder.semifuture_waitForCancel();
  auto accSemi = adder.co_accumulatePrimitive(1).scheduleOn(&eb).start();
  auto getSemi = adder.co_getPrimitive().scheduleOn(&eb).start();

  // blocked on baton1 in constructor, blocks TM worker
  auto semi = client->semifuture_addPrimitive(0, 0);
  EXPECT_FALSE(accSemi.isReady());
  EXPECT_FALSE(getSemi.isReady());
  handler->baton1.post();
  std::move(semi).via(&eb).getVia(&eb);
  // blocked on baton2 in first serial method, sole TM worker is free
  client->co_addPrimitive(0, 0).semi().via(&eb).getVia(&eb);
  EXPECT_FALSE(accSemi.isReady());
  EXPECT_FALSE(getSemi.isReady());

  // blocked on baton3 in second method, first method completes
  handler->baton2.post();
  client->co_addPrimitive(0, 0).semi().via(&eb).getVia(&eb);
  std::move(accSemi).via(&eb).getVia(&eb);
  EXPECT_FALSE(getSemi.isReady());

  // third method is blocked on second method completing
  accSemi = adder.co_accumulatePrimitive(1).scheduleOn(&eb).start();
  client->co_addPrimitive(0, 0).semi().via(&eb).getVia(&eb);
  EXPECT_FALSE(accSemi.isReady());
  EXPECT_FALSE(getSemi.isReady());

  // both methods complete
  handler->baton3.post();
  client->co_addPrimitive(0, 0).semi().via(&eb).getVia(&eb);
  std::move(accSemi).via(&eb).getVia(&eb);

  // second accumulate happens after get
  EXPECT_EQ(std::move(getSemi).via(&eb).getVia(&eb), 1);
#endif
}

TEST(InteractionCodegenTest, StreamExtendsInteractionLifetime) {
#if FOLLY_HAS_COROUTINES
  struct StreamingHandler : StreamerSvIf {
    StreamingHandler()
        : publisherPair(ServerStream<int>::createPublisher([&] {
            streamBaton.post();
            EXPECT_FALSE(
                tileBaton2.try_wait_for(std::chrono::milliseconds(100)));
          })) {}
    struct StreamTile : StreamerSvIf::StreamingIf {
      folly::coro::Task<ServerStream<int>> co_generatorStream() override {
        co_return folly::coro::co_invoke(
            [&]() -> folly::coro::AsyncGenerator<int&&> {
              SCOPE_EXIT {
                streamBaton.post();
                EXPECT_FALSE(
                    tileBaton2.try_wait_for(std::chrono::milliseconds(100)));
              };
              while (true) {
                co_yield 0;
              }
            });
      }

      folly::coro::Task<ServerStream<int>> co_publisherStream() override {
        co_return std::move(publisherStreamRef);
      }

      folly::coro::Task<apache::thrift::SinkConsumer<int32_t, int8_t>>
      co__sink() override {
        SinkConsumer<int32_t, int8_t> sink;
        sink.consumer = [&](auto gen) -> folly::coro::Task<int8_t> {
          SCOPE_EXIT {
            streamBaton.post();
            EXPECT_FALSE(
                tileBaton2.try_wait_for(std::chrono::milliseconds(100)));
          };
          co_await gen.next();
          co_return 0;
        };
        sink.bufferSize = 5;
        sink.setChunkTimeout(std::chrono::milliseconds(500));
        co_return sink;
      }

      StreamTile(
          folly::Baton<>& tileBaton_,
          folly::Baton<>& tileBaton2_,
          folly::Baton<>& streamBaton_,
          ServerStream<int>& publisherStream)
          : tileBaton(tileBaton_),
            tileBaton2(tileBaton2_),
            streamBaton(streamBaton_),
            publisherStreamRef(publisherStream) {}

      ~StreamTile() {
        tileBaton.post();
        tileBaton2.post();
        EXPECT_TRUE(streamBaton.try_wait_for(std::chrono::milliseconds(100)));
      }

      folly::Baton<>&tileBaton, &tileBaton2, &streamBaton;
      ServerStream<int>& publisherStreamRef;
    };

    std::unique_ptr<StreamingIf> createStreaming() override {
      return std::make_unique<StreamTile>(
          tileBaton, tileBaton2, streamBaton, publisherPair.first);
    }

    folly::Baton<> tileBaton, tileBaton2, streamBaton;
    std::pair<ServerStream<int>, ServerStreamPublisher<int>> publisherPair;
  };

  auto handler = std::make_shared<StreamingHandler>();
  ScopedServerInterfaceThread runner{handler};
  runner.getThriftServer().getThreadManager()->addWorker();
  auto client = runner.newClient<StreamerAsyncClient>(nullptr, [](auto socket) {
    return RocketClientChannel::newChannel(std::move(socket));
  });

  // Generator test
  {
    auto handle = folly::copy_to_unique_ptr(client->createStreaming());
    auto stream = handle->semifuture_generatorStream().get();
    // both stream and interaction handle are alive
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
    handle.reset();
    // stream keeps interaction alive after handle destroyed
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
  }
  // both stream and handle destroyed
  EXPECT_TRUE(handler->tileBaton.try_wait_for(std::chrono::milliseconds(300)));
  handler->tileBaton.reset();
  handler->tileBaton2.reset();
  handler->streamBaton.reset();

  // Publisher test
  {
    auto handle = folly::copy_to_unique_ptr(client->createStreaming());
    auto stream = handle->semifuture_publisherStream().get();
    // both stream and interaction handle are alive
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
    handle.reset();
    // stream keeps interaction alive after handle destroyed
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
  }
  // both stream and handle destroyed
  std::move(handler->publisherPair.second).complete();
  EXPECT_TRUE(handler->tileBaton.try_wait_for(std::chrono::milliseconds(300)));
  handler->tileBaton.reset();
  handler->tileBaton2.reset();
  handler->streamBaton.reset();

  // Sink test
  {
    auto handle = folly::copy_to_unique_ptr(client->createStreaming());
    auto sink = handle->co__sink().semi().get();
    // both sink and interaction handle are alive
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
    handle.reset();
    // sink keeps interaction alive after handle destroyed
    EXPECT_FALSE(
        handler->tileBaton.try_wait_for(std::chrono::milliseconds(100)));
    handler->tileBaton.reset();
  }
  // both sink and handle destroyed
  EXPECT_TRUE(handler->tileBaton.try_wait_for(std::chrono::milliseconds(300)));
  handler->tileBaton.reset();
  handler->tileBaton2.reset();
  handler->streamBaton.reset();

#endif
}

TEST(InteractionCodegenTest, ShutdownDuringStreamTeardown) {
#if FOLLY_HAS_COROUTINES
  struct StreamingHandler : StreamerSvIf {
    struct StreamTile : StreamerSvIf::StreamingIf {
      folly::coro::Task<ServerStream<int>> co_generatorStream() override {
        co_return folly::coro::co_invoke(
            [&]() -> folly::coro::AsyncGenerator<int&&> {
              while (true) {
                co_yield 0;
                co_await folly::coro::sleep(std::chrono::milliseconds(100));
              }
            });
      }
    };

    std::unique_ptr<StreamingIf> createStreaming() override {
      return std::make_unique<StreamTile>();
    }
  };

  auto handler = std::make_shared<StreamingHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client = runner.newClient<StreamerAsyncClient>(nullptr, [](auto socket) {
    return RocketClientChannel::newChannel(std::move(socket));
  });

  folly::coro::blockingWait(client->createStreaming().co_generatorStream());
#endif
}

TEST(InteractionCodegenTest, BasicEB) {
  struct ExceptionCalculatorHandler : CalculatorSvIf {
    struct AdditionHandler : CalculatorSvIf::AdditionFastIf {
      int acc_{0};
      void async_eb_accumulatePrimitive(
          std::unique_ptr<HandlerCallback<void>> cb, int32_t a) override {
        acc_ += a;
        cb->exception(std::runtime_error("Not Implemented Yet"));
      }
      void async_eb_getPrimitive(
          std::unique_ptr<HandlerCallback<int32_t>> cb) override {
        cb->result(acc_);
      }
    };
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      return std::make_unique<AdditionHandler>();
    }
    std::unique_ptr<AdditionIf> createAddition() override { std::terminate(); }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }
  };

  ScopedServerInterfaceThread runner{
      std::make_shared<ExceptionCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAdditionFast();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto [r1, r2] = co_await folly::coro::collectAllTry(
        adder.co_accumulatePrimitive(1), adder.co_getPrimitive());
    EXPECT_TRUE(r1.hasException());
    EXPECT_EQ(*r2, 1);
  }());
#endif
}

TEST(InteractionCodegenTest, ErrorEB) {
  struct ExceptionCalculatorHandler : CalculatorSvIf {
    std::unique_ptr<AdditionFastIf> createAdditionFast() override {
      throw std::runtime_error("Unimplemented");
    }
    std::unique_ptr<AdditionIf> createAddition() override { std::terminate(); }
    std::unique_ptr<SerialAdditionIf> createSerialAddition() override {
      std::terminate();
    }
  };

  ScopedServerInterfaceThread runner{
      std::make_shared<ExceptionCalculatorHandler>()};
  auto client = runner.newClient<CalculatorAsyncClient>(
      nullptr, RocketClientChannel::newChannel);

  auto adder = client->createAdditionFast();
#if FOLLY_HAS_COROUTINES
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto [r1, r2] = co_await folly::coro::collectAllTry(
        adder.co_accumulatePrimitive(1), adder.co_getPrimitive());
    EXPECT_TRUE(r1.hasException());
    EXPECT_TRUE(r2.hasException());
  }());
#endif
}
