/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Executor.h>
#include <folly/Portability.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/ThriftServerAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponseError.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponsePayloads.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::fast_thrift::thrift {

namespace detail {

inline constexpr auto kHandlerCallbackNotCompleted =
    "FastHandlerCallback not completed";
inline constexpr auto kHandlerExecutorUnavailable =
    "Fast handler executor rejected or dropped request";

enum class HandlerState : uint8_t {
  AwaitingDispatch,
  Running,
  Completed,
};

template <typename U>
struct IsUniquePtr : std::false_type {};
template <typename U>
struct IsUniquePtr<std::unique_ptr<U>> : std::true_type {};

FOLLY_EXPORT inline folly::Executor*& currentHandlerExecutor() noexcept {
  static thread_local folly::Executor* executor = nullptr;
  return executor;
}

inline bool isOnHandlerExecutor(folly::Executor* executor) noexcept {
  return executor == nullptr || executor == currentHandlerExecutor();
}

class HandlerExecutorScope {
 public:
  explicit HandlerExecutorScope(folly::Executor* executor) noexcept
      : previous_(std::exchange(currentHandlerExecutor(), executor)) {}

  HandlerExecutorScope(const HandlerExecutorScope&) = delete;
  HandlerExecutorScope& operator=(const HandlerExecutorScope&) = delete;
  HandlerExecutorScope(HandlerExecutorScope&&) = delete;
  HandlerExecutorScope& operator=(HandlerExecutorScope&&) = delete;

  ~HandlerExecutorScope() { currentHandlerExecutor() = previous_; }

 private:
  folly::Executor* previous_;
};

template <typename F>
class HandlerExecutorTask {
  static_assert(std::is_nothrow_move_constructible_v<F>);

 private:
  struct State {
    explicit State(folly::Executor::KeepAlive<folly::EventBase> evb) noexcept
        : evb(std::move(evb)) {}

    folly::Executor::KeepAlive<folly::EventBase> evb;
    std::optional<F> fn;
  };

 public:
  HandlerExecutorTask(folly::Executor::KeepAlive<folly::EventBase> evb, F&& fn)
      : state_(std::make_unique<State>(std::move(evb))) {
    state_->fn.emplace(static_cast<F&&>(fn));
  }

  template <typename Factory>
  HandlerExecutorTask(
      folly::Executor::KeepAlive<folly::EventBase> evb,
      std::in_place_t,
      Factory&& factory)
      : state_(std::make_unique<State>(std::move(evb))) {
    state_->fn.emplace(static_cast<Factory&&>(factory)());
  }

  HandlerExecutorTask(const HandlerExecutorTask&) = delete;
  HandlerExecutorTask& operator=(const HandlerExecutorTask&) = delete;
  HandlerExecutorTask(HandlerExecutorTask&&) noexcept = default;
  HandlerExecutorTask& operator=(HandlerExecutorTask&&) = delete;

  ~HandlerExecutorTask() {
    static_assert(std::is_nothrow_invocable_v<F&>);

    if (!state_) {
      return;
    }
    auto state = std::move(state_);
    auto* evb = state->evb.get();
    assert(evb != nullptr);
    if (evb == nullptr) {
      (*state->fn)();
      return;
    }
    auto fallback = [state = std::move(state)]() mutable noexcept {
      (*state->fn)();
    };
    static_assert(
        sizeof(fallback) <= 6 * sizeof(void*),
        "fallback task outgrew folly::Function's in-situ buffer");
    evb->runImmediatelyOrRunInEventBaseThread(std::move(fallback));
  }

  void run(folly::Executor* executor) noexcept {
    if (!state_) {
      return;
    }
    auto state = std::move(state_);
    HandlerExecutorScope scope(executor);
    (*state->fn)();
  }

