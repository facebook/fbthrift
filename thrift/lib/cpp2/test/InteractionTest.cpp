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
#include <folly/experimental/coro/Task.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Calculator.h>
#include <thrift/lib/cpp2/test/gen-cpp2/HandlerGeneric.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace ::testing;
using namespace apache::thrift;
using namespace apache::thrift::test;

namespace {
class DummyChannel : public RequestChannel {
 public:
  void sendRequestResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override {
    std::terminate();
  }
  void sendRequestNoResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override {
    std::terminate();
  }
  void setCloseCallback(CloseCallback*) override {
    std::terminate();
  }
  folly::EventBase* getEventBase() const override {
    std::terminate();
  }
  uint16_t getProtocolId() override {
    std::terminate();
  }

  void terminateInteraction(InteractionId id) override {
    releaseInteractionId(std::move(id));
  }
  void seedInteractionId(int64_t id) {
    nextId_ = id;
  }
  InteractionId createInteraction(folly::StringPiece) override {
    return createInteractionId(nextId_);
  }

 private:
  int64_t nextId_{0};
};
} // namespace

struct Handler : HandlerGenericSvIf {
  void get_string(std::string& out) override {
    if (auto interaction = getConnectionContext()->getInteractionId()) {
      out = std::to_string(interaction);
      if (auto create = getConnectionContext()->getInteractionCreate()) {
        out += *create->interactionName_ref();
      }
    } else {
      out = "";
    }
  }
};

TEST(InteractionTest, NoIDPropagated) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  RpcOptions rpcOpts;
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "");
}

TEST(InteractionTest, Register) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  folly::via(
      &eb, [&] { client.getChannel()->registerInteraction("Transaction", 42); })
      .getVia(&eb);

  RpcOptions rpcOpts;
  DummyChannel dummy;
  dummy.seedInteractionId(42);
  auto id = dummy.createInteraction("");
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "42Transaction");

  eb.runInEventBaseThread([channel = client.getChannelShared(),
                           ka = folly::getKeepAliveToken(eb),
                           id = std::move(id)]() mutable {
    channel->terminateInteraction(std::move(id));
  });
}

TEST(InteractionTest, Create) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  auto id = folly::via(
                &eb,
                [channel = client.getChannel()] {
                  return channel->createInteraction("Transaction");
                })
                .getVia(&eb);
  RpcOptions rpcOpts;
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "1Transaction");

  DummyChannel().terminateInteraction(std::move(id));
}

TEST(InteractionTest, TerminateUsed) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  folly::via(
      &eb,
      [&, channel = client.getChannel()] {
        channel->registerInteraction("Transaction", 42);
      })
      .getVia(&eb);

  RpcOptions rpcOpts;
  DummyChannel dummy;
  dummy.seedInteractionId(42);
  auto id = dummy.createInteraction("");
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);

  folly::getKeepAliveToken(eb).add([channel = client.getChannelShared(),
                                    id = std::move(id)](auto&&) mutable {
    channel->terminateInteraction(std::move(id));
  });
}

TEST(InteractionTest, TerminateUnused) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  std::string out;
  client.sync_get_string(out); // sends setup frame

  DummyChannel dummy;
  dummy.seedInteractionId(42);

  folly::via(
      &eb,
      [&,
       channel = client.getChannel(),
       id = dummy.createInteraction("")]() mutable {
        channel->registerInteraction("Transaction", 42);
        channel->terminateInteraction(std::move(id));
      })
      .getVia(&eb);
}

TEST(InteractionTest, TerminateWithoutSetup) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  DummyChannel dummy;
  dummy.seedInteractionId(42);

  folly::via(
      &eb,
      [&,
       channel = client.getChannel(),
       id = dummy.createInteraction("")]() mutable {
        channel->registerInteraction("Transaction", 42);
        channel->terminateInteraction(std::move(id));
      })
      .getVia(&eb);
}

TEST(InteractionTest, TerminatePRC) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  auto client =
      runner.newClient<HandlerGenericAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

  RpcOptions rpcOpts;

  auto id = client->getChannel()->createInteraction("Transaction");
  rpcOpts.setInteractionId(id);
  std::string out;
  client->sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, std::to_string(id) + "Transaction");

  client->getChannel()->terminateInteraction(std::move(id));
}

