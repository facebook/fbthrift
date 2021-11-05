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

  template <
      typename Value,
      std::enable_if_t<std::is_arithmetic_v<Value>, int> = 0>
  void combine(Value value) {
    result_ = folly::hash::hash_combine(value, result_);
  }

  void combine(const folly::IOBuf& value) {
    folly::IOBufHash hash;
    combine(hash(value));
  }

  void combine(folly::ByteRange value) {
    result_ = folly::hash::hash_combine(
        folly::hash::hash_range(value.begin(), value.end()), result_);
  }

  void combine(const StdHasher& other) { combine(other.result_); }

  bool operator<(const StdHasher& other) const {
    return result_ < other.result_;
  }

 private:
  std::size_t result_;
};

} // namespace hash
} // namespace thrift
} // namespace apache
