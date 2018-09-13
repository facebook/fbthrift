/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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

#include <folly/Optional.h>
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
  static Payload makeFromData(folly::IOBuf&& data) noexcept {
    return Payload(std::move(data));
  }
  static Payload makeFromData(folly::ByteRange data) {
    return Payload(data);
  }
  static Payload makeFromMetadataAndData(
      folly::IOBuf&& metadata,
      folly::IOBuf&& data) noexcept {
    return Payload(std::move(metadata), std::move(data));
  }
  static Payload makeFromMetadataAndData(
      folly::Optional<folly::IOBuf>&& metadata,
      folly::IOBuf&& data) noexcept {
    return Payload(std::move(metadata), std::move(data));
  }
  static Payload makeFromMetadataAndData(
      folly::ByteRange metadata,
      folly::ByteRange data) {
    return Payload(metadata, data);
  }

  const folly::IOBuf& data() const& {
    return data_;
  }
  folly::IOBuf& data() & {
    return data_;
  }
  folly::IOBuf data() && {
    return std::move(data_);
  }

  // The API for Payload is a bit leaky: users can access metadata() directly
  // and have the potential to fill metadata() with an empty IOBuf. So, when
  // serializing data onto the wire, we need to check whether there is actually
  // any non-empty metadata to send. (On the read path, we are also careful not
  // to fill metadata() with an empty IOBuf.)
  bool hasNonemptyMetadata() const noexcept {
    return metadata_ && !metadata_->empty();
  }

  const folly::Optional<folly::IOBuf>& metadata() const& {
    return metadata_;
  }
  folly::Optional<folly::IOBuf>& metadata() & {
    return metadata_;
  }
  folly::Optional<folly::IOBuf> metadata() && {
    return std::move(metadata_);
  }

  size_t metadataAndDataSize() const {
    return data_.computeChainDataLength() +
        (hasNonemptyMetadata() ? metadata_->computeChainDataLength() : 0ull);
  }

  size_t serializedSize() const {
    constexpr size_t kBytesForMetadataSize = 3;
    return data_.computeChainDataLength() +
        (hasNonemptyMetadata()
             ? (kBytesForMetadataSize + metadata_->computeChainDataLength())
             : 0ull);
  }

  void append(Payload&& other);

 private:
  folly::IOBuf data_;
  // Possible optimization: if payloads will commonly have metadata, then we can
  // maintain a single IOBuf containing both metadata and data, which are always
  // contiguous in memory.
  folly::Optional<folly::IOBuf> metadata_;

  explicit Payload(folly::IOBuf&& data) noexcept : data_(std::move(data)) {}
  Payload(folly::IOBuf&& metadata, folly::IOBuf&& data) noexcept
      : data_(std::move(data)), metadata_(std::move(metadata)) {}
  Payload(
      folly::Optional<folly::IOBuf>&& metadata,
      folly::IOBuf&& data) noexcept
      : data_(std::move(data)), metadata_(std::move(metadata)) {}

  explicit Payload(folly::ByteRange data)
      : data_(folly::IOBuf(folly::IOBuf::CopyBufferOp(), data)) {}
  Payload(folly::ByteRange metadata, folly::ByteRange data)
      : data_(folly::IOBuf(folly::IOBuf::CopyBufferOp(), data)),
        metadata_(folly::in_place, folly::IOBuf::CopyBufferOp(), metadata) {}
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
