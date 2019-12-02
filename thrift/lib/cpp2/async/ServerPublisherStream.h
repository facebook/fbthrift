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

#include <folly/Try.h>
#include <thrift/lib/cpp2/async/ServerStreamDetail.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>

namespace apache {
namespace thrift {
template <typename T>
class ServerStreamPublisher;

namespace detail {
template <typename T>
class ServerPublisherStream : private StreamServerCallback {
  struct Deleter {
    void operator()(ServerPublisherStream<T>* ptr) {
      ptr->decref();
    }
  };

 public:
  using Ptr = std::unique_ptr<ServerPublisherStream<T>, Deleter>;

  static std::pair<ServerStreamFn<T>, ServerStreamPublisher<T>> create(
      folly::Function<void()> onStreamCompleteOrCancel) {
    auto stream = new detail::ServerPublisherStream<T>(
        std::move(onStreamCompleteOrCancel));
    return {[stream](
                folly::Executor::KeepAlive<> serverExecutor,
                folly::Try<StreamPayload> (*encode)(folly::Try<T> &&)) mutable {
              stream->serverExecutor_ = std::move(serverExecutor);
              stream->encode_ = encode;
              return [stream](
                         FirstResponsePayload&& payload,
                         StreamClientCallback* callback,
                         folly::EventBase* clientEb) mutable {
                stream->streamClientCallback_ = callback;
                stream->clientEventBase_ = clientEb;
                callback->onFirstResponse(std::move(payload), clientEb, stream);
                return stream;
              };
            },
            ServerStreamPublisher<T>(stream->copy())};
  }

  void publish(folly::Try<T>&&) {}
  bool wasCancelled() {
    return false;
  }

 private:
  explicit ServerPublisherStream(
      folly::Function<void()> onStreamCompleteOrCancel)
      : streamClientCallback_(nullptr),
        clientEventBase_(nullptr),
        onStreamCompleteOrCancel_(std::move(onStreamCompleteOrCancel)),
        encode_(nullptr) {}

  Ptr copy() {
    auto refCount = refCount_.fetch_add(1, std::memory_order_relaxed);
    DCHECK(refCount > 0);
    return Ptr(this);
  }
  void decref() {
    if (refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

  void onStreamRequestN(uint64_t) override {}
  void onStreamCancel() override {}
  void resetClientCallback(StreamClientCallback&) override {}

  std::atomic<int8_t> refCount_{1};
  StreamClientCallback* streamClientCallback_;
  folly::Executor::KeepAlive<> serverExecutor_;
  folly::EventBase* clientEventBase_;
  folly::Function<void()> onStreamCompleteOrCancel_;
  folly::Try<StreamPayload> (*encode_)(folly::Try<T>&&);
};

} // namespace detail

template <typename T>
class ServerStreamPublisher {
 public:
  void next(T payload) {
    impl_->publish(folly::Try<T>(std::move(payload)));
  }
  void complete(folly::exception_wrapper ew) && {
    std::exchange(impl_, nullptr)->publish(folly::Try<T>(std::move(ew)));
  }
  void complete() && {
    std::exchange(impl_, nullptr)->publish(folly::Try<T>{});
  }

  explicit ServerStreamPublisher(
      typename detail::ServerPublisherStream<T>::Ptr impl)
      : impl_(std::move(impl)) {}
  ServerStreamPublisher(ServerStreamPublisher&&) = default;
  ServerStreamPublisher& operator=(ServerStreamPublisher&&) = default;
  ~ServerStreamPublisher() {
    CHECK(!impl_ || impl_->wasCancelled())
        << "StreamPublisher has to be completed or canceled.";
  }

 private:
  typename detail::ServerPublisherStream<T>::Ptr impl_;
};

} // namespace thrift
} // namespace apache