 private:
  std::unique_ptr<State> state_;
};

template <typename F>
void runOnHandlerExecutor(
    folly::Executor* executor, HandlerExecutorTask<F>&& task) noexcept {
  auto run = [executor, task = std::move(task)]() mutable noexcept {
    task.run(executor);
  };
  static_assert(
      sizeof(run) <= 6 * sizeof(void*),
      "completion task outgrew folly::Function's in-situ buffer");
  try {
    executor->add(std::move(run));
  } catch (...) {
    // Destruction of the sole task owner performs the fallback. A second
    // action here would compete if add() consumed the task before throwing.
  }
}

template <typename F>
void completeOnHandlerExecutorNothrow(
    HandlerState& state,
    ThriftServerAppAdapter* handler,
    uint32_t streamId,
    std::unique_ptr<ThriftRequestContext>& requestContext,
    folly::DelayedDestruction::DestructorGuard& adapterGuard,
    const folly::Executor::KeepAlive<folly::EventBase>& evb,
    folly::Executor* executor,
    F&& fn) {
  static_assert(std::is_nothrow_move_constructible_v<std::decay_t<F>>);

  assert(state != HandlerState::Completed);
  assert(!isOnHandlerExecutor(executor));

  auto makeTask = [&]() noexcept {
    return [handler,
            streamId,
            requestContext = std::move(requestContext),
            adapterGuard = std::move(adapterGuard),
            fn = static_cast<F&&>(fn)]() mutable noexcept {
      fn(handler, streamId, std::move(requestContext), std::move(adapterGuard));
    };
  };
  using Task = HandlerExecutorTask<decltype(makeTask())>;

  // State allocation completes before makeTask() moves requestContext and
  // adapterGuard out of the callback.
  Task task(evb, std::in_place, makeTask);
  state = HandlerState::Completed;
  runOnHandlerExecutor(executor, std::move(task));
}

template <typename F>
void completeOnHandlerExecutor(
    HandlerState& state,
    ThriftServerAppAdapter* handler,
    uint32_t streamId,
    std::unique_ptr<ThriftRequestContext>& requestContext,
    folly::DelayedDestruction::DestructorGuard& adapterGuard,
    const folly::Executor::KeepAlive<folly::EventBase>& evb,
    folly::Executor* executor,
    F&& fn) {
  using Completion = std::decay_t<F>;
  if constexpr (std::is_nothrow_move_constructible_v<Completion>) {
    completeOnHandlerExecutorNothrow(
        state,
        handler,
        streamId,
        requestContext,
        adapterGuard,
        evb,
        executor,
        static_cast<F&&>(fn));
  } else {
    // The box is created before requestContext or adapterGuard moves. Generated
    // result types avoid this allocation; custom throwing-move types require
    // the indirection only when their completion is deferred.
    auto boxed = std::make_unique<Completion>(static_cast<F&&>(fn));
    completeOnHandlerExecutorNothrow(
        state,
        handler,
        streamId,
        requestContext,
        adapterGuard,
        evb,
        executor,
        [boxed = std::move(boxed)](
            ThriftServerAppAdapter* handler,
            uint32_t streamId,
            std::unique_ptr<ThriftRequestContext> requestContext,
            folly::DelayedDestruction::DestructorGuard&&
                adapterGuard) mutable noexcept {
          (*boxed)(
              handler,
              streamId,
              std::move(requestContext),
              std::move(adapterGuard));
        });
  }
}

/**
 * Sole-ownership handle for a FastHandlerCallback.
 *
 * Move-only on purpose. A request has exactly one owner at any instant and
 * ownership is handed along a chain — dispatcher, then CPU pool task, then
 * handler, then possibly a continuation — with each hand-off carrying the
 * happens-before edge of whatever moved it (the executor queue, a future's
 * fulfilment, an EventBase notification queue). Two threads never hold the
 * callback at once, so nothing here needs to be atomic.
 *
 * Making the handle uncopyable is what enforces that: a second simultaneous
 * owner would reintroduce the "who destroys it?" question that a reference
 * count exists to answer, and there is deliberately no reference count.
 */
template <typename T>
class CallbackPtr {
 public:
  CallbackPtr() = default;
  explicit CallbackPtr(T* ptr) noexcept : ptr_(ptr) {}

