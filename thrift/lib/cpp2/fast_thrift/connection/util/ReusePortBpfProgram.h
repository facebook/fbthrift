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
#include <memory>

#include <folly/net/NetworkSocket.h>

namespace apache::thrift::fast_thrift::connection {

// Process-owned eBPF selector shared by every socket in a reuseport group.
class ReusePortBpfProgram {
 public:
  static std::shared_ptr<const ReusePortBpfProgram> createRoundRobin(
      uint32_t groupSize) noexcept;

  ~ReusePortBpfProgram();

  ReusePortBpfProgram(const ReusePortBpfProgram&) = delete;
  ReusePortBpfProgram& operator=(const ReusePortBpfProgram&) = delete;
  ReusePortBpfProgram(ReusePortBpfProgram&&) = delete;
  ReusePortBpfProgram& operator=(ReusePortBpfProgram&&) = delete;

  [[nodiscard]] bool attach(folly::NetworkSocket socket) const noexcept;

 private:
  explicit ReusePortBpfProgram(int fd) noexcept : fd_(fd) {}

  int fd_;
};

} // namespace apache::thrift::fast_thrift::connection
