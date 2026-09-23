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

#include <cstdint>
#include <utility>

#include <rust/cxx.h>
#include <folly/Function.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/rust/CallbackContext.h>
#include <thrift/lib/rust/channel_pipeline/src/ffi.rs.h>

namespace channel_pipeline_rust {

/**
 * Tail endpoint shim for an EventBase-local Rust application endpoint.
 *
 * The borrowed context and erased message are valid only for `onRead`. A Rust
 * endpoint that suspends must move the message and continuation into
 * `DeferredRead`. Read, activation, and write-ready may each return one IOBuf
 * chain; the shim writes it only after the Rust callback has released its
 * `&mut` endpoint borrow. Other lifecycle methods retain their no-context
 * contract.
 */
class RustTailEndpoint final {
 public:
  explicit RustTailEndpoint(rust::Box<RustTailEndpointOpaque> endpoint)
      : endpoint_{std::move(endpoint)} {}

  ~RustTailEndpoint() = default;

  RustTailEndpoint(const RustTailEndpoint&) = delete;
  RustTailEndpoint& operator=(const RustTailEndpoint&) = delete;
  RustTailEndpoint(RustTailEndpoint&&) = delete;
  RustTailEndpoint& operator=(RustTailEndpoint&&) = delete;

  // Attachment is one-shot for this endpoint's lifetime. The owner attaches
  // the completed pipeline before activation; removal clears the non-owning
  // pointer without permitting attachment to a different pipeline.
  [[nodiscard]] bool setPipeline(
      apache::thrift::fast_thrift::channel_pipeline::PipelineImpl*
          pipeline) noexcept {
    if (pipeline == nullptr || pipelineAttached_) {
      return false;
    }
    pipeline_ = pipeline;
    pipelineAttached_ = true;
    return true;
  }

  apache::thrift::fast_thrift::channel_pipeline::Result onRead(
      apache::thrift::fast_thrift::channel_pipeline::detail::ContextImpl& ctx,
      apache::thrift::fast_thrift::channel_pipeline::TypeErasedBox&&
          message) noexcept {
    using apache::thrift::fast_thrift::channel_pipeline::Result;
    try {
      if (message.empty()) {
        return Result::Error;
      }
      if (pipeline_ == nullptr) {
        pipeline_ = ctx.pipeline();
        pipelineAttached_ = true;
      } else if (pipeline_ != ctx.pipeline()) {
        return Result::Error;
      }
      if (rustCallbackActive_) {
        return Result::Error;
      }
      const auto lifecycleGeneration = lifecycleGeneration_;
      rustCallbackActive_ = true;
      auto outcome = [&] {
        CallbackContext context{ctx, message};
        return rust_tail_endpoint_on_read(*endpoint_, context, message);
      }();
      auto result = Result::Error;
      const bool current = isCurrent(lifecycleGeneration);
      if (outcome.message && !current) {
        logDroppedWrite(lifecycleGeneration);
      }
      if (current) {
        result = applyOutcome(
            std::move(outcome),
            lifecycleGeneration,
            [this](auto&& write) noexcept {
              return pipeline_->fireWrite(
                  apache::thrift::fast_thrift::channel_pipeline::erase_and_box(
                      std::forward<decltype(write)>(write)));
            });
      }
      rustCallbackActive_ = false;
      replayLatched();
      return removed_ || pipeline_ == nullptr || pipeline_->isClosed()
          ? Result::Error
          : result;
    } catch (...) {
      rustCallbackActive_ = false;
      replayLatched();
      return Result::Error;
    }
  }

  void onException(folly::exception_wrapper&& error) noexcept {
    if (rustCallbackActive_) {
      exceptionLatched_ = true;
      // Rust's hook is payload-free; the owning C++ callback still consumes
      // every exception payload synchronously, so no payload queue is needed.
      if (onException_) {
        onException_(std::move(error));
      }
      return;
    }
    invokeRust([this] { rust_tail_endpoint_on_exception(*endpoint_); });
    if (onException_) {
      onException_(std::move(error));
    }
    replayLatched();
  }

  void setOnException(
      folly::Function<void(folly::exception_wrapper&&) noexcept> callback) {
    onException_ = std::move(callback);
  }