  CallbackPtr(const CallbackPtr&) = delete;
  CallbackPtr& operator=(const CallbackPtr&) = delete;

  CallbackPtr(CallbackPtr&& other) noexcept
      : ptr_(std::exchange(other.ptr_, nullptr)) {}
  CallbackPtr& operator=(CallbackPtr&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = std::exchange(other.ptr_, nullptr);
    }
    return *this;
  }

  ~CallbackPtr() { reset(); }

  void reset() noexcept {
    if (auto* ptr = std::exchange(ptr_, nullptr)) {
      ptr->destroyOnEventBase();
    }
  }

  // Yields the pointer without destroying it, for a hand-off that cannot carry
  // the handle itself. The caller takes on the destroy-on-EventBase
  // obligation, so ownership is still held by exactly one party.
  [[nodiscard]] T* release() noexcept { return std::exchange(ptr_, nullptr); }

  T* get() const noexcept { return ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  friend bool operator==(const CallbackPtr& ptr, std::nullptr_t) noexcept {
    return ptr.ptr_ == nullptr;
  }

 private:
  T* ptr_{nullptr};
};

// what() materializes a std::string, so it can throw under memory pressure.
// The callers are noexcept, so an allocation failure degrades to an empty
// reason rather than terminating the process.
inline std::string exceptionMessage(
    const folly::exception_wrapper& ew) noexcept {
  try {
    return ew.what().toStdString();
  } catch (...) {
    return {};
  }
}

inline void writeAppError(
    ThriftServerAppAdapter* handler,
    uint32_t streamId,
    std::unique_ptr<ThriftRequestContext> requestContext,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard,
    const folly::exception_wrapper& ew) noexcept {
  auto message = makeAppErrorMessage(
      streamId,
      "TApplicationException",
      exceptionMessage(ew),
      apache::thrift::ErrorBlame::SERVER);
  message.requestContext = std::move(requestContext);
  handler->writeResponse(std::move(message), std::move(adapterGuard));
}

// Shared exception cascade — declared exception via insert_exn (success
// frame with declaredException metadata) vs undeclared (success frame with
// appUnknownException metadata). Templated on Presult/ProtocolWriter so the
// per-method types only need to flow in from the codegen instantiation
// site; the kHasReturnType bit is also a template arg because insert_exn
// itself is templated on it.
template <typename Presult, typename ProtocolWriter, bool HasReturnType>
inline void writeExceptionCascade(
    ThriftServerAppAdapter* a,
    uint32_t sid,
    std::unique_ptr<ThriftRequestContext> requestContext,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard,
    folly::exception_wrapper ew) noexcept {
  Presult presult;
  std::optional<apache::thrift::ErrorClassification> classification;
  bool handled = ::apache::thrift::detail::ap::insert_exn<HasReturnType>(
      presult, ew, [&]<typename Ex>(Ex&) {
        classification = getDeclaredExceptionClassification<Ex>(ew);
      });
  auto message = handled ? makeDeclaredExceptionMessage<ProtocolWriter>(
                               sid, presult, ew, classification)
                         : makeUnknownExceptionMessage(
                               sid, ew, apache::thrift::ErrorBlame::SERVER);
  message.requestContext = std::move(requestContext);
  a->writeResponse(std::move(message), std::move(adapterGuard));
}

} // namespace detail

/**
 * Per-request completion handle handed to a FastServiceHandler method.
 *
 * Lifetime is sole ownership, not shared: exactly one owner at any instant,
 * passed hand to hand — dispatcher, then CPU pool task, then handler, then
 * possibly a continuation. Each hand-off travels through something that
 * already synchronizes (an executor queue, a future, an EventBase queue), so
 * the new owner sees everything the previous one wrote and no two threads
 * ever touch the callback at once. That is why there is no reference count
 * here, atomic or otherwise, and no vtable.
 *
 * The one placement requirement is that *destruction* lands on the adapter's
 * EventBase, because it releases two counts that are deliberately non-atomic
 * — the adapter's DelayedDestruction guard and, via the request context, a
 * ThriftConnContext reference. destroyOnEventBase enforces that, independent
 * of which thread happened to own the callback last.
 *
 * Construction must happen on the EventBase for the mirror-image reason: the
 * constructor acquires the adapter guard.
 */
