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

#include <cstddef>

#include <folly/lang/SafeAssert.h>

namespace apache::thrift::fast_thrift::thrift {

class ReadBackpressureArbiter {
 public:
  void acquire() noexcept { ++holders_; }

  [[nodiscard]] bool release() noexcept {
    FOLLY_SAFE_DCHECK(holders_ > 0u, "unbalanced read-backpressure release");
    return --holders_ == 0;
  }

  size_t holderCount() const noexcept { return holders_; }

 private:
  size_t holders_{0};
};

} // namespace apache::thrift::fast_thrift::thrift
