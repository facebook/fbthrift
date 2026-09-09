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

#include <thrift/lib/cpp2/dynamic/detail/ValueSize.h>
#include <thrift/lib/cpp2/dynamic/fwd.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <memory>
#include <memory_resource>
#include <utility>

namespace apache::thrift::dynamic {

/**
 * A Binary type backed by folly::IOBuf for efficient buffer management.
 * Data is exposed using folly::io::Cursor for read access.
 */
class Binary final {
 public:
  // Constructors
  Binary() = default;
  explicit Binary(std::pmr::memory_resource* mr) : mr_(mr) {}
  explicit Binary(
      std::unique_ptr<folly::IOBuf> buf,
      std::pmr::memory_resource* mr = nullptr)
      : data_(std::move(buf)),
        mr_(mr),
        size_(data_ ? data_->computeChainDataLength() : 0) {
    detail::checkThriftValueSize(size_);
  }

  // Copy and move
  Binary(const Binary& other);
  Binary(Binary&& other) noexcept
      : data_(std::move(other.data_)),
        mr_(other.mr_),
        size_(std::exchange(other.size_, 0)) {}
  Binary& operator=(const Binary& other);
  Binary& operator=(Binary&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      mr_ = other.mr_;
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }
  ~Binary() = default;

  // Create cursor for reading
  folly::io::Cursor cursor() const;
  // No RWCursor access because it doesn't support memory_resource.

  // Size and empty checks
  size_t computeChainDataLength() const { return size_; }
  bool empty() const { return size_ == 0; }

  // Clone with memory resource
  Binary clone(std::pmr::memory_resource* mr = nullptr) const;

  // Comparison
  friend bool operator==(const Binary& lhs, const Binary& rhs) noexcept;

 private:
  std::unique_ptr<folly::IOBuf> data_;
  std::pmr::memory_resource* mr_ = nullptr;
  size_t size_ = 0;

  template <typename ProtocolWriter>
  friend void serialize(ProtocolWriter& writer, const Binary& binary);
  friend struct detail::DatumHash;
  friend struct detail::DatumEqual;
};

} // namespace apache::thrift::dynamic
