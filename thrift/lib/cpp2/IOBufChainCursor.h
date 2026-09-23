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
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

#include <folly/Range.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/lang/Bits.h>
#include <thrift/lib/cpp2/IOBufChain.h>

namespace apache::thrift::io {

class IOBufChainCursor {
 public:
  IOBufChainCursor() noexcept = default;
  explicit IOBufChainCursor(const apache::thrift::IOBufChain& chain) noexcept;

  void reset(const apache::thrift::IOBufChain* chain = nullptr) noexcept;

  template <typename T>
  T read();

  template <typename T>
  T readBE();

  const uint8_t* data() const noexcept {
    return isAtEnd() ? nullptr : current_->data() + offset_;
  }
  size_t length() const noexcept {
    return isAtEnd() ? 0 : current_->length() - offset_;
  }
  folly::ByteRange peekBytes() noexcept;
  bool canAdvance(size_t length) const noexcept {
    return length <= totalLength() - position_;
  }

  size_t pullAtMost(void* destination, size_t length) {
    if (FOLLY_UNLIKELY(length == 0)) {
      return 0;
    }
    if (FOLLY_LIKELY(length <= this->length())) {
      std::memcpy(destination, data(), length);
      advanceWithinCurrentBuffer(length);
      return length;
    }
    return pullAtMostSlow(destination, length);
  }
  void pull(void* destination, size_t length) {
    if (FOLLY_UNLIKELY(length == 0)) {
      return;
    }
    if (FOLLY_LIKELY(length <= this->length())) {
      std::memcpy(destination, data(), length);
      advanceWithinCurrentBuffer(length);
      return;
    }
    pullSlow(destination, length);
  }
  void skip(size_t length) {
    if (FOLLY_UNLIKELY(length == 0)) {
      return;
    }
    if (FOLLY_LIKELY(length <= this->length())) {
      advanceWithinCurrentBuffer(length);
      return;
    }
    skipSlow(length);
  }
  void skipNoAdvance(size_t length) noexcept {
    DCHECK_LE(length, this->length());
    offset_ += length;
    position_ += length;
  }
  std::string readFixedString(size_t length);

  void clone(
      apache::thrift::IOBufChain& destination,
      size_t length,
      folly::io::CloneOwnership ownership = folly::io::CloneOwnership::Shared);
  void clone(
      std::unique_ptr<folly::IOBuf>& destination,
      size_t length,
      folly::io::CloneOwnership ownership = folly::io::CloneOwnership::Shared);
  void clone(
      folly::IOBuf& destination,
      size_t length,
      folly::io::CloneOwnership ownership = folly::io::CloneOwnership::Shared);

  size_t getCurrentPosition() const noexcept { return position_; }
  bool isAtEnd() const noexcept { return position_ == totalLength(); }

  size_t operator-(const IOBufChainCursor& other) const noexcept;

 private:
  size_t pullAtMostSlow(void* destination, size_t length);
  void pullSlow(void* destination, size_t length);
  void skipSlow(size_t length);
  void advanceWithinCurrentBuffer(size_t length) noexcept {
    DCHECK_LE(length, this->length());
    offset_ += length;
    position_ += length;
    if (offset_ == current_->length()) {
      advancePastEmptyBuffers();
    }
  }
  void advancePastEmptyBuffers() noexcept;
  size_t totalLength() const noexcept {
    return chain_ ? chain_->chainLength() : 0;
  }

  const apache::thrift::IOBufChain* chain_{nullptr};
  apache::thrift::IOBufChain::const_iterator current_;
  size_t offset_{0};
  size_t position_{0};
};

template <typename T>
T IOBufChainCursor::read() {
  static_assert(std::is_trivially_copyable_v<T>);
  if (FOLLY_LIKELY(sizeof(T) <= length())) {
    const auto value = folly::loadUnaligned<T>(data());
    advanceWithinCurrentBuffer(sizeof(T));
    return value;
  }
  T value;
  pullSlow(&value, sizeof(value));
  return value;
}

template <typename T>
T IOBufChainCursor::readBE() {
  return folly::Endian::big(read<T>());
}

} // namespace apache::thrift::io
