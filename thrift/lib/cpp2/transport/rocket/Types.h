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

#include <functional>
#include <iosfwd>
#include <memory>
#include <utility>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>

namespace apache {
namespace thrift {
namespace rocket {

class StreamId {
 public:
  using underlying_type = uint32_t;

  constexpr StreamId() = default;
  constexpr explicit StreamId(uint32_t streamId) : streamId_(streamId) {}

  bool operator==(StreamId other) const {
    return streamId_ == other.streamId_;
  }

  bool operator!=(StreamId other) const {
    return streamId_ != other.streamId_;
  }

  StreamId& operator+=(uint32_t delta) {
    streamId_ += delta;
    return *this;
  }

  explicit operator uint32_t() const {
    return streamId_;
  }

 private:
  uint32_t streamId_{0};
};

std::ostream& operator<<(std::ostream& os, StreamId streamId);

class Payload {
 public:
  Payload() = default;

  Payload(const Payload&) = delete;
  Payload& operator=(const Payload&) = delete;

  Payload(Payload&&) = default;
  Payload& operator=(Payload&&) = default;

  ~Payload() = default;

  // Force user code to be explicit about the order in which metadata and data
  // are being passed by making non-default constructors private.
  static Payload makeFromData(std::unique_ptr<folly::IOBuf> data) {
    return Payload(std::move(data));
  }
  static Payload makeFromData(folly::ByteRange data) {
    return Payload(data);
  }
  static Payload makeFromMetadataAndData(
      std::unique_ptr<folly::IOBuf> metadata,
      std::unique_ptr<folly::IOBuf> data) {
    return Payload(std::move(metadata), std::move(data));
  }
  static Payload makeFromMetadataAndData(
      folly::ByteRange metadata,
      folly::ByteRange data) {
    return Payload(metadata, data);
  }
  static Payload makeCombined(
      std::unique_ptr<folly::IOBuf> buffer,
      size_t metadataSize) {
    return Payload(std::move(buffer), metadataSize);
  }

  std::unique_ptr<folly::IOBuf> data() && {
    DCHECK(buffer_ != nullptr);
    DCHECK_LE(metadataSize_, buffer_->computeChainDataLength());

    auto toTrim = metadataSize_;
    auto data = std::move(buffer_);
    while (toTrim > 0) {
      if (data->length() >= toTrim) {
        data->trimStart(toTrim);
        toTrim = 0;
      } else {
        toTrim -= data->length();
        data = data->pop();
      }
    }
    return data;
  }

  bool hasNonemptyMetadata() const noexcept {
    return metadataSize_;
  }

  size_t metadataSize() const noexcept {
    return metadataSize_;
  }

  size_t dataSize() const noexcept {
    auto length = buffer_->computeChainDataLength();
    DCHECK(length >= metadataSize_);
    return length - metadataSize_;
  }

  const folly::IOBuf* buffer() const& {
    return buffer_.get();
  }

  std::unique_ptr<folly::IOBuf> buffer() && {
    return std::move(buffer_);
  }

  size_t metadataAndDataSize() const {
    return buffer_->computeChainDataLength();
  }

  size_t serializedSize() const {
    constexpr size_t kBytesForMetadataSize = 3;
    return buffer_->computeChainDataLength() +
        (hasNonemptyMetadata() ? kBytesForMetadataSize : 0ull);
  }

  void append(Payload&& other);

 private:
  size_t metadataSize_{0};
  std::unique_ptr<folly::IOBuf> buffer_;

  explicit Payload(std::unique_ptr<folly::IOBuf> data)
      : buffer_(data ? std::move(data) : std::make_unique<folly::IOBuf>()) {}
  Payload(
      std::unique_ptr<folly::IOBuf> metadata,
      std::unique_ptr<folly::IOBuf> data) {
    if (metadata) {
      metadataSize_ = metadata->computeChainDataLength();
      if (data) {
        metadata->prependChain(std::move(data));
      }
      buffer_ = std::move(metadata);
    } else if (data) {
      buffer_ = std::move(data);
    } else {
      buffer_ = std::make_unique<folly::IOBuf>();
    }
  }
  Payload(std::unique_ptr<folly::IOBuf> buffer, size_t metadataSize)
      : metadataSize_(metadataSize),
        buffer_(buffer ? std::move(buffer) : std::make_unique<folly::IOBuf>()) {
  }

  explicit Payload(folly::ByteRange data)
      : buffer_(folly::IOBuf::copyBuffer(data)) {}
  Payload(folly::ByteRange metadata, folly::ByteRange data)
      : metadataSize_(metadata.size()),
        buffer_(folly::IOBuf::copyBuffer(metadata)) {
    buffer_->prependChain(folly::IOBuf::copyBuffer(data));
  }
};

} // namespace rocket
} // namespace thrift
} // namespace apache

namespace std {
template <>
struct hash<apache::thrift::rocket::StreamId> {
  size_t operator()(apache::thrift::rocket::StreamId streamId) const {
    return hash<uint32_t>()(static_cast<uint32_t>(streamId));
  }
};
} // namespace std