template <typename T>
class FastHandlerCallback {
 public:
  using ResultFn = void (*)(
      ThriftServerAppAdapter*,
      uint32_t,
      std::unique_ptr<ThriftRequestContext>,
      folly::DelayedDestruction::DestructorGuard&&,
      T&&) noexcept;
  using ExceptionFn = void (*)(
      ThriftServerAppAdapter*,
      uint32_t,
      std::unique_ptr<ThriftRequestContext>,
      folly::DelayedDestruction::DestructorGuard&&,
      folly::exception_wrapper) noexcept;

  // Must be constructed on `evb`: adapterGuard_ bumps the adapter's
  // non-atomic guardCount_.
  FastHandlerCallback(
      ResultFn resultFn,
      ExceptionFn exceptionFn,
      ThriftServerAppAdapter* handler,
      uint32_t streamId,
      folly::EventBase& evb,
      folly::Executor* executor,
      std::unique_ptr<ThriftRequestContext> requestContext)
      : resultFn_(resultFn),
        exceptionFn_(exceptionFn),
        handler_(handler),
        adapterGuard_(handler),
        streamId_(streamId),
        evb_(folly::getKeepAliveToken(&evb)),
        executor_(executor),
        requestContext_(std::move(requestContext)),
        state_(
            executor == nullptr ? detail::HandlerState::Running
                                : detail::HandlerState::AwaitingDispatch) {}

  FastHandlerCallback(const FastHandlerCallback&) = delete;
  FastHandlerCallback& operator=(const FastHandlerCallback&) = delete;
  FastHandlerCallback(FastHandlerCallback&&) = delete;
  FastHandlerCallback& operator=(FastHandlerCallback&&) = delete;

