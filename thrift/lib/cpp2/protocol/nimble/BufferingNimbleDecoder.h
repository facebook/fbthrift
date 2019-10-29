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

#include <cstdint>
#include <cstring>
#include <utility>

#include <folly/Likely.h>
#include <folly/io/Cursor.h>

#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/DecodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

struct BufferingNimbleDecoderState {
 public:
  BufferingNimbleDecoderState() = default;
  BufferingNimbleDecoderState(const BufferingNimbleDecoderState&) = delete;
  BufferingNimbleDecoderState(BufferingNimbleDecoderState&& other) noexcept {
    nextChunkToReturn_ = std::exchange(other.nextChunkToReturn_, -1);
  }
  BufferingNimbleDecoderState& operator=(const BufferingNimbleDecoderState&) =
      delete;
  BufferingNimbleDecoderState& operator=(
      BufferingNimbleDecoderState&& other) noexcept {
    nextChunkToReturn_ = std::exchange(other.nextChunkToReturn_, -1);
    return *this;
  }

 private:
  template <ChunkRepr repr>
  friend class BufferingNimbleDecoder;

  explicit BufferingNimbleDecoderState(ssize_t nextChunkToReturn) noexcept
      : nextChunkToReturn_(nextChunkToReturn) {}

  ssize_t nextChunkToReturn_ = -1;
};

// The buffering decoder serves two purposes:
// 1. Provides chunk-at-a-time semantics. Naively, lower level decoders only
//    work in blocks of 4 chunks (because each control stream byte maps to 4
//    chunks). But often times, consumer classes will want just one or two
//    chunks, and don't want to have to deal with bookkeeping the extra ones.
// 2. It decodes more than 1 block at a time, as an optimization. (We can save
//    some boundary calculation, as well as do some cache locality tuning).
template <ChunkRepr repr>
class BufferingNimbleDecoder {
 public:
  BufferingNimbleDecoder() = default;
  ~BufferingNimbleDecoder() = default;
  BufferingNimbleDecoder(const BufferingNimbleDecoder&) = delete;
  BufferingNimbleDecoder& operator=(const BufferingNimbleDecoder&) = delete;

  void setControlInput(const folly::io::Cursor& cursor) {
    controlCursor_ = cursor;
  }

  void setDataInput(const folly::io::Cursor& cursor) {
    dataCursor_ = cursor;
  }

  BufferingNimbleDecoderState borrowState() {
    if (folly::kIsDebug) {
      DCHECK(!stateBorrowed_);
      stateBorrowed_ = true;
    }
    return BufferingNimbleDecoderState{nextChunkToReturn_};
  }

  void returnState(BufferingNimbleDecoderState state) {
    DCHECK(state.nextChunkToReturn_ != -1);

    nextChunkToReturn_ = state.nextChunkToReturn_;

    if (folly::kIsDebug) {
      DCHECK(stateBorrowed_);
      stateBorrowed_ = false;
      state.nextChunkToReturn_ = -1;
    }
  }

  std::uint32_t nextChunk() {
    DCHECK(!stateBorrowed_);
    auto state = borrowState();
    std::uint32_t ret = nextChunk(state);
    returnState(std::move(state));
    return ret;
  }

  std::uint32_t nextChunk(BufferingNimbleDecoderState& state) {
    DCHECK(stateBorrowed_);
    DCHECK(state.nextChunkToReturn_ != -1);

    if (UNLIKELY(state.nextChunkToReturn_ == maxChunkFilled_)) {
      fillBuffer();
      state = BufferingNimbleDecoderState{0};
    }
    std::uint32_t result = chunks_[state.nextChunkToReturn_];
    ++state.nextChunkToReturn_;
    return result;
  }

