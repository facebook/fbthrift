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

namespace apache::thrift::fast_thrift::frame::write {

// Writes the 3-byte big-endian length field that starts a frame. The same
// field also carries the metadata length.
//
// Only the low 24 bits go out. A longer length loses its top bits with no
// error, so the caller has to keep it under 0xFFFFFF.
//
// This is not in a detail namespace because read-side tests and benches call
// it to build their input. The reader, read::detail::readFrameOrMetadataSize,
// has one user inside its own file, so it stays private.
inline void writeFrameLength(uint8_t* buf, size_t length) noexcept {
  buf[0] = static_cast<uint8_t>((length >> 16) & 0xFF);
  buf[1] = static_cast<uint8_t>((length >> 8) & 0xFF);
  buf[2] = static_cast<uint8_t>(length & 0xFF);
}

} // namespace apache::thrift::fast_thrift::frame::write