  void onWriteReady() noexcept {
    if (!canCallRust()) {
      return;
    }
    if (rustCallbackActive_) {
      writeReadyLatched_ = true;
      return;
    }
    dispatchWriteReady();
    replayLatched();
  }

  void onPipelineActive() noexcept {
    if (rustCallbackActive_) {
      activeLatched_ = true;
      return;
    }
    dispatchPipelineActive();
    replayLatched();
  }

  void onPipelineInactive() noexcept {
    ++lifecycleGeneration_;
    activeLatched_ = false;
    writeReadyLatched_ = false;
    if (rustCallbackActive_) {
      inactiveLatched_ = true;
      return;
    }
    invokeRust([this] { rust_tail_endpoint_on_pipeline_inactive(*endpoint_); });
    replayLatched();
  }

  void handlerAdded() noexcept {
    if (rustCallbackActive_) {
      addedLatched_ = true;
      return;
    }
    invokeRust([this] { rust_tail_endpoint_handler_added(*endpoint_); });
    replayLatched();
  }

  void handlerRemoved() noexcept {
    ++lifecycleGeneration_;
    removed_ = true;
    activeLatched_ = false;
    writeReadyLatched_ = false;
    if (rustCallbackActive_) {
      removedLatched_ = true;
      pipeline_ = nullptr;
      return;
    }
    invokeRust([this] { rust_tail_endpoint_handler_removed(*endpoint_); });
    pipeline_ = nullptr;
    replayLatched();
  }

 private:
  using Result = apache::thrift::fast_thrift::channel_pipeline::Result;

  void dispatchPipelineActive() noexcept {
    activeLatched_ = false;
    if (!canCallRust()) {
      return;
    }
    const auto lifecycleGeneration = lifecycleGeneration_;
    rustCallbackActive_ = true;
    auto message = rust_tail_endpoint_on_pipeline_active(*endpoint_);
    const bool current = isCurrent(lifecycleGeneration);
    if (message && !current) {
      logDroppedWrite(lifecycleGeneration);
    } else if (message) {
      auto& pipeline = *pipeline_;
      const auto result = pipeline.fireWrite(
          apache::thrift::fast_thrift::channel_pipeline::erase_and_box(
              std::move(message)));
      if (result == Result::Error && isCurrent(lifecycleGeneration)) {
        pipeline.close();
      }
    }
    rustCallbackActive_ = false;
  }

  static Result decodeResult(int32_t result) noexcept {
    switch (result) {
      case static_cast<int32_t>(Result::Success):
        return Result::Success;
      case static_cast<int32_t>(Result::Backpressure):
        return Result::Backpressure;
      default:
        return Result::Error;
    }
  }

  static Result compose(Result first, Result second) noexcept {
    if (first == Result::Error || second == Result::Error) {
      return Result::Error;
    }
    if (first == Result::Backpressure || second == Result::Backpressure) {
      return Result::Backpressure;
    }
    return Result::Success;
  }

  bool canCallRust() const noexcept {
    return !removed_ && (pipeline_ == nullptr || !pipeline_->isClosed());
  }

  void logDroppedWrite(uint64_t lifecycleGeneration) const noexcept {
    if (pipeline_ == nullptr) {
      XLOG(ERR) << "dropping Rust tail write "
                << (pipelineAttached_ ? "after handler removal"
                                      : "before pipeline attachment");
    } else if (pipeline_->isClosed()) {
      XLOG(ERR) << "dropping Rust tail write after pipeline closure";
    } else if (lifecycleGeneration_ != lifecycleGeneration) {
      XLOG(ERR) << "dropping Rust tail write after pipeline lifecycle changed";
    } else {
      XLOG(ERR) << "dropping Rust tail write after endpoint removal";
    }
  }

  bool isCurrent(uint64_t lifecycleGeneration) const noexcept {
    return lifecycleGeneration_ == lifecycleGeneration && !removed_ &&
        pipeline_ != nullptr && !pipeline_->isClosed();
  }

  template <typename Callback>
  void invokeRust(Callback&& callback) noexcept {
    const bool wasRustCallbackActive = rustCallbackActive_;
    rustCallbackActive_ = true;
    callback();
    rustCallbackActive_ = wasRustCallbackActive;
  }

