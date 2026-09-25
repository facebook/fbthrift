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
#include <folly/io/IOBufQueue.h>
#include <folly/lang/Hint.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/transport/Parser.h>

namespace apache::thrift::fast_thrift::frame::read {

/**
 * Parses RSocket frames and emits each frame without its three-byte length
 * prefix. Every frame uses the same queue path, so buffer boundaries can fall
 * anywhere.
 *
 * The parser owns its read buffers. It implements Parser and not
 * MovableBufferParser.
 *
 * The fixed header buffer is allocated before the frame length is known. The
 * body buffer is allocated when the transport asks for body space. The parser
 * checks maxFrameSize after allocating the fixed header, but before allocating
 * the body.
 *
 * If the queue has less than minBufferSize of tailroom, it requests the full
 * wire frame size or maxBufferSize, whichever is larger.
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

  void setIOBufFactory(folly::IOBufFactory* factory) noexcept;

  void reset() noexcept;

 private:
  enum class State {
    AwaitingHeader,
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
  void bodyReadBuffer(void** bufReturn, size_t* lenReturn) noexcept;

  Step onHeaderBytes(size_t len) noexcept;
  Step onBodyBytes(size_t len) noexcept;
  channel_pipeline::BytesPtr takeFrame() noexcept;
  void startNextFrame() noexcept;

  static constexpr size_t kAlignment = 16;

  // Binary protocol puts 10 bytes in front of the blob's own bytes: 3 for the
  // field header of the struct Thrift builds around the RPC arguments, 3 for
  // the field header of the blob, and 4 for the blob length.
  static constexpr size_t kBytesBeforeFirstField = 3 + 3 + 4;

  static constexpr size_t kDataBufferPadding =
      kAlignment - kBytesBeforeFirstField;

  const size_t minBufferSize_;
  const size_t maxBufferSize_;
  const size_t maxFrameSize_;

  State state_{State::AwaitingHeader};
  size_t remainingHeader_;
  size_t remainingBody_{0};
  size_t frameLength_{0};

  // The parser fills this first, then moves it into body_, so the frame comes
  // out as one piece.
  channel_pipeline::BytesPtr header_;
  folly::IOBufQueue body_{folly::IOBufQueue::cacheChainLength()};

  folly::IOBufFactory* bufFactory_{nullptr};
};

template <typename Sink>
channel_pipeline::Result AlignedParser::consume(
    size_t len, Sink&& sink) noexcept {
  const Step step =
      state_ == State::AwaitingHeader ? onHeaderBytes(len) : onBodyBytes(len);
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
