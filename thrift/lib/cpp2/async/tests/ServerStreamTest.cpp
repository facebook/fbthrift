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

#include <thrift/lib/cpp2/async/ServerStream.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Sleep.h>
#endif // FOLLY_HAS_COROUTINES

namespace apache {
namespace thrift {

using namespace ::testing;

class ClientCallback : public StreamClientCallback {
 public:
  bool onFirstResponse(
      FirstResponsePayload&&,
      folly::EventBase*,
      StreamServerCallback* c) override {
    cb = c;
    started.post();
    return true;
  }
  void onFirstResponseError(folly::exception_wrapper) override {
    std::terminate();
  }

  bool onStreamNext(StreamPayload&& payload) override {
    if (i < 1024) {
      EXPECT_EQ(payload.payload->capacity(), i++);
    } else {
      ++i;
    }
    return true;
  }
  void onStreamError(folly::exception_wrapper) override {
    std::terminate();
  }
  void onStreamComplete() override {
    completed.post();
  }

  void resetServerCallback(StreamServerCallback&) override {
    std::terminate();
  }

  int i = 0;
  folly::fibers::Baton started, completed;
  StreamServerCallback* cb;
};
class MyFirstResponseCallback
    : public detail::ClientStreamBridge::FirstResponseCallback {
 public:
  void onFirstResponse(
      FirstResponsePayload&&,
      detail::ClientStreamBridge::ClientPtr ptr) override {
    clientStreamBridge = std::move(ptr);
    baton.post();
  }
  void onFirstResponseError(folly::exception_wrapper) override {
    std::terminate();
  }
  detail::ClientStreamBridge::ClientPtr clientStreamBridge;
  folly::fibers::Baton baton;
};

auto encode(folly::Try<int>&& i) -> folly::Try<StreamPayload> {
  if (i.hasValue()) {
    return folly::Try<StreamPayload>(
        StreamPayload{folly::IOBuf::create(*i), {}});
  } else if (i.hasException()) {
    return folly::Try<StreamPayload>(i.exception());
  } else {
    return folly::Try<StreamPayload>();
  }
}
auto decode(folly::Try<StreamPayload>&& i) -> folly::Try<int> {
  if (i.hasValue()) {
    return folly::Try<int>(i->payload->capacity());
  } else if (i.hasException()) {
    return folly::Try<int>(i.exception());
  } else {
    return folly::Try<int>();
  }
}

#if FOLLY_HAS_COROUTINES
TEST(ServerStreamTest, PublishConsumeCoro) {
  folly::ScopedEventBaseThread clientEb, serverEb;
  ClientCallback clientCallback;
  ServerStream<int> factory([]() -> folly::coro::AsyncGenerator<int&&> {
    for (int i = 0; i < 10; ++i) {
      co_yield std::move(i);
    }
  }());
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientEb.add([&] {
    clientCallback.started.wait();
    std::ignore = clientCallback.cb->onStreamRequestN(11); // complete costs 1
  });
  clientCallback.completed.wait();
  EXPECT_EQ(clientCallback.i, 10);
}

TEST(ServerStreamTest, ImmediateCancel) {
  class CancellingClientCallback : public ClientCallback {
    bool onFirstResponse(
        FirstResponsePayload&&,
        folly::EventBase*,
        StreamServerCallback* c) override {
      c->onStreamCancel();
      completed.post();
      return false;
    }
  };
  folly::ScopedEventBaseThread clientEb, serverEb;
  CancellingClientCallback clientCallback;
  ServerStream<int> factory([]() -> folly::coro::AsyncGenerator<int&&> {
    for (int i = 0; i < 10; ++i) {
      co_await folly::coro::sleep(std::chrono::milliseconds(10));
      co_yield std::move(i);
    }
    EXPECT_TRUE(false);
  }());
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientCallback.completed.wait();
}

TEST(ServerStreamTest, DelayedCancel) {
  class CancellingClientCallback : public ClientCallback {
    bool onStreamNext(StreamPayload&&) override {
      if (i++ == 3) {
        cb->onStreamCancel();
        completed.post();
        return false;
      }
      return true;
    }
  };
  folly::ScopedEventBaseThread clientEb, serverEb;
  CancellingClientCallback clientCallback;
  ServerStream<int> factory([]() -> folly::coro::AsyncGenerator<int&&> {
    for (int i = 0; i < 10; ++i) {
      co_await folly::coro::sleep(std::chrono::milliseconds(10));
      co_yield std::move(i);
    }
    EXPECT_TRUE(false);
  }());
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientEb.add([&] {
    clientCallback.started.wait();
    std::ignore = clientCallback.cb->onStreamRequestN(11); // complete costs 1
  });
  clientCallback.completed.wait();
  EXPECT_EQ(clientCallback.i, 4);
}

TEST(ServerStreamTest, PropagatedCancel) {
  folly::ScopedEventBaseThread clientEb, serverEb;
  ClientCallback clientCallback;
  folly::Baton<> setup, canceled;
  ServerStream<int> factory([&]() -> folly::coro::AsyncGenerator<int&&> {
    folly::CancellationCallback cb{
        co_await folly::coro::co_current_cancellation_token,
        [&] { canceled.post(); }};
    setup.post();
    co_await folly::coro::sleep(std::chrono::minutes(1));
  }());
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientCallback.started.wait();
  clientEb.getEventBase()->add(
      [&] { std::ignore = clientCallback.cb->onStreamRequestN(1); });
  setup.wait();
  clientEb.getEventBase()->add([&] { clientCallback.cb->onStreamCancel(); });
  ASSERT_TRUE(canceled.try_wait_for(std::chrono::seconds(1)));
}

TEST(ServerStreamTest, CancelCoro) {
  ClientCallback clientCallback;
  {
    folly::coro::Baton baton;
    folly::ScopedEventBaseThread clientEb, serverEb;
    ServerStream<int> factory([&]() -> folly::coro::AsyncGenerator<int&&> {
      baton.post();
      for (int i = 0;; ++i) {
        EXPECT_LT(i, 10);
        co_await folly::coro::sleep(std::chrono::milliseconds(10));
        co_yield std::move(i);
      }
    }());
    factory(serverEb.getEventBase(), &encode)(
        FirstResponsePayload{nullptr, {}},
        &clientCallback,
        clientEb.getEventBase());
    clientEb.add([&] {
      clientCallback.started.wait();
      std::ignore = clientCallback.cb->onStreamRequestN(11); // complete costs 1
    });
    folly::coro::blockingWait(baton);
    clientEb.add([&] { clientCallback.cb->onStreamCancel(); });
  }
  EXPECT_LT(clientCallback.i, 10);
}
#endif // FOLLY_HAS_COROUTINES

TEST(ServerStreamTest, MustClosePublisher) {
  EXPECT_DEATH(
      ([] {
        ClientCallback clientCallback;
        folly::ScopedEventBaseThread clientEb, serverEb;
        auto [factory, publisher] = ServerStream<int>::createPublisher();
        factory(serverEb.getEventBase(), &encode)(
            FirstResponsePayload{nullptr, {}},
            &clientCallback,
            clientEb.getEventBase());
        clientEb.add([&] {
          clientCallback.started.wait();
          std::ignore = clientCallback.cb->onStreamRequestN(5);
        });
        publisher.next(0);
      })(),
      "StreamPublisher has to be completed or canceled");
}

TEST(ServerStreamTest, PublishConsumePublisher) {
  ClientCallback clientCallback;
  folly::ScopedEventBaseThread clientEb, serverEb;
  bool closed = false;
  auto [factory, publisher] =
      ServerStream<int>::createPublisher([&] { closed = true; });
  for (int i = 0; i < 5; i++) {
    publisher.next(i);
  }
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientEb.add([&] {
    clientCallback.started.wait();
    std::ignore = clientCallback.cb->onStreamRequestN(11); // complete costs 1
  });
  for (int i = 5; i < 10; i++) {
    publisher.next(i);
  }
  std::move(publisher).complete();
  clientCallback.completed.wait();
  EXPECT_EQ(clientCallback.i, 10);
  EXPECT_TRUE(closed);
}

TEST(ServerStreamTest, CancelPublisher) {
  ClientCallback clientCallback;
  folly::ScopedEventBaseThread clientEb, serverEb;
  bool closed = false;
  auto [factory, publisher] =
      ServerStream<int>::createPublisher([&] { closed = true; });
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      &clientCallback,
      clientEb.getEventBase());
  clientEb.add([&] {
    clientCallback.started.wait();
    std::ignore = clientCallback.cb->onStreamRequestN(11); // complete costs 1
  });
  std::thread([&, publisher = std::move(publisher)]() mutable {
    for (int i = 0; i < 10; i++) {
      if (i == 1) {
        clientEb.add([&] { clientCallback.cb->onStreamCancel(); });
      }
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      publisher.next(i);
    }
    std::move(publisher).complete();
  })
      .join();
  EXPECT_LT(clientCallback.i, 10);
  EXPECT_TRUE(closed);
}

TEST(ServerStreamTest, ClientBufferedStreamGeneratorIntegration) {
  folly::ScopedEventBaseThread clientEb, serverEb;
  MyFirstResponseCallback firstResponseCallback;
  auto clientStreamBridge =
      detail::ClientStreamBridge::create(&firstResponseCallback);

  ServerStream<int> factory([&]() -> folly::coro::AsyncGenerator<int&&> {
    for (int i = 0; i < 10; ++i) {
      co_yield std::move(i);
    }
  }());
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      clientStreamBridge,
      clientEb.getEventBase());