  template <typename Write>
  Result applyOutcome(
      FfiTailOutcome outcome,
      uint64_t lifecycleGeneration,
      Write&& write) noexcept {
    auto writeResult = Result::Success;
    if (outcome.message) {
      writeResult = write(std::move(outcome.message));
    }
    if (outcome.feedback_token != 0 && isCurrent(lifecycleGeneration)) {
      deliverFeedback(outcome.feedback_token, writeResult);
    }
    return compose(decodeResult(outcome.result), writeResult);
  }

  void deliverFeedback(uint64_t token, Result result) noexcept {
    rust_tail_endpoint_on_write_result(
        *endpoint_, token, static_cast<int32_t>(result));
  }

  void replayLifecycle() noexcept {
    // Each payload-free notification coalesces while Rust is borrowed. Replay
    // in pipeline order, with readiness handled only after terminal events.
    while (addedLatched_ || exceptionLatched_ || inactiveLatched_ ||
           removedLatched_) {
      if (addedLatched_) {
        addedLatched_ = false;
        invokeRust([this] { rust_tail_endpoint_handler_added(*endpoint_); });
        continue;
      }
      if (exceptionLatched_) {
        exceptionLatched_ = false;
        invokeRust([this] { rust_tail_endpoint_on_exception(*endpoint_); });
        continue;
      }
      if (inactiveLatched_) {
        inactiveLatched_ = false;
        invokeRust(
            [this] { rust_tail_endpoint_on_pipeline_inactive(*endpoint_); });
        continue;
      }
      removedLatched_ = false;
      activeLatched_ = false;
      invokeRust([this] { rust_tail_endpoint_handler_removed(*endpoint_); });
    }
  }

  void dispatchWriteReady() noexcept {
    do {
      writeReadyLatched_ = false;
      if (!canCallRust()) {
        return;
      }
      const auto lifecycleGeneration = lifecycleGeneration_;
      rustCallbackActive_ = true;
      auto outcome = rust_tail_endpoint_on_write_ready(*endpoint_);
      auto result = Result::Error;
      const bool current = isCurrent(lifecycleGeneration);
      if (outcome.message && !current) {
        logDroppedWrite(lifecycleGeneration);
      }
      if (current) {
        result = applyOutcome(
            std::move(outcome),
            lifecycleGeneration,
            [this](auto&& write) noexcept {
              return pipeline_->fireWrite(
                  apache::thrift::fast_thrift::channel_pipeline::erase_and_box(
                      std::forward<decltype(write)>(write)));
            });
      }
      if (result == Result::Error && isCurrent(lifecycleGeneration)) {
        pipeline_->close();
      }
      rustCallbackActive_ = false;
      const bool lifecyclePending = addedLatched_ || exceptionLatched_ ||
          inactiveLatched_ || removedLatched_ || activeLatched_;
      replayLifecycle();
      if (lifecyclePending || activeLatched_) {
        return;
      }
    } while (writeReadyLatched_);
  }

  void replayLatched() noexcept {
    while (true) {
      replayLifecycle();
      if (!canCallRust()) {
        activeLatched_ = false;
        writeReadyLatched_ = false;
        return;
      }
      if (activeLatched_) {
        dispatchPipelineActive();
        continue;
      }
      if (writeReadyLatched_) {
        dispatchWriteReady();
        continue;
      }
      return;
    }
  }

  rust::Box<RustTailEndpointOpaque> endpoint_;
  folly::Function<void(folly::exception_wrapper&&) noexcept> onException_;
  apache::thrift::fast_thrift::channel_pipeline::PipelineImpl* pipeline_{
      nullptr};
  bool pipelineAttached_{false};
  bool removed_{false};
  // Coalesce notifications while Rust is borrowed or its returned write and
  // feedback are applied. A lifecycle edge invalidates that callback's output.
  bool rustCallbackActive_{false};
  bool writeReadyLatched_{false};
  bool activeLatched_{false};
  bool addedLatched_{false};
  bool exceptionLatched_{false};
  bool inactiveLatched_{false};
  bool removedLatched_{false};
  uint64_t lifecycleGeneration_{0};
};

static_assert(
    apache::thrift::fast_thrift::channel_pipeline::TailEndpointHandler<
        RustTailEndpoint>);

} // namespace channel_pipeline_rust
