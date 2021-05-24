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

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

struct SerializedRequest {
  explicit SerializedRequest(std::unique_ptr<folly::IOBuf> buffer_)
      : buffer(std::move(buffer_)) {}

  std::unique_ptr<folly::IOBuf> buffer;
};

class SerializedCompressedRequest {
 public:
  explicit SerializedCompressedRequest(
      std::unique_ptr<folly::IOBuf> buffer,
      CompressionAlgorithm compression = CompressionAlgorithm::NONE)
      : buffer_(std::move(buffer)), compression_(compression) {}

  explicit SerializedCompressedRequest(SerializedRequest&& request)
      : buffer_(std::move(request.buffer)),
        compression_(CompressionAlgorithm::NONE) {}

  SerializedRequest uncompress() &&;

  SerializedCompressedRequest clone() const;

  CompressionAlgorithm getCompressionAlgorithm() const { return compression_; }

 private:
  std::unique_ptr<folly::IOBuf> buffer_;
  CompressionAlgorithm compression_;
};

struct LegacySerializedRequest {
  /* implicit */ LegacySerializedRequest(std::unique_ptr<folly::IOBuf> buffer_)
      : buffer(std::move(buffer_)) {}

  LegacySerializedRequest(
      uint16_t protocolId,
      folly::StringPiece methodName,
      SerializedRequest&& serializedRequest);

  LegacySerializedRequest(
      uint16_t protocolId,
      int32_t seqid,
      folly::StringPiece methodName,
      SerializedRequest&& serializedRequest);

  std::unique_ptr<folly::IOBuf> buffer;
};

struct ResponsePayload {
  ResponsePayload() = default;
  /* implicit */ ResponsePayload(std::unique_ptr<folly::IOBuf> buffer)
      : buffer_(std::move(buffer)) {}

  std::unique_ptr<folly::IOBuf> buffer() && { return std::move(buffer_); }

  const folly::IOBuf* buffer() const& { return buffer_.get(); }

  explicit operator bool() const { return static_cast<bool>(buffer_); }
  std::size_t length() const { return buffer_->computeChainDataLength(); }

 private:
  std::unique_ptr<folly::IOBuf> buffer_;
};

struct SerializedResponse {
  explicit SerializedResponse(std::unique_ptr<folly::IOBuf> buffer_)
      : buffer(std::move(buffer_)) {}

  std::unique_ptr<folly::IOBuf> buffer;
};

struct LegacySerializedResponse {
  explicit LegacySerializedResponse(
      std::unique_ptr<folly::IOBuf> buffer_ = std::unique_ptr<folly::IOBuf>{})
      : buffer(std::move(buffer_)) {}

  LegacySerializedResponse(
      uint16_t protocolId,
      folly::StringPiece methodName,
      SerializedResponse&& serializedResponse);

  LegacySerializedResponse(
      uint16_t protocolId,
      int32_t seqid,
      folly::StringPiece methodName,
      SerializedResponse&& serializedResponse);

  LegacySerializedResponse(
      uint16_t protocolId,
      folly::StringPiece methodName,
      const TApplicationException& ex);

  LegacySerializedResponse(
      uint16_t protocolId,
      int32_t seqid,
      folly::StringPiece methodName,
      const TApplicationException& ex);

  LegacySerializedResponse(
      uint16_t protocolId,
      int32_t seqid,
      MessageType mtype,
      folly::StringPiece methodName,
      SerializedResponse&& serializedResponse);

  std::unique_ptr<folly::IOBuf> buffer;
};

} // namespace thrift
} // namespace apache