  size_t expected = 0;
  bool done = false;
  firstResponseCallback.baton.wait();
  ClientBufferedStream<int> clientStream(
      std::move(firstResponseCallback.clientStreamBridge), &decode, 0);
  std::move(clientStream).subscribeInline([&](folly::Try<int>&& next) {
    if (next.hasValue()) {
      EXPECT_EQ(expected++, *next);
    } else {
      done = true;
    }
  });
  EXPECT_EQ(10, expected);
  EXPECT_TRUE(done);
}

TEST(ServerStreamTest, ClientBufferedStreamPublisherIntegration) {
  folly::ScopedEventBaseThread clientEb, serverEb;
  MyFirstResponseCallback firstResponseCallback;
  auto clientStreamBridge =
      detail::ClientStreamBridge::create(&firstResponseCallback);

  auto [factory, publisher] = ServerStream<int>::createPublisher();
  for (int i = 0; i < 5; i++) {
    publisher.next(i);
  }
  factory(serverEb.getEventBase(), &encode)(
      FirstResponsePayload{nullptr, {}},
      clientStreamBridge,
      clientEb.getEventBase());

  for (int i = 5; i < 10; i++) {
    publisher.next(i);
  }
  std::move(publisher).complete();

  size_t expected = 0;
  bool done = false;
  firstResponseCallback.baton.wait();
  ClientBufferedStream<int> clientStream(
      std::move(firstResponseCallback.clientStreamBridge), &decode, 0);
  std::move(clientStream).subscribeInline([&](folly::Try<int>&& next) {
    if (next.hasValue()) {
      EXPECT_EQ(expected++, *next);
    } else {
      done = true;
    }
  });
  EXPECT_EQ(10, expected);
  EXPECT_TRUE(done);
}

TEST(ServerStreamTest, FactoryLeak) {
  auto [stream, publisher] = ServerStream<int>::createPublisher([] {});
  std::move(publisher).complete();
  stream = [&]() -> folly::coro::AsyncGenerator<int&&> {
    for (int i = 0; i < 10; ++i) {
      co_yield std::move(i);
    }
  }
  ();
  stream = apache::thrift::Stream<int>();
}

} // namespace thrift
} // namespace apache
