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

#include <cstdint>
#include <type_traits>

#include <folly/Hash.h>
#include <folly/Range.h>
#include <folly/io/IOBuf.h>

namespace apache {
namespace thrift {
namespace hash {

class StdHasher {
 public:
  std::size_t getResult() const { return result_; }

  void finalize() {}

  void combine(bool value) { combinePrimitive(value); }

  void combine(std::int8_t value) { combinePrimitive(value); }

  void combine(std::int16_t value) { combinePrimitive(value); }

  void combine(std::int32_t value) { combinePrimitive(value); }

  void combine(std::int64_t value) { combinePrimitive(value); }

  void combine(float value) { combinePrimitive(value); }

  void combine(double value) { combinePrimitive(value); }

  void combine(const folly::IOBuf& value) {
    for (const auto buf : value) {
      combine(buf);
    }
  }

  void combine(folly::ByteRange value) {
    result_ = folly::hash::hash_combine(
        folly::hash::hash_range(value.begin(), value.end()), result_);
  }

  void combine(const StdHasher& other) { combinePrimitive(other.result_); }

  bool operator<(const StdHasher& other) const {
    return result_ < other.result_;
  }

 private:
  template <typename Value>
  void combinePrimitive(Value value) {
    static_assert(
        std::is_arithmetic_v<Value>, "Only for primitive arithmetic types");
    result_ = folly::hash::hash_combine(value, result_);
  }

  std::size_t result_ = 0;
};

} // namespace hash
} // namespace thrift
} // namespace apache