  // Callable from any thread when the request uses a CPU executor.
  // Completion returns to that executor for response serialization, then the
  // adapter's writeResponse handles the EventBase hop. Without an executor,
  // completion must stay on the EventBase.
  //
  // Safe even after the connection has been force-closed: the donated guard
  // keeps the adapter alive, and writeResponse drops the write because
  // pipelineActive_ is false.
  void result(T value) {
    if (tryCompleteInline([&]() noexcept {
          resultFn_(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_),
              std::move(value));
        })) {
      return;
    }
    complete([resultFn = resultFn_, value = std::move(value)](
                 ThriftServerAppAdapter* handler,
                 uint32_t streamId,
                 std::unique_ptr<ThriftRequestContext> requestContext,
                 folly::DelayedDestruction::DestructorGuard&&
                     adapterGuard) mutable noexcept {
      resultFn(
          handler,
          streamId,
          std::move(requestContext),
          std::move(adapterGuard),
          std::move(value));
    });
  }

  // The context rides onto the error response too: an exception reply is a
  // response like any other, and the write-side handlers owe it the same
  // request-derived state a success reply gets.
  void exception(folly::exception_wrapper ew) {
    if (tryCompleteInline([&]() noexcept {
          exceptionFn_(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_),
              std::move(ew));
        })) {
      return;
    }
    complete([exceptionFn = exceptionFn_, ew = std::move(ew)](
                 ThriftServerAppAdapter* handler,
                 uint32_t streamId,
                 std::unique_ptr<ThriftRequestContext> requestContext,
                 folly::DelayedDestruction::DestructorGuard&&
                     adapterGuard) mutable noexcept {
      exceptionFn(
          handler,
          streamId,
          std::move(requestContext),
          std::move(adapterGuard),
          std::move(ew));
    });
  }

  // Writes a TApplicationException frame directly rather than through the
  // declared/undeclared cascade, for requests that never decoded far enough
  // for the cascade to mean anything. Marks the callback complete so the
  // destructor does not add a second response.
  void sendAppError(const folly::exception_wrapper& ew) noexcept {
    if (tryCompleteInline([&]() noexcept {
          detail::writeAppError(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_),
              ew);
        })) {
      return;
    }
    try {
      complete([ew = ew](
                   ThriftServerAppAdapter* handler,
                   uint32_t streamId,
                   std::unique_ptr<ThriftRequestContext> requestContext,
                   folly::DelayedDestruction::DestructorGuard&&
                       adapterGuard) mutable noexcept {
        detail::writeAppError(
            handler,
            streamId,
            std::move(requestContext),
            std::move(adapterGuard),
            ew);
      });
    } catch (...) {
      // Callback destruction will synthesize the fallback response.
    }
  }

  uint32_t streamId() const noexcept { return streamId_; }

  folly::EventBase* getEventBase() const { return evb_.get(); }

  // Where a generated dispatcher should run a coroutine handler body. Null
  // when dispatch is configured to stay on the EventBase. Raw rather than a
  // KeepAlive: the executor outlives the
  // adapter, which this callback already keeps alive through adapterGuard_,
  // so a per-request refcount would buy nothing.
  folly::Executor* getHandlerExecutor() const noexcept { return executor_; }

  ThriftRequestContext* requestContext() const noexcept {
    return requestContext_.get();
  }

  // Distinguishes executor rejection/drop from an abandoned handler callback.
  void markHandlerStarted() noexcept {
    if (state_ == detail::HandlerState::AwaitingDispatch) {
      state_ = detail::HandlerState::Running;
    }
  }

  // Called by CallbackPtr when the sole owner lets go. Destruction has to land
  // on the adapter's EventBase: it releases adapterGuard_ and, through
  // requestContext_, a ThriftConnContext reference, and neither of those
  // counts is atomic.
  //
  // isInEventBaseThread answers "true" for a stopped loop, which is fine here:
  // we hold a KeepAlive on the EventBase, so its loop cannot have finished
  // while this object is alive. The stopped-loop case is only reachable from
  // tests driving an EventBase by hand, where deleting inline is what's
  // wanted.
  void destroyOnEventBase() noexcept {
    if (!evb_ || evb_->isInEventBaseThread()) {
      delete this;
      return;
    }
    evb_->runInEventBaseThread([this] { delete this; });
  }

  // ---- Codegen-targeted static helpers ----
  // Codegen instantiates these with the per-method Presult / ProtocolWriter
  // and passes the function pointers to the ctor above. Each thunk builds
  // the response message via util/ResponsePayloads.h and hands it to the
  // adapter's single writeResponse entry point.

  template <typename Presult, typename ProtocolWriter>
  static void writeSuccess(
      ThriftServerAppAdapter* a,
      uint32_t sid,
      std::unique_ptr<ThriftRequestContext> requestContext,
      folly::DelayedDestruction::DestructorGuard&& adapterGuard,
      T&& value) noexcept {
    Presult presult;
    if constexpr (detail::IsUniquePtr<T>::value) {
      presult.template get<0>().value = value.get();
    } else {
      presult.template get<0>().value = &value;
    }
    presult.setIsSet(0, true);
    auto message = makeSuccessResponseMessage<ProtocolWriter>(sid, presult);
    message.requestContext = std::move(requestContext);
    a->writeResponse(std::move(message), std::move(adapterGuard));
  }

  template <typename Presult, typename ProtocolWriter>
  static void writeException(
      ThriftServerAppAdapter* a,
      uint32_t sid,
      std::unique_ptr<ThriftRequestContext> requestContext,
      folly::DelayedDestruction::DestructorGuard&& adapterGuard,
      folly::exception_wrapper ew) noexcept {
    detail::
        writeExceptionCascade<Presult, ProtocolWriter, /*HasReturnType=*/true>(
            a,
            sid,
            std::move(requestContext),
            std::move(adapterGuard),
            std::move(ew));
  }

 private:
  template <typename F>
  bool tryCompleteInline(F&& fn) noexcept {
    static_assert(std::is_nothrow_invocable_v<F&>);
    if (state_ == detail::HandlerState::Completed) {
      return true;
    }
    if (!detail::isOnHandlerExecutor(executor_)) {
      return false;
    }
    state_ = detail::HandlerState::Completed;
    fn();
    return true;
  }

  template <typename F>
  void complete(F&& fn) {
    detail::completeOnHandlerExecutor(
        state_,
        handler_,
        streamId_,
        requestContext_,
        adapterGuard_,
        evb_,
        executor_,
        static_cast<F&&>(fn));
  }

  // Pre-dispatch failures must not retry the executor that rejected the task.
  void completeInline(folly::exception_wrapper ew) noexcept {
    state_ = detail::HandlerState::Completed;
    exceptionFn_(
        handler_,
        streamId_,
        std::move(requestContext_),
        std::move(adapterGuard_),
        std::move(ew));
  }

  // Non-virtual and private: the only caller is destroyOnEventBase, so there
  // is no vtable on this type. Destruction before dispatch reports executor
  // overload; destruction after dispatch reports an abandoned callback.
  ~FastHandlerCallback() {
    try {
      if (state_ == detail::HandlerState::Completed) {
        return;
      }
      if (state_ == detail::HandlerState::AwaitingDispatch) {
        completeInline(
            folly::make_exception_wrapper<TApplicationException>(
                TApplicationException::LOADSHEDDING,
                detail::kHandlerExecutorUnavailable));
        return;
      }
      exception(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::INTERNAL_ERROR,
              detail::kHandlerCallbackNotCompleted));
    } catch (...) {
      // Error construction must not make destruction terminate the process.
    }
  }

  ResultFn resultFn_;
  ExceptionFn exceptionFn_;
  ThriftServerAppAdapter* handler_;
  // Keeps the adapter alive while this request is outstanding. Acquired on
  // the EventBase in the constructor, and either donated to the write hop on
  // completion and ultimately released on the EventBase — never acquired or
  // released from a CPU thread, because the adapter's count is not atomic.
  folly::DelayedDestruction::DestructorGuard adapterGuard_;
  uint32_t streamId_;
  // Keepalive rather than a raw pointer: destroyOnEventBase may need to hop
  // onto this EventBase from a CPU thread, so it has to outlive us.
  folly::Executor::KeepAlive<folly::EventBase> evb_;
  // Non-owning; see getHandlerExecutor().
  folly::Executor* executor_{nullptr};
  std::unique_ptr<ThriftRequestContext> requestContext_;
  detail::HandlerState state_;
};

