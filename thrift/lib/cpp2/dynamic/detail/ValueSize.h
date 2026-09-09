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
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace apache::thrift::dynamic::detail {

inline constexpr size_t kMaxThriftValueSize =
    std::numeric_limits<int32_t>::max();

[[noreturn]] inline void throwThriftValueSizeExceeded() {
  throw std::length_error("Thrift value size exceeds INT32_MAX");
}

inline void checkThriftValueSize(size_t size) {
  if (size > kMaxThriftValueSize) {
    throwThriftValueSizeExceeded();
  }
}

inline void checkThriftValueGrowth(size_t size, size_t growth) {
  if (size > kMaxThriftValueSize || growth > kMaxThriftValueSize - size) {
    throwThriftValueSizeExceeded();
  }
}

} // namespace apache::thrift::dynamic::detail
