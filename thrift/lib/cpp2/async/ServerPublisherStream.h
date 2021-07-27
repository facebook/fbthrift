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
#include <folly/synchronization/AtomicUtil.h>
#include <folly/synchronization/Baton.h>
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
    void operator()(ServerPublisherStream<T>* ptr) { ptr->decref(); }
  };

  struct CancelDeleter {
    void operator()(ServerPublisherStream<T>* ptr) {
      ptr->onStreamCompleteOrCancel_.call();
      ptr->decref();
    }
  };

  struct CallOnceFunction {
   private:
    struct Function {
      virtual void operator()() = 0;
      virtual ~Function() = default;
    };
    template <typename Func>
    struct FunctionHolder : public Function {
      explicit FunctionHolder(Func&& f) : f_(std::forward<Func>(f)) {}
      void operator()() override { f_(); }

     private:
      Func f_;
    };

   public:
    template <typename Func>
    CallOnceFunction(Func&& f)
        : storage_(
              reinterpret_cast<intptr_t>(
                  new FunctionHolder<folly::remove_cvref_t<Func>>(
                      std::forward<Func>(f))) |
              static_cast<intptr_t>(Type::READY)) {}
    ~CallOnceFunction() {
      DCHECK_EQ(
          storage_.load(std::memory_order_relaxed) & kTypeMask,
          static_cast<intptr_t>(Type::DONE));
    }

    explicit operator bool() const {
      auto type = static_cast<Type>(
          storage_.load(std::memory_order_relaxed) & kTypeMask);
      return type == Type::READY;
    }

    void call() {
      auto storage = storage_.load(std::memory_order_relaxed);
      if (static_cast<Type>(storage & kTypeMask) == Type::READY) {
        if (folly::atomic_compare_exchange_strong_explicit(
                &storage_,
                &storage,
                static_cast<intptr_t>(Type::IN_PROGRESS),
                std::memory_order_acquire,
                std::memory_order_relaxed)) {
          auto func = reinterpret_cast<Function*>(storage & kPtrMask);
          storage = static_cast<intptr_t>(Type::IN_PROGRESS);

          SCOPE_EXIT {
            if (!folly::atomic_compare_exchange_strong_explicit(
                    &storage_,
                    &storage,
                    static_cast<intptr_t>(Type::DONE),
                    std::memory_order_release,
                    std::memory_order_acquire)) {
              DCHECK_EQ(
                  storage & kTypeMask, static_cast<intptr_t>(Type::WAITING));
              reinterpret_cast<folly::Baton<>*>(storage & kPtrMask)->post();
            }
            delete func;
          };

          (*func)();
        }
      }
    }

    void callOrJoin() {
      call();
      auto storage = storage_.load(std::memory_order_relaxed);
      if (static_cast<Type>(storage & kTypeMask) == Type::IN_PROGRESS) {
        folly::Baton<> baton;
        if (folly::atomic_compare_exchange_strong_explicit(
                &storage_,
                &storage,
                reinterpret_cast<intptr_t>(&baton) |
                    static_cast<intptr_t>(Type::WAITING),
                std::memory_order_release,
                std::memory_order_acquire)) {
          baton.wait();
          // no one observes this final state transition; just for completeness
          DCHECK_EQ(
              storage_.exchange(
                  static_cast<intptr_t>(Type::DONE), std::memory_order_relaxed),
              reinterpret_cast<intptr_t>(&baton) |
                  static_cast<intptr_t>(Type::WAITING));
        }
      }
    }

   private:
    enum class Type : uint8_t {
      READY = 0,
      IN_PROGRESS = 1,
      WAITING = 2,
      DONE = 3
    };
    const intptr_t kTypeMask = 0x3;
    const intptr_t kPtrMask = ~kTypeMask;
    std::atomic<intptr_t> storage_{0};
  };

  union CreditBuffer {
    using Queue =
        typename twowaybridge_detail::Queue<folly::Try<StreamPayload>>;
    static constexpr uint64_t creditValSize = sizeof(void*) * 8 - 1;
    static constexpr uint64_t maxCreditVal = (1ull << creditValSize) - 1;

    Queue buffer;
    struct {
      uint64_t isSet : 1;
      uint64_t val : creditValSize;
    } credits;

    CreditBuffer() {
      static_assert(
          sizeof credits == sizeof buffer,
          "CreditBuffer members must be the same size");
      credits.isSet = true;
      credits.val = 0;
    }
    ~CreditBuffer() {
      if (!credits.isSet) {
        buffer.~Queue();
      }
    }
    void addCredits(int64_t delta) {
      DCHECK(credits.isSet);
      credits.val = std::min(maxCreditVal, credits.val + delta);
    }
    bool hasCredit() { return credits.isSet && credits.val; }
    void storeBuffer(Queue&& buf) {
      if (credits.isSet) {
        new (this) Queue(std::move(buf));
      } else {
        buffer = std::move(buf);
      }
    }
    void storeCredits(uint64_t val) {
      if (!credits.isSet) {
        buffer.~Queue();
        credits.isSet = true;
      }
      credits.val = std::min(maxCreditVal, val);
    }
    Queue getBuffer() { return credits.isSet ? Queue() : std::move(buffer); }
    uint64_t getCredits() { return credits.isSet ? credits.val : 0; }
  };

 public:
  using Ptr = std::unique_ptr<ServerPublisherStream<T>, Deleter>;

  template <typename Func>
  static std::pair<ServerStreamFn<T>, ServerStreamPublisher<T>> create(
      Func onStreamCompleteOrCancel) {
    auto stream =
        new ServerPublisherStream<T>(std::move(onStreamCompleteOrCancel));
    return {
        [stream =
             std::unique_ptr<ServerPublisherStream<T>, CancelDeleter>(stream)](
            folly::Executor::KeepAlive<> serverExecutor,
            folly::Try<StreamPayload> (*encode)(folly::Try<T> &&)) mutable {
          stream->serverExecutor_ = std::move(serverExecutor);

          while (auto messages =
                     stream->encodeOrQueue_.closeOrGetMessages(encode)) {
            for (; !messages.empty(); messages.pop()) {
              stream->queue_.push(encode(std::move(messages.front())));
            }
          }

          return ServerStreamFactory([stream = std::move(stream)](
                                         FirstResponsePayload&& payload,
                                         StreamClientCallback* callback,
                                         folly::EventBase* clientEb,
                                         TilePtr&& interaction) mutable {
            stream->streamClientCallback_ = callback;
            stream->clientEventBase_ = clientEb;
            stream->interaction_ =
                TileStreamGuard::transferFrom(std::move(interaction));
            std::ignore = callback->onFirstResponse(
                std::move(payload), clientEb, stream.release());
          });
        },
        ServerStreamPublisher<T>(stream->copy())};
  }

  void publish(folly::Try<T>&& payload) {
    bool close = !payload.hasValue();

    // pushOrGetClosedPayload only moves from payload on success
    if (auto encode =
            encodeOrQueue_.pushOrGetClosedPayload(std::move(payload))) {
      queue_.push(encode(std::move(payload)));
    }

    if (close) {
      // ensure the callback has completed when we return from complete()
      // (if started from onStreamCancel())
      onStreamCompleteOrCancel_.callOrJoin();
    }
  }

  bool wasCancelled() { return !onStreamCompleteOrCancel_; }

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
  template <typename Func>
  explicit ServerPublisherStream(Func onStreamCompleteOrCancel)
      : streamClientCallback_(nullptr),
        clientEventBase_(nullptr),
        onStreamCompleteOrCancel_(std::move(onStreamCompleteOrCancel)) {}

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
    if (!creditBuffer_.hasCredit()) {
      // we need creditBuffer_ to hold credits before calling processPayloads
      auto buffer = creditBuffer_.getBuffer();
      creditBuffer_.storeCredits(credits);
      // we are responsible for waking the queue reader
      return processPayloads(std::move(buffer));
    } else {
      creditBuffer_.addCredits(credits);
      return true;
    }
  }

  void onStreamCancel() override {
    clientEventBase_->dcheckIsInEventBaseThread();
    serverExecutor_->add([ex = serverExecutor_, self = copy()] {
      self->onStreamCompleteOrCancel_.call();
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
  bool processPayloads(
      typename CreditBuffer::Queue buffer = typename CreditBuffer::Queue()) {
    clientEventBase_->dcheckIsInEventBaseThread();
    DCHECK(encodeOrQueue_.isClosed());

    DCHECK(!queue_.isClosed());
    DCHECK(creditBuffer_.hasCredit());

    // returns stream liveness
    auto processQueue = [&] {
      for (; creditBuffer_.hasCredit() && !buffer.empty(); buffer.pop()) {
        DCHECK(!queue_.isClosed());
        auto payload = std::move(buffer.front());
        if (payload.hasValue()) {
          auto alive =
              streamClientCallback_->onStreamNext(std::move(payload.value()));
          if (!alive) {
            return false;
          }
          creditBuffer_.addCredits(-1);
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

    while (creditBuffer_.hasCredit() && !queue_.wait(this)) {
      DCHECK(buffer.empty());
      buffer = queue_.getMessages();
      if (!processQueue()) {
        return false;
      }
    }

    if (!buffer.empty()) {
      DCHECK(!creditBuffer_.hasCredit());
      creditBuffer_.storeBuffer(std::move(buffer));
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

  CallOnceFunction onStreamCompleteOrCancel_;

  using EncodeFn = typename std::remove_pointer_t<folly::Try<StreamPayload> (*)(
      folly::Try<T>&&)>;
  typename twowaybridge_detail::AtomicQueueOrPtr<folly::Try<T>, EncodeFn>
      encodeOrQueue_;

  // must only be read/written on client thread
  CreditBuffer creditBuffer_;

  TileStreamGuard interaction_;
};

} // namespace detail

template <typename T>
class ServerStreamPublisher {
 public:
  void next(T payload) const {
    impl_->publish(folly::Try<T>(std::move(payload)));
  }
  void complete(folly::exception_wrapper ew) && {
    std::exchange(impl_, nullptr)->publish(folly::Try<T>(std::move(ew)));
  }
  void complete() && {
    std::exchange(impl_, nullptr)->publish(folly::Try<T>{});
  }

  explicit ServerStreamPublisher(
      typename apache::thrift::detail::ServerPublisherStream<T>::Ptr impl)
      : impl_(std::move(impl)) {}
  ServerStreamPublisher(ServerStreamPublisher&&) = default;
  ServerStreamPublisher& operator=(ServerStreamPublisher&&) = default;
  ~ServerStreamPublisher() {
    CHECK(!impl_ || impl_->wasCancelled())
        << "StreamPublisher has to be completed or canceled.";
  }

 private:
  typename apache::thrift::detail::ServerPublisherStream<T>::Ptr impl_;
};

} // namespace thrift
} // namespace apache