template <>
class FastHandlerCallback<void> {
 public:
  using DoneFn = void (*)(
      ThriftServerAppAdapter*,
      uint32_t,
      std::unique_ptr<ThriftRequestContext>,
      folly::DelayedDestruction::DestructorGuard&&) noexcept;
  using ExceptionFn = void (*)(
      ThriftServerAppAdapter*,
      uint32_t,
      std::unique_ptr<ThriftRequestContext>,
      folly::DelayedDestruction::DestructorGuard&&,
      folly::exception_wrapper) noexcept;

  // See FastHandlerCallback<T>'s constructor.
  FastHandlerCallback(
      DoneFn doneFn,
      ExceptionFn exceptionFn,
      ThriftServerAppAdapter* handler,
      uint32_t streamId,
      folly::EventBase& evb,
      folly::Executor* executor,
      std::unique_ptr<ThriftRequestContext> requestContext)
      : doneFn_(doneFn),
        exceptionFn_(exceptionFn),
        handler_(handler),
        adapterGuard_(handler),
        streamId_(streamId),
        evb_(folly::getKeepAliveToken(&evb)),
        executor_(executor),
        requestContext_(std::move(requestContext)),
        state_(
            executor == nullptr ? detail::HandlerState::Running
                                : detail::HandlerState::AwaitingDispatch) {}

  FastHandlerCallback(const FastHandlerCallback&) = delete;
  FastHandlerCallback& operator=(const FastHandlerCallback&) = delete;
  FastHandlerCallback(FastHandlerCallback&&) = delete;
  FastHandlerCallback& operator=(FastHandlerCallback&&) = delete;

