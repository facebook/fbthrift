/*
 * Copyright 2019-present Facebook, Inc.
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

#include <cstdint>
#include <cstring>

#include <folly/Likely.h>
#include <folly/io/Cursor.h>

#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/DecodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

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

  void setControlInput(const folly::IOBuf& buf) {
    controlCursor_.reset(&buf);
  }

  void setDataInput(const folly::IOBuf& buf) {
    dataCursor_.reset(&buf);
  }

  std::uint32_t nextChunk() {
    if (UNLIKELY(nextChunkToReturn_ == maxChunkFilled_)) {
      nextChunkToReturn_ = 0;
      maxChunkFilled_ = 0;
      fillBuffer();
    }
    std::uint32_t result = chunks_[nextChunkToReturn_];
    ++nextChunkToReturn_;
    return result;
  }

 private:
  void fillBuffer() {
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
        for (ssize_t i = 0; i < numUnconditionalIters; ++i) {
          std::uint8_t controlByte = controlCursor_.data()[i];
          DCHECK_GE(dataCursor_.length(), kMaxBytesPerBlock);
          dataBytesConsumed += decodeNimbleBlock<repr>(
              controlByte,
              // Note that more obvious thing, of subranging a peekBytes()
              // result, unfortunately has a cost here; it introduces at least
              // an extra branch per block.
              folly::ByteRange(
                  dataCursor_.data() + dataBytesConsumed,
                  dataCursor_.data() + dataBytesConsumed + kMaxBytesPerBlock),
              &chunks_[maxChunkFilled_]);
          maxChunkFilled_ += kChunksPerBlock;
        }
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
  ssize_t nextChunkToReturn_ = 0;
  ssize_t maxChunkFilled_ = 0;

  folly::io::Cursor controlCursor_{nullptr};
  folly::io::Cursor dataCursor_{nullptr};
};

} // namespace detail
} // namespace thrift
} // namespace apache
