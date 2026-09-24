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

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <folly/Range.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/portability/GMock.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>

namespace apache::thrift::fast_thrift::frame::read {

/**
 * The parser owns the memory. It gives out a writable buffer, the caller
 * writes into it and says how many bytes it wrote. Every parser supports this.
 */
struct ConsumeDriver {
  template <typename Parser, typename Sink>
  static channel_pipeline::Result feed(
      Parser& parser, folly::ByteRange bytes, Sink&& sink) {
    // A parser may offer only enough room for its next field, so one frame can
    // take several rounds.
    while (!bytes.empty()) {
      void* buf = nullptr;
      size_t avail = 0;
      parser.getReadBuffer(&buf, &avail);
      if (avail == 0) {
        ADD_FAILURE() << "parser offered no room with " << bytes.size()
                      << " bytes left to feed";
        return channel_pipeline::Result::Error;
      }
      const auto len = std::min(avail, bytes.size());
      std::memcpy(buf, bytes.data(), len);
      bytes.advance(len);
      const auto result = parser.consume(len, sink);
      if (result != channel_pipeline::Result::Success) {
        return result;
      }
    }
    return channel_pipeline::Result::Success;
  }
};

/**
 * The caller owns the memory. It hands the parser a ready buffer and the parser
 * takes it over. Only a parser that satisfies transport::MovableBufferParser
 * can do this.
 */
struct ConsumeBufferDriver {
  template <typename Parser, typename Sink>
  static channel_pipeline::Result feed(
      Parser& parser, folly::ByteRange bytes, Sink&& sink) {
    return parser.consumeBuffer(folly::IOBuf::copyBuffer(bytes), sink);
  }
};

/**
 * Like ConsumeBufferDriver, but the buffer is a chain of several pieces. A
 * transport can send either shape. A parser that reads only the first piece
 * still passes the flat case. So the chain case needs its own run.
 */
struct ConsumeBufferChainDriver {
  template <typename Parser, typename Sink>
  static channel_pipeline::Result feed(
      Parser& parser, folly::ByteRange bytes, Sink&& sink) {
    return parser.consumeBuffer(splitIntoChain(bytes), sink);
  }

 private:
  // Two cuts. The first is after one byte, so a piece ends inside the header.
  // The second is in the middle, so a piece ends inside the payload.
  static channel_pipeline::BytesPtr splitIntoChain(folly::ByteRange bytes) {
    const std::array<size_t, 3> cuts{
        std::min<size_t>(1, bytes.size()), bytes.size() / 2, bytes.size()};

    channel_pipeline::BytesPtr chain;
    size_t offset = 0;
    for (const auto cut : cuts) {
      if (cut <= offset) {
        continue;
      }
      auto piece =
          folly::IOBuf::copyBuffer(bytes.subpiece(offset, cut - offset));
      if (chain) {
        chain->appendToChain(std::move(piece));
      } else {
        chain = std::move(piece);
      }
      offset = cut;
    }

    if (!chain) {
      return folly::IOBuf::create(0);
    }
    return chain;
  }
};

/**
 * What the suite needs to know about a parser: its wire format, and which
 * driver runs it.
 */
template <typename T>
concept ParserTraits = std::default_initializable<typename T::Parser> &&
    requires(size_t payload, uint8_t fill) {
      typename T::Driver;
      // The bytes of one frame as they arrive on the wire: this parser's
      // header, then `payload` bytes, all equal to `fill`.
      { T::makeFrame(payload, fill) } -> std::same_as<std::vector<uint8_t>>;
      // Can be larger than payload: a parser may keep part of the header in
      // the frame it emits.
      { T::emittedSize(payload) } -> std::same_as<size_t>;
      // Bytes the parser must see before it knows the frame length.
      { T::headerSize() } -> std::same_as<size_t>;
    };

template <ParserTraits TraitsT>
class ParserContractTest : public ::testing::Test {
 protected:
  using Traits = TraitsT;
  using Driver = typename Traits::Driver;

  auto sink() noexcept {
    return [this](channel_pipeline::BytesPtr&& frame) noexcept {
      frames_.push_back(std::move(frame));
      return sinkResult_;
    };
  }

  channel_pipeline::Result feed(folly::ByteRange bytes) {
    return Driver::feed(parser_, bytes, sink());
  }

  channel_pipeline::Result feed(const std::vector<uint8_t>& bytes) {
    return feed(folly::ByteRange{bytes.data(), bytes.size()});
  }

  // A different byte per frame, so a test can tell which frame is which.
  static uint8_t fillFor(size_t index) {
    return static_cast<uint8_t>('a' + index);
  }

  static std::vector<uint8_t> concatFrames(size_t count, size_t payloadSize) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < count; ++i) {
      const auto frame = Traits::makeFrame(payloadSize, fillFor(i));
      bytes.insert(bytes.end(), frame.begin(), frame.end());
    }
    return bytes;
  }

  // The payload is always at the end of a frame, so check the last bytes.
  static void expectPayload(
      const folly::IOBuf& frame, size_t payloadSize, uint8_t fill) {
    const size_t frameSize = frame.computeChainDataLength();
    EXPECT_THAT(frameSize, ::testing::Ge(payloadSize));
    if (frameSize < payloadSize) {
      return;
    }

    folly::io::Cursor cursor{&frame};
    cursor.skip(frameSize - payloadSize);
    size_t remaining = payloadSize;
    while (remaining > 0) {
      const folly::ByteRange bytes = cursor.peekBytes();
      EXPECT_THAT(bytes.empty(), ::testing::IsFalse());
      if (bytes.empty()) {
        return;
      }
      const size_t length = std::min(bytes.size(), remaining);
      EXPECT_THAT(
          std::all_of(
              bytes.begin(),
              bytes.begin() + length,
              [fill](uint8_t byte) { return byte == fill; }),
          ::testing::IsTrue());
      cursor.skip(length);
      remaining -= length;
    }
  }

  std::vector<channel_pipeline::BytesPtr> frames_;
  channel_pipeline::Result sinkResult_{channel_pipeline::Result::Success};
  typename Traits::Parser parser_;
};

} // namespace apache::thrift::fast_thrift::frame::read
