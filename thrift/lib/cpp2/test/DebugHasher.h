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
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include <folly/Conv.h>
#include <folly/Range.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

namespace apache {
namespace thrift {
namespace hash {

class DebugHasher {
 public:
  DebugHasher() {
    queue_ = std::make_unique<folly::IOBufQueue>();
    appender_.reset(queue_.get(), 1 << 10);
    combine("[");
  }

  DebugHasher(const DebugHasher& other) = delete;
  DebugHasher& operator=(const DebugHasher& other) = delete;

  DebugHasher(DebugHasher&& other) = default;
  DebugHasher& operator=(DebugHasher&& other) = default;

  void finalize() {
    combine("]");
    auto queue = queue_->move();
    auto buf = queue->coalesce();
    result_ = {reinterpret_cast<const char*>(buf.data()), buf.size()};
  }

  std::string getResult() const { return result_.value(); }

  template <typename Value>
  typename std::enable_if<std::is_arithmetic<Value>::value, void>::type combine(
      const Value& value) {
    handlePrefix();
    combine(folly::to<std::string>(value));
  }

  void combine(const folly::IOBuf& value) {
    handlePrefix();
    for (const auto buf : value) {
      combine(buf);
    }
  }

  void combine(folly::StringPiece value) { combine(folly::ByteRange{value}); }

  void combine(folly::ByteRange value) { appender_.push(value); }

  void combine(const DebugHasher& other) {
    handlePrefix();
    combine(other.result_.value());
  }

  bool operator<(const DebugHasher& other) {
    return result_.value() < other.result_.value();
  }

 private:
  void handlePrefix() {
    if (empty_) {
      empty_ = false;
    } else {
      combine(",");
    }
  }

  bool empty_ = true;
  std::unique_ptr<folly::IOBufQueue> queue_{nullptr};
  folly::io::QueueAppender appender_{nullptr, 0};
  std::optional<std::string> result_;
};

} // namespace hash
} // namespace thrift
} // namespace apache