TEST(InteractionTest, IsDetachable) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));
  auto channel = static_cast<ClientChannel*>(client.getChannel());

  bool detached = false;
  channel->setOnDetachable([&] { detached = true; });
  EXPECT_TRUE(channel->isDetachable());

  auto id = channel->createInteraction("Transaction");
  EXPECT_FALSE(channel->isDetachable());

  RpcOptions rpcOpts;
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_FALSE(channel->isDetachable());

  channel->terminateInteraction(std::move(id));
  client.sync_get_string(out); // drive the EB
  EXPECT_TRUE(channel->isDetachable());
  EXPECT_TRUE(detached);
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
    folly::coro::Task<void> co_noop() override {
      co_return;
    }
    folly::coro::Task<void> co_accumulatePoint(
        std::unique_ptr<::apache::thrift::test::Point> a) override {
      *pacc_.x_ref() += *a->x_ref();
      *pacc_.y_ref() += *a->y_ref();
      co_return;
    }
    folly::coro::Task<int32_t> co_getPrimitive() override {
      co_return acc_;
    }
    folly::coro::Task<std::unique_ptr<::apache::thrift::test::Point>>
    co_getPoint() override {
      co_return folly::copy_to_unique_ptr(pacc_);
    }
#endif
  };

  std::unique_ptr<AdditionIf> createAddition() override {
    return std::make_unique<AdditionHandler>();
  }

  folly::SemiFuture<int32_t> semifuture_addPrimitive(int32_t a, int32_t b)
      override {
    return a + b;
  }
};

TEST(InteractionCodegenTest, Basic) {
  ScopedServerInterfaceThread runner{std::make_shared<CalculatorHandler>()};
  auto client =
      runner.newClient<CalculatorAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

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

TEST(InteractionCodegenTest, BasicSemiFuture) {
  struct SemiCalculatorHandler : CalculatorSvIf {
    struct SemiAdditionHandler : CalculatorSvIf::AdditionIf {
      int acc_{0};
      Point pacc_;

      folly::SemiFuture<folly::Unit> semifuture_accumulatePrimitive(
          int32_t a) override {
        acc_ += a;
        return folly::makeSemiFuture();
      }
      folly::SemiFuture<folly::Unit> semifuture_noop() override {
        return folly::makeSemiFuture();
      }
      folly::SemiFuture<folly::Unit> semifuture_accumulatePoint(
          std::unique_ptr<::apache::thrift::test::Point> a) override {
        *pacc_.x_ref() += *a->x_ref();
        *pacc_.y_ref() += *a->y_ref();
        return folly::makeSemiFuture();
      }
      folly::SemiFuture<int32_t> semifuture_getPrimitive() override {
        return acc_;
      }
      folly::SemiFuture<std::unique_ptr<::apache::thrift::test::Point>>
      semifuture_getPoint() override {
        return folly::copy_to_unique_ptr(pacc_);
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<SemiAdditionHandler>();
    }

    folly::SemiFuture<int32_t> semifuture_addPrimitive(int32_t a, int32_t b)
        override {
      return a + b;
    }
  };
  ScopedServerInterfaceThread runner{std::make_shared<CalculatorHandler>()};
  auto client =
      runner.newClient<CalculatorAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

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

TEST(InteractionCodegenTest, Error) {
  struct BrokenCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      throw std::runtime_error("Plus key is broken");
    }
  };
  ScopedServerInterfaceThread runner{
      std::make_shared<BrokenCalculatorHandler>()};
  auto client =
      runner.newClient<CalculatorAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

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

TEST(InteractionCodegenTest, SlowConstructor) {
  struct SlowCalculatorHandler : CalculatorHandler {
    std::unique_ptr<AdditionIf> createAddition() override {
      b.wait();
      return std::make_unique<AdditionHandler>();
    }

    folly::Baton<> b;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client =
      runner.newClient<CalculatorAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

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
          folly::coro::Baton& baton,
          folly::coro::Baton& destroyed)
          : baton_(baton), destroyed_(destroyed) {}
      ~SlowAdditionHandler() override {
        destroyed_.post();
      }

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

    folly::coro::Baton baton, destroyed;
  };
  auto handler = std::make_shared<SlowCalculatorHandler>();
  ScopedServerInterfaceThread runner{handler};
  auto client =
      runner.newClient<CalculatorAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

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
          folly::coro::Baton& baton,
          folly::coro::Baton& destroyed)
          : baton_(baton), destroyed_(destroyed) {}
      ~SlowAdditionHandler() override {
        destroyed_.post();
      }

      folly::coro::Task<void> co_noop() override {
        co_await baton_;
        co_return;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      return std::make_unique<SlowAdditionHandler>(baton, destroyed);
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
      ~SlowAdditionHandler() override {
        destroyed_.post();
      }

      folly::coro::Task<void> co_noop() override {
        executed_ = true;
        co_return;
      }
    };

    std::unique_ptr<AdditionIf> createAddition() override {
      folly::coro::blockingWait(baton);
      return std::make_unique<SlowAdditionHandler>(baton, destroyed, executed);
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
  EXPECT_FALSE(handler->executed);
#endif
}
