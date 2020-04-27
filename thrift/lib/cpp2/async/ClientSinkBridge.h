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

#include <folly/Overload.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>
#endif

#include <thrift/lib/cpp2/async/SinkBridgeUtil.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>

namespace apache {
namespace thrift {
namespace detail {

#if FOLLY_HAS_COROUTINES
class ClientSinkBridge : public TwoWayBridge<
                             CoroConsumer,
                             ClientMessage,
                             ClientSinkBridge,
                             ServerMessage,
                             ClientSinkBridge>,
                         public SinkClientCallback {
 public:
  folly::coro::Task<folly::Try<FirstResponsePayload>> getFirstThriftResponse() {
    co_await firstResponseBaton_;
    co_return std::move(firstResponse_);
  }

  static Ptr create() {
    return (new ClientSinkBridge())->copy();
  }

  void close() {
    serverClose();
    serverCallback_ = nullptr;
    evb_.reset();
    Ptr(this);
  }

  folly::coro::Task<folly::Try<StreamPayload>> sink(
      folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> generator) {
    int64_t credit = 0;
    folly::Try<StreamPayload> finalResponse;

    auto waitEvent = [&]() -> folly::coro::Task<void> {
      CoroConsumer consumer;
      if (clientWait(&consumer)) {
        co_await consumer.wait();
      }

      auto queue = clientGetMessages();
      while (!queue.empty()) {
        auto& message = queue.front();
        folly::variant_match(
            message,
            [&](folly::Try<StreamPayload>& payload) {
              finalResponse = std::move(payload);
            },
            [&](int64_t n) { credit += n; });
        queue.pop();
        if (finalResponse.hasValue() || finalResponse.hasException()) {
          co_return;
        }
      }
    };

    bool sinkComplete = false;

    while (true) {
      co_await waitEvent();
      DCHECK(
          finalResponse.hasValue() || finalResponse.hasException() ||
          credit > 0);
      if (finalResponse.hasValue() || finalResponse.hasException()) {
        break;
      }

      while (credit > 0 && !sinkComplete) {
        auto item = co_await generator.next();

        if (item.has_value()) {
          if ((*item).hasValue()) {
            clientPush(std::move(*item));
          } else {
            clientPush(std::move(*item));
            // AsyncGenerator who serialized and yield the exception also in
            // charge of propagating it back to user, just return empty Try
            // here.
            co_return folly::Try<StreamPayload>();
          }
        } else {
          clientPush(SinkComplete{});
          sinkComplete = true;
          // release generator
          generator = {};
        }
        credit--;
      }
    }
    co_return std::move(finalResponse);
  }

  void cancel(std::unique_ptr<folly::IOBuf> ex) {
    clientPush(folly::Try<StreamPayload>(rocket::RocketException(
        rocket::ErrorCode::APPLICATION_ERROR, std::move(ex))));
  }

  // SinkClientCallback method
  bool onFirstResponse(
      FirstResponsePayload&& firstPayload,
      folly::EventBase* evb,
      SinkServerCallback* serverCallback) override {
    serverCallback_ = serverCallback;
    evb_ = folly::getKeepAliveToken(evb);
    bool scheduledWait = serverWait(this);
    DCHECK(scheduledWait);
    firstResponse_.emplace(std::move(firstPayload));
    firstResponseBaton_.post();
    return true;
  }

  void onFirstResponseError(folly::exception_wrapper ew) override {
    firstResponse_.emplaceException(std::move(ew));
    firstResponseBaton_.post();
    close();
  }

  void onFinalResponse(StreamPayload&& payload) override {
    serverPush(folly::Try<StreamPayload>(std::move(payload)));
    close();
  }

  void onFinalResponseError(folly::exception_wrapper ew) override {
    folly::exception_wrapper hijacked;
    if (ew.with_exception([&hijacked](rocket::RocketException& rex) {
          hijacked = folly::exception_wrapper(
              apache::thrift::detail::EncodedError(rex.moveErrorData()));
        })) {
      serverPush(folly::Try<StreamPayload>(std::move(hijacked)));
    } else {
      serverPush(folly::Try<StreamPayload>(std::move(ew)));
    }
    close();
  }

  bool onSinkRequestN(uint64_t n) override {
    serverPush(n);
    return true;
  }

  void consume() {
    DCHECK(evb_);
    evb_->runInEventBaseThread(
        [self = copy()]() mutable { self->processServerMessages(); });
  }

  void canceled() {}

 private:
  void processServerMessages() {
    if (!serverCallback_) {
      return;
    }

    do {
      for (auto messages = serverGetMessages(); !messages.empty();
           messages.pop()) {
        bool terminated = false;
        ServerMessage message = std::move(messages.front());
        folly::variant_match(
            message,
            [&](folly::Try<StreamPayload>& payload) {
              if (payload.hasException()) {
                serverCallback_->onSinkError(std::move(payload).exception());
                terminated = true;
              } else {
                terminated =
                    !serverCallback_->onSinkNext(std::move(payload).value());
              }
            },
            [&](const StreamCancel&) {
              serverCallback_->onStreamCancel();
              terminated = true;
            },
            [&](const SinkComplete&) {
              terminated = !serverCallback_->onSinkComplete();
            });
        if (terminated) {
          close();
          return;
        }
      }
    } while (!serverWait(this));
  }

 private:
  folly::coro::Baton firstResponseBaton_{};
  folly::Try<FirstResponsePayload> firstResponse_;

  SinkServerCallback* serverCallback_{nullptr};
  folly::Executor::KeepAlive<folly::EventBase> evb_;
};
#else
class ClientSinkBridge {
 public:
  using Ptr = std::unique_ptr<ClientSinkBridge>;
};
#endif

} // namespace detail
} // namespace thrift
} // namespace apache