  // Safe to call from any thread; see FastHandlerCallback<T>::result.
  void done() {
    if (tryCompleteInline([&]() noexcept {
          doneFn_(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_));
        })) {
      return;
    }
    complete([doneFn = doneFn_](
                 ThriftServerAppAdapter* handler,
                 uint32_t streamId,
                 std::unique_ptr<ThriftRequestContext> requestContext,
                 folly::DelayedDestruction::DestructorGuard&&
                     adapterGuard) mutable noexcept {
      doneFn(
          handler,
          streamId,
          std::move(requestContext),
          std::move(adapterGuard));
    });
  }

  // See FastHandlerCallback<T>::exception.
  void exception(folly::exception_wrapper ew) {
    if (tryCompleteInline([&]() noexcept {
          exceptionFn_(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_),
              std::move(ew));
        })) {
      return;
    }
    complete([exceptionFn = exceptionFn_, ew = std::move(ew)](
                 ThriftServerAppAdapter* handler,
                 uint32_t streamId,
                 std::unique_ptr<ThriftRequestContext> requestContext,
                 folly::DelayedDestruction::DestructorGuard&&
                     adapterGuard) mutable noexcept {
      exceptionFn(
          handler,
          streamId,
          std::move(requestContext),
          std::move(adapterGuard),
          std::move(ew));
    });
  }

  // See FastHandlerCallback<T>::sendAppError.
  void sendAppError(const folly::exception_wrapper& ew) noexcept {
    if (tryCompleteInline([&]() noexcept {
          detail::writeAppError(
              handler_,
              streamId_,
              std::move(requestContext_),
              std::move(adapterGuard_),
              ew);
        })) {
      return;
    }
    try {
      complete([ew = ew](
                   ThriftServerAppAdapter* handler,
                   uint32_t streamId,
                   std::unique_ptr<ThriftRequestContext> requestContext,
                   folly::DelayedDestruction::DestructorGuard&&
                       adapterGuard) mutable noexcept {
        detail::writeAppError(
            handler,
            streamId,
            std::move(requestContext),
            std::move(adapterGuard),
            ew);
      });
    } catch (...) {
      // Callback destruction will synthesize the fallback response.
    }
  }

  uint32_t streamId() const noexcept { return streamId_; }

  folly::EventBase* getEventBase() const { return evb_.get(); }

  // Where a generated dispatcher should run a coroutine handler body. Null
  // when dispatch is configured to stay on the EventBase. Raw rather than a
  // KeepAlive: the executor outlives the
  // adapter, which this callback already keeps alive through adapterGuard_,
  // so a per-request refcount would buy nothing.
  folly::Executor* getHandlerExecutor() const noexcept { return executor_; }

  ThriftRequestContext* requestContext() const noexcept {
    return requestContext_.get();
  }

  // Distinguishes executor rejection/drop from an abandoned handler callback.
  void markHandlerStarted() noexcept {
    if (state_ == detail::HandlerState::AwaitingDispatch) {
      state_ = detail::HandlerState::Running;
    }
  }

  // See FastHandlerCallback<T>::destroyOnEventBase.
  void destroyOnEventBase() noexcept {
    if (!evb_ || evb_->isInEventBaseThread()) {
      delete this;
      return;
    }
    evb_->runInEventBaseThread([this] { delete this; });
  }

  // ---- Codegen-targeted static helpers (void return) ----

  template <typename Presult, typename ProtocolWriter>
  static void writeDone(
      ThriftServerAppAdapter* a,
      uint32_t sid,
      std::unique_ptr<ThriftRequestContext> requestContext,
      folly::DelayedDestruction::DestructorGuard&& adapterGuard) noexcept {
    Presult presult;
    auto message = makeSuccessResponseMessage<ProtocolWriter>(sid, presult);
    message.requestContext = std::move(requestContext);
    a->writeResponse(std::move(message), std::move(adapterGuard));
  }

  template <typename Presult, typename ProtocolWriter>
  static void writeException(
      ThriftServerAppAdapter* a,
      uint32_t sid,
      std::unique_ptr<ThriftRequestContext> requestContext,
      folly::DelayedDestruction::DestructorGuard&& adapterGuard,
      folly::exception_wrapper ew) noexcept {
    detail::
        writeExceptionCascade<Presult, ProtocolWriter, /*HasReturnType=*/false>(
            a,
            sid,
            std::move(requestContext),
            std::move(adapterGuard),
            std::move(ew));
  }

 private:
  template <typename F>
  bool tryCompleteInline(F&& fn) noexcept {
    static_assert(std::is_nothrow_invocable_v<F&>);
    if (state_ == detail::HandlerState::Completed) {
      return true;
    }
    if (!detail::isOnHandlerExecutor(executor_)) {
      return false;
    }
    state_ = detail::HandlerState::Completed;
    fn();
    return true;
  }

  template <typename F>
  void complete(F&& fn) {
    detail::completeOnHandlerExecutor(
        state_,
        handler_,
        streamId_,
        requestContext_,
        adapterGuard_,
        evb_,
        executor_,
        static_cast<F&&>(fn));
  }

  // Pre-dispatch failures must not retry the executor that rejected the task.
  void completeInline(folly::exception_wrapper ew) noexcept {
    state_ = detail::HandlerState::Completed;
    exceptionFn_(
        handler_,
        streamId_,
        std::move(requestContext_),
        std::move(adapterGuard_),
        std::move(ew));
  }

  // See FastHandlerCallback<T>::~FastHandlerCallback.
  ~FastHandlerCallback() {
    try {
      if (state_ == detail::HandlerState::Completed) {
        return;
      }
      if (state_ == detail::HandlerState::AwaitingDispatch) {
        completeInline(
            folly::make_exception_wrapper<TApplicationException>(
                TApplicationException::LOADSHEDDING,
                detail::kHandlerExecutorUnavailable));
        return;
      }
      exception(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::INTERNAL_ERROR,
              detail::kHandlerCallbackNotCompleted));
    } catch (...) {
      // Error construction must not make destruction terminate the process.
    }
  }

  DoneFn doneFn_;
  ExceptionFn exceptionFn_;
  ThriftServerAppAdapter* handler_;
  // See FastHandlerCallback<T>::adapterGuard_.
  folly::DelayedDestruction::DestructorGuard adapterGuard_;
  uint32_t streamId_;
  // See FastHandlerCallback<T>::evb_.
  folly::Executor::KeepAlive<folly::EventBase> evb_;
  // Non-owning; see getHandlerExecutor().
  folly::Executor* executor_{nullptr};
  std::unique_ptr<ThriftRequestContext> requestContext_;
  detail::HandlerState state_;
};

