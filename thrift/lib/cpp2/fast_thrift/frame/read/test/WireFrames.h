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

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameLength.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameWriter.h>

namespace apache::thrift::fast_thrift::frame::read {

// One frame as it arrives on the wire: 3 bytes of length, then the frame.
// frame::write builds the frame but does not write the length, so this adds it.
// Takes the same arguments as frame::write::serialize.
template <typename Header, typename... Payload>
std::vector<uint8_t> serializeFrame(
    const Header& header, Payload&&... payload) {
  const std::unique_ptr<folly::IOBuf> body =
      write::serialize(header, std::forward<Payload>(payload)...);
  const folly::ByteRange frame = body->coalesce();
  std::vector<uint8_t> bytes(kMetadataLengthSize + frame.size());
  write::writeFrameLength(bytes.data(), frame.size());
  std::memcpy(bytes.data() + kMetadataLengthSize, frame.data(), frame.size());
  return bytes;
}

} // namespace apache::thrift::fast_thrift::frame::read
