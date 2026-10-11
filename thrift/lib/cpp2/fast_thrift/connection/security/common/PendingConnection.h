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

#include <glog/logging.h>

namespace apache::thrift::fast_thrift::connection::security {

struct PendingConnectionState {
  explicit PendingConnectionState(uint32_t maxPendingConnections) noexcept
      : maxPendingConnections(maxPendingConnections) {}

  bool atCapacity() const noexcept {
    return maxPendingConnections != 0 &&
        pendingConnections >= maxPendingConnections;
  }

  uint32_t maxPendingConnections;
  uint32_t pendingConnections{0};
};

class PendingConnectionPermit {
 public:
  PendingConnectionPermit() = default;

  explicit PendingConnectionPermit(PendingConnectionState& state) noexcept
      : state_(&state) {
    DCHECK(!state_->atCapacity());
    ++state_->pendingConnections;
  }

  ~PendingConnectionPermit() { reset(); }

  PendingConnectionPermit(const PendingConnectionPermit&) = delete;
  PendingConnectionPermit& operator=(const PendingConnectionPermit&) = delete;

  PendingConnectionPermit(PendingConnectionPermit&& other) noexcept
      : state_(std::exchange(other.state_, nullptr)) {}

  PendingConnectionPermit& operator=(PendingConnectionPermit&& other) noexcept {
    if (this != &other) {
      reset();
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }

 private:
  void reset() noexcept {
    if (state_ != nullptr) {
      DCHECK_GT(state_->pendingConnections, 0);
      --state_->pendingConnections;
      state_ = nullptr;
    }
  }

  PendingConnectionState* state_{nullptr};
};

} // namespace apache::thrift::fast_thrift::connection::security
