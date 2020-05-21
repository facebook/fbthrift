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

namespace apache {
namespace thrift {

struct SerializedRequest {
  explicit SerializedRequest(std::unique_ptr<folly::IOBuf> buffer_)
      : buffer(std::move(buffer_)) {}

  std::unique_ptr<folly::IOBuf> buffer;
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

struct SerializedResponse {
  explicit SerializedResponse(std::unique_ptr<folly::IOBuf> buffer_)
      : buffer(std::move(buffer_)) {}

  std::unique_ptr<folly::IOBuf> buffer;
};

struct LegacySerializedResponse {
  /* implicit */ LegacySerializedResponse(std::unique_ptr<folly::IOBuf> buffer_)
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

  std::unique_ptr<folly::IOBuf> buffer;
};

} // namespace thrift
} // namespace apache
