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

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/transport/Parser.h>

namespace apache::thrift::fast_thrift::frame::read {

/**
 * Parses RSocket frames. A request gets two things the ordinary parser does
 * not give it. None of it is written yet. This is the skeleton, and every test
 * for it is disabled.
 *
 *   struct Request {
 *     1: binary blob;   // alone in its own buffer, on a 16-byte boundary
 *     2: i64 offset;
 *   }
 *
 * Thrift does not copy a binary field, it hands over the bytes that arrived,
 * so keeping `blob` keeps those bytes and nothing else.
 *
 * The alignment holds under two conditions, and the parser checks neither:
 *   - the blob is the first field of the request;
 *   - the request is serialized with the binary protocol.
 *
 * Break either one and the parser still works, it just stops aligning.
 *
 * A request comes out as a chain with one piece per field. First the header,
 * then the metadata if there is any, then the data. Every other frame type
 * comes out as one run of bytes, and the pieces can break anywhere.
 *
 * To place the bytes the parser has to allocate the buffer itself, so it
 * cannot read into a buffer that someone else allocated. That is why it
 * implements Parser and not MovableBufferParser.
 */
class AlignedParser {
 public:
  void getReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;

  template <typename Sink>
  channel_pipeline::Result consume(size_t len, Sink&& sink) noexcept;

  void setIOBufFactory(folly::IOBufFactory* factory) noexcept;

  void reset() noexcept;

 private:
  static constexpr size_t kAlignment = 16;

  // Binary protocol puts 10 bytes in front of the blob's own bytes: 3 for the
  // field header of the struct Thrift builds around the RPC arguments, 3 for
  // the field header of the blob, and 4 for the blob length.
  static constexpr size_t kBytesBeforeFirstField = 3 + 3 + 4;

  static constexpr size_t kDataBufferPadding =
      kAlignment - kBytesBeforeFirstField;

  folly::IOBufFactory* bufFactory_{nullptr};
};

template <typename Sink>
channel_pipeline::Result AlignedParser::consume(
    size_t /*len*/, Sink&& /*sink*/) noexcept {
  return channel_pipeline::Result::Success;
}

static_assert(transport::Parser<AlignedParser>);
static_assert(!transport::MovableBufferParser<AlignedParser>);

} // namespace apache::thrift::fast_thrift::frame::read
