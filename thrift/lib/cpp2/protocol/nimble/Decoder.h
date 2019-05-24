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
#include <memory>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleDecoder.h>
#include <thrift/lib/cpp2/protocol/nimble/NimbleTypes.h>

namespace apache {
namespace thrift {
namespace detail {

class Decoder {
 public:
  Decoder() : cursor_(nullptr) {}
  ~Decoder() = default;
  Decoder(const Decoder&) = delete;
  Decoder& operator=(const Decoder&) = delete;

  void setInput(const folly::io::Cursor& cursor) {
    cursor_ = cursor;
    std::uint32_t fieldControlSize = cursor_.readLE<std::uint32_t>();
    std::uint32_t fieldDataSize = cursor_.readLE<std::uint32_t>();
    std::uint32_t contentControlSize = cursor_.readLE<std::uint32_t>();
    std::uint32_t contentDataSize = cursor_.readLE<std::uint32_t>();
    // binaryDataSize
    cursor_.readLE<std::uint32_t>();

    cursor_.clone(fieldControl_, fieldControlSize);
    cursor_.clone(fieldData_, fieldDataSize);
    cursor_.clone(contentControl_, contentControlSize);
    cursor_.clone(contentData_, contentDataSize);

    fieldStream_.setControlInput(fieldControl_);
    fieldStream_.setDataInput(fieldData_);
    contentStream_.setControlInput(contentControl_);
    contentStream_.setDataInput(contentData_);
  }

  std::uint32_t nextFieldChunk() {
    return fieldStream_.nextChunk();
  }

  std::uint32_t nextContentChunk() {
    return contentStream_.nextChunk();
  }

  void nextBinary(unsigned char* buf, std::size_t size) {
    cursor_.pull(buf, size);
  }

  void nextBinary(char* buf, std::size_t size) {
    cursor_.pull(buf, size);
  }

  template <class T>
  void nextBinary(T& str, std::size_t size) {
    str.reserve(size);
    str.clear();

    while (size > 0) {
      auto data = cursor_.peekBytes();
      auto data_avail = std::min(data.size(), size);
      if (data.empty()) {
        protocol::TProtocolException::throwExceededSizeLimit();
      }

      str.append(reinterpret_cast<const char*>(data.data()), data_avail);
      size -= data_avail;
      cursor_.skipNoAdvance(data_avail);
    }
  }

  void nextBinary(folly::IOBuf& buf, std::size_t size) {
    cursor_.clone(buf, size);
  }

  // For stringy types, and structy containers of a large size, we use a varint
  // encoding; the high bit of the chunk is set to 1, to indicate that there's a
  // follow-on chunk containing the remaining bits of the size info. This method
  // returns the next field stream chunk (which is assumed to be a metadata
  // chunk[1], because of a previously obtained chunk), but with the high size
  // bits already filled in by obtaining the next chunk.
  //
  // [1] The phrasing might make this seem like a security issue; we're assuming
  // a property of potentially untrusted data without checking it. But by
  // "assume" here, we really just mean that the low bits are ignored entirely;
  // an attacker could fill them with invalid data or valid data, and we'd act
  // in the same way. We may at some point want to check and throw as
  // performance/convenience tradeoff.
  std::uint64_t nextMetadataChunks() {
    // Note: we pad this chunk out to 64 bits. This makes the arithmetic easier
    // later on if we need to pad it out because the high bit is set (indicating
    // a large value for "size").
    std::uint64_t metadataChunk = nextFieldChunk();
    if (UNLIKELY((metadataChunk >> 31) > 0)) {
      std::uint64_t nextChunk = nextFieldChunk();
      // Low bits are just the type; since we know there's a continuation chunk
      // coming, we can ignore them.
      // We could check this and throw if they're not what we expect. This
      // isn't actually a useful validation for *securtiy* purposes (a hostile
      // message could just include the correct bits after all), but may help in
      // detecting that a stream of bytes was not in fact sent by a nimble
      // serializer, which will never emit a metadata chunk with the high-bit
      // set followed by a non-metadata chunk.
      nextChunk >>= nimble::kFieldChunkHintBits;
      metadataChunk &= 0x7FFFFFFFU;
      metadataChunk |= (nextChunk << 31);
    }
    return metadataChunk;
  }

 private:
  BufferingNimbleDecoder<ChunkRepr::kRaw> fieldStream_;
  BufferingNimbleDecoder<ChunkRepr::kZigzag> contentStream_;

  folly::IOBuf fieldControl_;
  folly::IOBuf fieldData_;
  folly::IOBuf contentControl_;
  folly::IOBuf contentData_;

  folly::io::Cursor cursor_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