 private:
  void fillBuffer() {
    maxChunkFilled_ = 0;
    // We were asked to produce more chunks, but can't advance in the control
    // stream; the input is invalid. Note that a similar check on the data
    // stream wouldn't be correct; a control byte may correspond to 0 data
    // bytes. Instead, we check that we didn't overflow manually, in the slow
    // path below.
    if (!controlCursor_.canAdvance(1)) {
      protocol::TProtocolException::throwExceededSizeLimit();
    }
    while (maxChunkFilled_ < kChunksToBuffer && controlCursor_.canAdvance(1)) {
      ssize_t minControlBytes = controlCursor_.length();
      ssize_t minBlocks = dataCursor_.length() / kMaxBytesPerBlock;
      ssize_t headRoom = (kChunksToBuffer - maxChunkFilled_) / kChunksPerBlock;
      ssize_t numUnconditionalIters =
          std::min({minControlBytes, minBlocks, headRoom});
      ssize_t dataBytesConsumed = 0;
      if (UNLIKELY(numUnconditionalIters == 0)) {
        // Slow path where we cross IOBufs.
        std::uint8_t controlByte = controlCursor_.read<std::uint8_t>();
        std::array<unsigned char, kMaxBytesPerBlock> buf;
        size_t bytesPulled =
            dataCursor_.pullAtMost(buf.data(), kMaxBytesPerBlock);
        if (LIKELY(bytesPulled == kMaxBytesPerBlock)) {
          // We're at the end of the current IOBuf, but not necessarily at the
          // end of the whole chain.
          size_t bytesDecoded = decodeNimbleBlock<repr>(
              controlByte, folly::ByteRange(buf), &chunks_[maxChunkFilled_]);
          dataCursor_.retreat(bytesPulled - bytesDecoded);
          maxChunkFilled_ += kChunksPerBlock;
        } else {
          std::memset(&buf[bytesPulled], 0, kMaxBytesPerBlock - bytesPulled);
          size_t bytesDecoded = decodeNimbleBlock<repr>(
              controlByte, folly::ByteRange(buf), &chunks_[maxChunkFilled_]);
          if (bytesDecoded > bytesPulled) {
            protocol::TProtocolException::throwExceededSizeLimit();
          }
          dataCursor_.retreat(bytesPulled - bytesDecoded);
          maxChunkFilled_ += kChunksPerBlock;
        }
      } else {
        // Otherwise, we iterate through until we hit the end or run out of
        // buffer.

        // We do a little dance where we move values into temporaries, run the
        // loop, and then move them back. This lets the compiler keep the values
        // in registers, avoiding having to materialize the memory operations;
        // this would not be a correct optimization otherwise (what if the
        // buffers aliased *this?).
        ssize_t maxChunkFilledTemp = maxChunkFilled_;
        const std::uint8_t* controlBufTemp = controlCursor_.data();
        const std::uint8_t* dataBufTemp = dataCursor_.data();
        for (ssize_t i = 0; i < numUnconditionalIters; ++i) {
          std::uint8_t controlByte = controlBufTemp[i];
          dataBytesConsumed += decodeNimbleBlock<repr>(
              controlByte,
              // Note that more obvious thing, of subranging a peekBytes()
              // result, unfortunately has a cost here; it introduces at least
              // an extra branch per block.
              folly::ByteRange(
                  dataBufTemp + dataBytesConsumed,
                  dataBufTemp + dataBytesConsumed + kMaxBytesPerBlock),
              &chunks_[maxChunkFilledTemp]);
          maxChunkFilledTemp += kChunksPerBlock;
        }
        maxChunkFilled_ = maxChunkFilledTemp;
        controlCursor_.skip(numUnconditionalIters);
        dataCursor_.skip(dataBytesConsumed);
      }
    }
  }

  // Low level decode works 4 chunks at a time.
  const static int kChunksToBuffer = 256;
  static_assert(
      kChunksToBuffer % kChunksPerBlock == 0,
      "Must not have partially decoded blocks");

  std::uint32_t chunks_[kChunksToBuffer];
  bool stateBorrowed_ = false;
  ssize_t nextChunkToReturn_ = 0;
  ssize_t maxChunkFilled_ = 0;

  folly::io::Cursor controlCursor_{nullptr};
  folly::io::Cursor dataCursor_{nullptr};
};

} // namespace detail
} // namespace thrift
} // namespace apache
