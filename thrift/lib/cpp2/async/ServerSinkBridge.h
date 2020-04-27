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

#pragma once

#include <boost/variant.hpp>

#include <folly/Portability.h>

#if FOLLY_HAS_COROUTINES
#include <folly/Function.h>
#include <folly/Overload.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>

#include <thrift/lib/cpp2/async/SinkBridgeUtil.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>

namespace apache {
namespace thrift {
namespace detail {

struct SinkConsumerImpl {
  using Consumer = folly::Function<folly::coro::Task<folly::Try<StreamPayload>>(
      folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&>)>;
  Consumer consumer;
  uint64_t bufferSize;
  std::chrono::milliseconds chunkTimeout;
  folly::Executor::KeepAlive<> executor;

  explicit operator bool() const {
    return (bool)consumer;
  }
};

class ServerSinkBridge : public TwoWayBridge<
                             ServerSinkBridge,
                             ClientMessage,
                             CoroConsumer,
                             ServerMessage,
                             ServerSinkBridge>,
                         public SinkServerCallback {
 public:
  static Ptr create(
      SinkConsumerImpl&& sinkConsumer,
      folly::EventBase& evb,
      SinkClientCallback* callback) {
    return (new ServerSinkBridge(std::move(sinkConsumer), evb, callback))
        ->copy();
  }

  // SinkServerCallback method
  bool onSinkNext(StreamPayload&& payload) override {
    clientPush(folly::Try<StreamPayload>(std::move(payload)));
    return true;
  }

  void onSinkError(folly::exception_wrapper ew) override {
    folly::exception_wrapper hijacked;
    if (ew.with_exception([&hijacked](rocket::RocketException& rex) {
          hijacked = folly::exception_wrapper(
              apache::thrift::detail::EncodedError(rex.moveErrorData()));
        })) {
      clientPush(folly::Try<StreamPayload>(std::move(hijacked)));
    } else {
      clientPush(folly::Try<StreamPayload>(std::move(ew)));
    }
    close();
  }

  bool onSinkComplete() override {
    clientPush(SinkComplete{});
    sinkComplete_ = true;
    return true;
  }

  void onStreamCancel() override {
    clientPush(StreamCancel{});
    close();
  }

  // start should be called on threadmanager's thread
  folly::coro::Task<void> start() {
    serverPush(consumer_.bufferSize);
    folly::Try<StreamPayload> finalResponse =
        co_await consumer_.consumer(makeGenerator());

    if (clientException_) {
      co_return;
    }

    serverPush(std::move(finalResponse));
  }

  // TwoWayBridge consumer
  void consume() {
    DCHECK(evb_);
    evb_->runInEventBaseThread(
        [self = copy()]() { self->processClientMessages(); });
  }

  void canceled() {}

 private:
  ServerSinkBridge(
      SinkConsumerImpl&& sinkConsumer,
      folly::EventBase& evb,
      SinkClientCallback* callback)
      : consumer_(std::move(sinkConsumer)),
        evb_(folly::getKeepAliveToken(&evb)),
        clientCallback_(callback) {
    bool scheduledWait = clientWait(this);
    DCHECK(scheduledWait);
  }

  folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> makeGenerator() {
    uint64_t counter = 0;
    while (true) {
      CoroConsumer consumer;
      if (serverWait(&consumer)) {
        co_await consumer.wait();
      }
      for (auto messages = serverGetMessages(); !messages.empty();
           messages.pop()) {
        auto& message = messages.front();
        folly::Try<StreamPayload> ele;

        folly::variant_match(
            message,
            [&](folly::Try<StreamPayload>& payload) {
              ele = std::move(payload);
            },
            [&](StreamCancel&) {
              ele = folly::Try<StreamPayload>(
                  folly::make_exception_wrapper<transport::TTransportException>(
                      transport::TTransportException::TTransportExceptionType::
                          INTERRUPTED,
                      "sink cancelled"));
            },
            [](SinkComplete&) {});

        // empty Try represent the normal completion of the sink
        if (!ele.hasValue() && !ele.hasException()) {
          co_return;
        }

        if (ele.hasException()) {
          clientException_ = true;
          co_yield std::move(ele);
          co_return;
        }

        co_yield std::move(ele);
        counter++;
        if (counter > consumer_.bufferSize / 2) {
          serverPush(counter);
          counter = 0;
        }
      }
    }
  }

  void processClientMessages() {
    if (!clientCallback_) {
      return;
    }

    int64_t credits = 0;
    do {
      for (auto messages = clientGetMessages(); !messages.empty();
           messages.pop()) {
        bool terminated = false;
        auto& message = messages.front();
        folly::variant_match(
            message,
            [&](folly::Try<StreamPayload>& payload) {
              terminated = true;
              if (payload.hasValue()) {
                clientCallback_->onFinalResponse(std::move(payload).value());
              } else {
                clientCallback_->onFinalResponseError(
                    std::move(payload).exception());
              }
            },
            [&](int64_t n) { credits += n; });
        if (terminated) {
          close();
          return;
        }
      }
    } while (!clientWait(this));

    if (!sinkComplete_ && credits > 0) {
      std::ignore = clientCallback_->onSinkRequestN(credits);
    }
  }

  void close() {
    clientClose();
    evb_.reset();
    clientCallback_ = nullptr;
    Ptr(this);
  }

  SinkConsumerImpl consumer_;
  folly::Executor::KeepAlive<folly::EventBase> evb_;
  SinkClientCallback* clientCallback_;

  // only access in threadManager thread
  bool clientException_{false};

  // only access in evb_ thread
  bool sinkComplete_{false};
};

} // namespace detail
} // namespace thrift
} // namespace apache
#endif
