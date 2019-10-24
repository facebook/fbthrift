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

#include <folly/Likely.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/EncodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

// This is a counterpart to the buffering Decoder, and serves much the same
// purposes -- see the comment there.
template <ChunkRepr repr>
class BufferingNimbleEncoder {
 public:
  BufferingNimbleEncoder() = default;
  ~BufferingNimbleEncoder() = default;
  BufferingNimbleEncoder(const BufferingNimbleEncoder&) = delete;
  BufferingNimbleEncoder& operator=(const BufferingNimbleEncoder&) = delete;

  void setControlOutput(folly::IOBufQueue* controlBuf) {
    controlBuf_ = controlBuf;
  }

  void setDataOutput(folly::IOBufQueue* dataBuf) {
    dataBuf_ = dataBuf;
  }

  // TODO: We will eventually want a way to "reserve" chunks in the output
  // stream. That is to say, we should be able to decide up-front that we'll
  // grab enough space for any value of the given type, and get a token back,
  // later filling in the chunk(s) value(s) by calling a method on the token.
  void encodeChunk(std::uint32_t chunk) {
    if (UNLIKELY(nextChunk_ == kChunksToBuffer)) {
      flushBuffer();
    }
    chunks_[nextChunk_] = chunk;
    ++nextChunk_;
  }

  // TODO: We currently just "pad-out" with zeros to make the result a full
  // 4-chunk block. But really, we should think about this in the context of a
  // more reasonably interleaved stream.
  void finalize() {
    while (nextChunk_ % kChunksPerBlock != 0) {
      encodeChunk(0);
    }
    flushBuffer();
  }

 private:
  const static int kChunksToBuffer = 256;
  static_assert(
      kChunksToBuffer % kChunksPerBlock == 0,
      "LowLevelEncoder works 4 chunks_ at a time");

  void flushBuffer() {
    DCHECK(nextChunk_ % kChunksPerBlock == 0);
    for (int i = 0; i < nextChunk_; i += kChunksPerBlock) {
      // Taken from CompactProtocol
      constexpr size_t kDesiredGrowth = (1 << 14) - 64;

      auto controlPair = controlBuf_->preallocate(1, kDesiredGrowth);
      auto dataPair = dataBuf_->preallocate(kMaxBytesPerBlock, kDesiredGrowth);
      std::uint8_t* controlOut = static_cast<std::uint8_t*>(controlPair.first);
      unsigned char* dataOut = static_cast<unsigned char*>(dataPair.first);

      int dataBytesConsumed =
          encodeNimbleBlock<repr>(&chunks_[i], controlOut, dataOut);

      controlBuf_->postallocate(1);
      dataBuf_->postallocate(dataBytesConsumed);
    }
    nextChunk_ = 0;
  }

  std::uint32_t chunks_[kChunksToBuffer];
  int nextChunk_ = 0;

  folly::IOBufQueue* controlBuf_{nullptr};
  folly::IOBufQueue* dataBuf_{nullptr};
};

} // namespace detail
} // namespace thrift
} // namespace apache
