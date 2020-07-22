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

#include <array>
#include <cstdint>
#include <exception>
#include <string>

#include <folly/Memory.h>
#include <folly/io/Cursor.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/FrameType.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace detail {
[[noreturn]] void throwUnexpectedFrameType(uint8_t frameType);
} // namespace detail

inline StreamId readStreamId(folly::io::Cursor& cursor) {
  return StreamId{cursor.readBE<StreamId::underlying_type>()};
}

inline size_t readFrameOrMetadataSize(folly::io::Cursor& cursor) {
  std::array<uint8_t, 3> bytes;
  cursor.pull(bytes.data(), bytes.size());

  return (static_cast<size_t>(bytes[0]) << 16) |
      (static_cast<size_t>(bytes[1]) << 8) | static_cast<size_t>(bytes[2]);
}

inline std::pair<uint8_t, Flags> readFrameTypeAndFlagsUnsafe(
    folly::io::Cursor& cursor) {
  const uint16_t frameTypeAndFlags = cursor.readBE<uint16_t>();
  const uint8_t frameType = frameTypeAndFlags >> Flags::frameTypeOffset();
  const Flags flags(frameTypeAndFlags & Flags::mask());
  return {frameType, flags};
}

inline std::pair<FrameType, Flags> readFrameTypeAndFlags(
    folly::io::Cursor& cursor) {
  auto const pair = readFrameTypeAndFlagsUnsafe(cursor);
  switch (static_cast<FrameType>(pair.first)) {
    case FrameType::SETUP:
    case FrameType::REQUEST_RESPONSE:
    case FrameType::REQUEST_FNF:
    case FrameType::REQUEST_STREAM:
    case FrameType::REQUEST_CHANNEL:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::PAYLOAD:
    case FrameType::ERROR:
    case FrameType::METADATA_PUSH:
    case FrameType::KEEPALIVE:
    case FrameType::EXT:
      return {static_cast<FrameType>(pair.first), pair.second};

    default:
      detail::throwUnexpectedFrameType(pair.first);
  }
}

inline ExtFrameType readExtFrameType(folly::io::Cursor& cursor) {
  const auto extFrameType = cursor.readBE<uint32_t>();
  switch (static_cast<ExtFrameType>(extFrameType)) {
    case ExtFrameType::HEADERS_PUSH:
    case ExtFrameType::ALIGNED_PAGE:
    case ExtFrameType::INTERACTION_TERMINATE:
      return static_cast<ExtFrameType>(extFrameType);
    default:
      return ExtFrameType::UNKNOWN;
  }
}

inline bool
alignTo4k(folly::IOBuf& buffer, size_t startOffset, size_t frameSize) {
  constexpr int kPageSize = 4096;
  size_t padding = kPageSize - startOffset % kPageSize;
  size_t allocationSize = padding + std::max(buffer.length(), frameSize);

  void* rawbuf = folly::aligned_malloc(allocationSize, kPageSize);
  if (!rawbuf) {
    LOG(ERROR) << "Allocating : " << kPageSize
               << " aligned memory of size: " << allocationSize << " failed!";
    return false;
  }

  auto iobuf = folly::IOBuf::takeOwnership(
      static_cast<void*>(rawbuf),
      allocationSize,
      allocationSize,
      [](void* p, void*) { folly::aligned_free(p); });

  iobuf->trimStart(padding);
  iobuf->trimEnd(allocationSize - buffer.length() - padding);
  memcpy(iobuf->writableData(), buffer.writableData(), buffer.length());
  buffer = *std::move(iobuf);
  return true;
}

} // namespace rocket
} // namespace thrift
} // namespace apache