template <typename T>
using FastHandlerCallbackPtr = detail::CallbackPtr<FastHandlerCallback<T>>;

// The callback sits on the per-request path, so it carries no vtable: nothing
// about it is dispatched dynamically, and destruction is driven by the handle
// rather than a virtual destructor. Asserted rather than commented so that
// reintroducing a base class or a virtual is a build failure.
static_assert(
    !std::is_polymorphic_v<FastHandlerCallback<void>>,
    "FastHandlerCallback must not gain a vtable");
static_assert(
    !std::is_polymorphic_v<FastHandlerCallback<int>>,
    "FastHandlerCallback must not gain a vtable");

// Unique ownership is what removes the need for a reference count. A copyable
// handle would allow two simultaneous owners and quietly bring the whole
// question back.
static_assert(
    !std::is_copy_constructible_v<FastHandlerCallbackPtr<void>> &&
        !std::is_copy_assignable_v<FastHandlerCallbackPtr<void>>,
    "FastHandlerCallbackPtr must stay move-only");

// Constructs a callback and returns the owning handle. Must be called on the
// adapter's EventBase — see FastHandlerCallback's constructor.
template <typename Cb, typename... Args>
detail::CallbackPtr<Cb> makeFastHandlerCallback(Args&&... args) {
  return detail::CallbackPtr<Cb>(new Cb(static_cast<Args&&>(args)...));
}

} // namespace apache::thrift::fast_thrift::thrift
