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

#include <atomic>
#include <cstdint>
#include <optional>

namespace apache::thrift::fast_thrift::thrift::detail {

enum class HandlerState : uint8_t {
  AwaitingDispatch,
  Running,
  Completed,
};

class EvbCancellationSlot {
 public:
  explicit EvbCancellationSlot(HandlerState state) noexcept : state_(state) {}

  bool tryComplete() noexcept {
    if (state_ == HandlerState::Completed) {
      return false;
    }
    state_ = HandlerState::Completed;
    return true;
  }

  void markHandlerStarted() noexcept {
    if (state_ == HandlerState::AwaitingDispatch) {
      state_ = HandlerState::Running;
    }
  }

  HandlerState state() const noexcept { return state_; }

 private:
  HandlerState state_;
};

class CpuCancellationSlot {
 public:
  explicit CpuCancellationSlot(HandlerState state) noexcept : state_(state) {}

  bool tryComplete() noexcept {
    auto state = state_.load(std::memory_order_acquire);
    while (state != HandlerState::Completed) {
      if (state_.compare_exchange_weak(
              state,
              HandlerState::Completed,
              std::memory_order_acq_rel,
              std::memory_order_acquire)) {
        return true;
      }
    }
    return false;
  }

  void markHandlerStarted() noexcept {
    auto expected = HandlerState::AwaitingDispatch;
    state_.compare_exchange_strong(
        expected,
        HandlerState::Running,
        std::memory_order_release,
        std::memory_order_relaxed);
  }

  HandlerState state() const noexcept {
    return state_.load(std::memory_order_acquire);
  }

 private:
  std::atomic<HandlerState> state_;
};

// Selects the synchronization policy once per request. The optional keeps the
// CPU slot, including construction of its atomic, off the EventBase-only path.
class CancellationSlot {
 public:
  CancellationSlot(bool useCpuSlot, HandlerState state) noexcept : evb_(state) {
    if (useCpuSlot) {
      cpu_.emplace(state);
    }
  }

  bool tryComplete() noexcept {
    return cpu_ ? cpu_->tryComplete() : evb_.tryComplete();
  }

  void markHandlerStarted() noexcept {
    if (cpu_) {
      cpu_->markHandlerStarted();
    } else {
      evb_.markHandlerStarted();
    }
  }

  HandlerState state() const noexcept {
    return cpu_ ? cpu_->state() : evb_.state();
  }

 private:
  EvbCancellationSlot evb_;
  std::optional<CpuCancellationSlot> cpu_;
};

} // namespace apache::thrift::fast_thrift::thrift::detail
