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

#include <array>
#include <cstdint>
#include <exception>
#include <string>

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

inline std::pair<FrameType, Flags> readFrameTypeAndFlags(
    folly::io::Cursor& cursor) {
  const uint16_t frameTypeAndFlags = cursor.readBE<uint16_t>();
  const uint8_t frameType = frameTypeAndFlags >> Flags::frameTypeOffset();
  const Flags flags(frameTypeAndFlags & Flags::mask());

  switch (static_cast<FrameType>(frameType)) {
    case FrameType::SETUP:
    case FrameType::REQUEST_RESPONSE:
    case FrameType::REQUEST_FNF:
    case FrameType::REQUEST_STREAM:
    case FrameType::REQUEST_CHANNEL:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::PAYLOAD:
    case FrameType::ERROR:
    case FrameType::KEEPALIVE:
      return {static_cast<FrameType>(frameType), flags};

    default:
      detail::throwUnexpectedFrameType(frameType);
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
