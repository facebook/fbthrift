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

#include <concepts>
#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>

namespace apache::thrift::fast_thrift::rocket {

template <typename T>
concept RocketTransportLifecycleOwner =
    requires(T& owner, folly::exception_wrapper&& error) {
      { owner.start() } noexcept -> std::same_as<void>;
      { owner.disconnect(std::move(error)) } noexcept -> std::same_as<void>;
    };

/** Cold-path ownership for a transport-specific Rocket edge bundle. */
class RocketTransportLifecycle final {
 public:
  RocketTransportLifecycle() noexcept = default;
  RocketTransportLifecycle(const RocketTransportLifecycle&) = delete;
  RocketTransportLifecycle& operator=(const RocketTransportLifecycle&) = delete;

  RocketTransportLifecycle(RocketTransportLifecycle&& other) noexcept {
    moveFrom(other);
  }

  RocketTransportLifecycle& operator=(
      RocketTransportLifecycle&& other) noexcept {
    if (this != &other) {
      reset();
      moveFrom(other);
    }
    return *this;
  }

  ~RocketTransportLifecycle() { reset(); }

  template <RocketTransportLifecycleOwner T>
  static RocketTransportLifecycle own(std::unique_ptr<T> owner) noexcept {
    RocketTransportLifecycle result;
    result.owner_ = owner.release();
    result.start_ = [](void* value) noexcept {
      static_cast<T*>(value)->start();
    };
    result.disconnect_ = [](void* value,
                            folly::exception_wrapper&& error) noexcept {
      static_cast<T*>(value)->disconnect(std::move(error));
    };
    result.destroy_ = [](void* value) noexcept {
      delete static_cast<T*>(value);
    };
    return result;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return owner_ != nullptr;
  }

  void start() noexcept {
    if (owner_ != nullptr) {
      start_(owner_);
    }
  }

  void disconnect(folly::exception_wrapper&& error = {}) noexcept {
    if (owner_ != nullptr) {
      disconnect_(owner_, std::move(error));
    }
  }

  void reset() noexcept {
    if (owner_ != nullptr) {
      destroy_(owner_);
    }
    owner_ = nullptr;
    start_ = nullptr;
    disconnect_ = nullptr;
    destroy_ = nullptr;
  }

 private:
  using Start = void (*)(void*) noexcept;
  using Disconnect = void (*)(void*, folly::exception_wrapper&&) noexcept;
  using Destroy = void (*)(void*) noexcept;

  void moveFrom(RocketTransportLifecycle& other) noexcept {
    owner_ = std::exchange(other.owner_, nullptr);
    start_ = std::exchange(other.start_, nullptr);
    disconnect_ = std::exchange(other.disconnect_, nullptr);
    destroy_ = std::exchange(other.destroy_, nullptr);
  }

  void* owner_{nullptr};
  Start start_{nullptr};
  Disconnect disconnect_{nullptr};
  Destroy destroy_{nullptr};
};

} // namespace apache::thrift::fast_thrift::rocket
