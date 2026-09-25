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

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/lang/Hint.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/transport/Parser.h>

namespace apache::thrift::fast_thrift::frame::read {

/**
 * Parses RSocket frames. A REQUEST_RESPONSE or PAYLOAD frame returns its data
 * in a separate buffer. REQUEST_RESPONSE also places its first binary field on
 * a 16-byte boundary.
 *
 *   struct Request {
 *     1: binary blob;   // alone in its own buffer, on a 16-byte boundary
 *     2: i64 offset;
 *   }
 *
 * Thrift hands over a binary field without copying it. Code that keeps `blob`
 * therefore keeps only the data buffer. PAYLOAD gets the same separation, but
 * no alignment.
 *
 * A binary field stays in one buffer only when the frame is not fragmented.
 * REQUEST_RESPONSE alignment also requires the blob to be the first field and
 * the request to use the binary protocol. The parser checks none of these
 * conditions. Parsing still works if one is false, but the binary field may
 * span buffers or lose alignment.
 *
 * A REQUEST_RESPONSE or PAYLOAD frame has separate header, metadata and data
 * buffers. Every other frame uses a queue whose node boundaries can fall
 * anywhere.
 *
 * The parser controls where data lands, so it implements Parser and not
 * MovableBufferParser.
 *
 * The fixed header buffer is allocated before the frame length is known. The
 * parser allocates each variable-size buffer when the transport asks for its
 * space. It checks maxFrameSize after allocating the fixed header, but before
 * allocating the body, metadata or data.
 */
class AlignedParser {
 public:
  static constexpr size_t kDefaultMinBufferSize = 256;
  static constexpr size_t kDefaultMaxBufferSize = 4096;

  // No frame on the wire can be bigger, so by default the cap turns nothing
  // away. A caller that wants a real cap passes the largest frame its service
  // expects.
  static constexpr size_t kDefaultMaxFrameSize = kMaxFrameLength;

  explicit AlignedParser(
      size_t minBufferSize = kDefaultMinBufferSize,
      size_t maxBufferSize = kDefaultMaxBufferSize,
      size_t maxFrameSize = kDefaultMaxFrameSize) noexcept;

  void getReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;

  // Returns Error when the frame is malformed. After that the parser stays on
  // the bad frame, and getReadBuffer offers 0 bytes of room. Call reset()
  // before feeding more bytes, or close the connection.
  template <typename Sink>
  channel_pipeline::Result consume(size_t len, Sink&& sink) noexcept;

  // The factory must return an empty buffer with at least the tailroom asked
  // for. A short buffer leaves a field the parser cannot finish, and from then
  // on getReadBuffer offers no room. The parser may also move the start of the
  // buffer forward by up to 15 bytes.
  void setIOBufFactory(folly::IOBufFactory* factory) noexcept;

  void reset() noexcept;

 private:
  enum class State {
    AwaitingHeader,
    // The three below run only for frames that get their own buffers.
    AwaitingMetadataLength,
    AwaitingMetadata,
    AwaitingData,
    // Everything else lands here and is read into one queue.
    AwaitingBody,
  };

  // What a byte handler tells consume to do next. This exists so the handlers
  // can live in the .cpp. Only the call to the sink has to be a template.
  enum class Step {
    NeedMore,
    FrameReady,
    Bad,
  };

  // One per state, so getReadBuffer is nothing but a switch.
  void headerReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;
  void metadataReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;
  void dataReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;
  void bodyReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;

  Step onBytes(size_t len) noexcept;
  Step onHeaderBytes(size_t len) noexcept;
  Step onMetadataLengthBytes(size_t len) noexcept;
  Step onMetadataBytes(size_t len) noexcept;
  Step onDataBytes(size_t len) noexcept;
  Step onBodyBytes(size_t len) noexcept;

  // REQUEST_RESPONSE and PAYLOAD both get their header, metadata and data in
  // buffers of their own. Only the request also has its data shifted.
  bool hasOwnBuffers() const noexcept;
  bool needsAlignment() const noexcept;

  Step startOwnBuffers(uint16_t flags) noexcept;
  channel_pipeline::BytesPtr newBuffer(size_t capacity) noexcept;
  channel_pipeline::BytesPtr newDataBuffer() noexcept;
  channel_pipeline::BytesPtr takeFrame() noexcept;
  void startNextFrame() noexcept;

  static constexpr size_t kAlignment = 16;

  // Binary protocol puts 10 bytes in front of the blob's own bytes: 3 for the
  // field header of the struct Thrift builds around the RPC arguments, 3 for
  // the field header of the blob, and 4 for the blob length.
  static constexpr size_t kBytesBeforeFirstField = 3 + 3 + 4;

  const size_t minBufferSize_;
  const size_t maxBufferSize_;
  const size_t maxFrameSize_;

  State state_{State::AwaitingHeader};
  FrameType frameType_{FrameType::RESERVED};
  size_t remainingHeader_;
  size_t remainingMetadata_{0};
  size_t remainingData_{0};
  size_t remainingBody_{0};
  size_t frameLength_{0};

  // For a frame with its own buffers these three come out as one chain. For
  // any other frame header_ is moved into body_ and the two below stay empty.
  channel_pipeline::BytesPtr header_;
  channel_pipeline::BytesPtr metadata_;
  channel_pipeline::BytesPtr data_;
  folly::IOBufQueue body_{folly::IOBufQueue::cacheChainLength()};

  folly::IOBufFactory* bufFactory_{nullptr};
};

template <typename Sink>
channel_pipeline::Result AlignedParser::consume(
    size_t len, Sink&& sink) noexcept {
  const Step step = onBytes(len);
  if (step == Step::NeedMore) {
    return channel_pipeline::Result::Success;
  }
  if (FOLLY_UNLIKELY(step == Step::Bad)) {
    return channel_pipeline::Result::Error;
  }
  return sink(takeFrame());
}

static_assert(transport::Parser<AlignedParser>);
static_assert(!transport::MovableBufferParser<AlignedParser>);

} // namespace apache::thrift::fast_thrift::frame::read
