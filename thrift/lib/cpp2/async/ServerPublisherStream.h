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
    return {[stream = Ptr(stream)](
                folly::Executor::KeepAlive<> serverExecutor,
                folly::Try<StreamPayload> (*encode)(folly::Try<T> &&)) mutable {
              stream->serverExecutor_ = std::move(serverExecutor);

              {
                std::lock_guard<std::mutex> guard(stream->encodeMutex_);
                for (auto messages = stream->unencodedQueue_.getMessages();
                     !messages.empty();
                     messages.pop()) {
                  stream->queue_.push(encode(std::move(messages.front())));
                }
                stream->encode_ = encode;
              }

              return [stream = std::move(stream)](
                         FirstResponsePayload&& payload,
                         StreamClientCallback* callback,
                         folly::EventBase* clientEb) mutable {
                stream->streamClientCallback_ = callback;
                stream->clientEventBase_ = clientEb;
                std::ignore = callback->onFirstResponse(
                    std::move(payload), clientEb, stream.release());
              };
            },
            ServerStreamPublisher<T>(stream->copy())};
  }

  void publish(folly::Try<T>&& payload) {
    bool close = !payload.hasValue();

    {
      std::unique_lock<std::mutex> guard(encodeMutex_);
      if (encode_) {
        guard.unlock();
        queue_.push(encode_(std::move(payload)));
      } else {
        unencodedQueue_.push(std::move(payload));
      }
    }

    if (close) {
      std::lock_guard<std::mutex> guard(callbackMutex_);
      if (onStreamCompleteOrCancel_) {
        std::exchange(onStreamCompleteOrCancel_, nullptr)();
      }
    }
  }

  bool wasCancelled() {
    std::lock_guard<std::mutex> guard(callbackMutex_);
    return !onStreamCompleteOrCancel_;
  }

  void consume() {
    clientEventBase_->add([self = copy()]() {
      if (self->queue_.isClosed()) {
        return;
      }
      self->processPayloads();
    });
  }
  void canceled() {}

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

  template <typename Payload>
  using Queue = typename twowaybridge_detail::
      AtomicQueue<ServerPublisherStream, folly::Try<Payload>>;

  bool onStreamRequestN(uint64_t credits) override {
    clientEventBase_->dcheckIsInEventBaseThread();
    credits_ += credits;
    if (credits_ == credits) {
      // we are responsible for waking the queue reader
      return processPayloads();
    }
    return true;
  }

  void onStreamCancel() override {
    clientEventBase_->dcheckIsInEventBaseThread();
    serverExecutor_->add([ex = serverExecutor_, self = copy()] {
      auto onStreamCompleteOrCancel = [&] {
        std::lock_guard<std::mutex> guard(self->callbackMutex_);
        return std::exchange(self->onStreamCompleteOrCancel_, nullptr);
      }();
      if (onStreamCompleteOrCancel) {
        onStreamCompleteOrCancel();
      }
    });
    queue_.close();
    close();
  }

  void resetClientCallback(StreamClientCallback& clientCallback) override {
    streamClientCallback_ = &clientCallback;
  }

  // resume processing buffered requests
  // called by onStreamRequestN when credits were previously empty,
  // otherwise by queue on publish
  bool processPayloads() {
    clientEventBase_->dcheckIsInEventBaseThread();
    DCHECK(encode_);

    DCHECK(!queue_.isClosed());

    // returns stream completion status
    auto processQueue = [&] {
      for (; credits_ && !buffer_.empty(); buffer_.pop()) {
        DCHECK(!queue_.isClosed());
        auto payload = std::move(buffer_.front());
        if (payload.hasValue()) {
          auto alive =
              streamClientCallback_->onStreamNext(std::move(payload.value()));
          --credits_;
          if (!alive) {
            return false;
          }
        } else if (payload.hasException()) {
          streamClientCallback_->onStreamError(std::move(payload.exception()));
          close();
          return false;
        } else {
          streamClientCallback_->onStreamComplete();
          close();
          return false;
        }
      }
      return true;
    };

    if (!processQueue()) {
      return false;
    }

    DCHECK(!queue_.isClosed());

    while (credits_ && !queue_.wait(this)) {
      DCHECK(buffer_.empty());
      buffer_ = queue_.getMessages();
      if (!processQueue()) {
        return false;
      }
    }

    return true;
  }

  void close() {
    serverExecutor_.reset();
    Ptr(this);
  }

  void decref() {
    if (refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

  std::atomic<int8_t> refCount_{1};
  StreamClientCallback* streamClientCallback_;
  folly::Executor::KeepAlive<> serverExecutor_;
  folly::EventBase* clientEventBase_;

  Queue<StreamPayload> queue_;

  // this mutex will turn into a state atomic in a future diff
  folly::Function<void()> onStreamCompleteOrCancel_;
  std::mutex callbackMutex_;

  // these will be combined into a single atomic in a future diff
  Queue<T> unencodedQueue_;
  folly::Try<StreamPayload> (*encode_)(folly::Try<T>&&);
  std::mutex encodeMutex_;

  // these will be combined into a single atomic in a future diff
  // must only be read/written on client thread
  typename Queue<StreamPayload>::MessageQueue buffer_;
  std::atomic<uint64_t> credits_{0};
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
