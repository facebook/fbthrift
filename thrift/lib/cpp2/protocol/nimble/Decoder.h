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
#include <memory>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/util/VarintUtils.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleDecoder.h>
#include <thrift/lib/cpp2/protocol/nimble/NimbleTypes.h>

namespace apache {
namespace thrift {
namespace detail {

class Decoder {
 public:
  Decoder() : fieldCursor_(nullptr), stringCursor_(nullptr) {}
  ~Decoder() = default;
  Decoder(const Decoder&) = delete;
  Decoder& operator=(const Decoder&) = delete;

  void setInput(folly::io::Cursor cursor) {
    // For now this is ignored; but it allows the flexibility we'll need for
    // defining struct skipping eventually.
    std::uint64_t messageFlags;
    apache::thrift::util::readVarint(cursor, messageFlags);

    std::uint64_t fieldSize;
    apache::thrift::util::readVarint(cursor, fieldSize);

    std::uint64_t sizeControlSize;
    apache::thrift::util::readVarint(cursor, sizeControlSize);
    std::uint64_t sizeDataSize;
    apache::thrift::util::readVarint(cursor, sizeDataSize);

    std::uint64_t contentControlSize;
    apache::thrift::util::readVarint(cursor, contentControlSize);
    std::uint64_t contentDataSize;
    apache::thrift::util::readVarint(cursor, contentDataSize);

    std::uint64_t stringSize;
    apache::thrift::util::readVarint(cursor, stringSize);

    auto splice = [&](std::uint64_t size) {
      if (size != static_cast<std::size_t>(size)) { // when size_t != uint64_t
        protocol::TProtocolException::throwExceededSizeLimit();
      }
      folly::io::Cursor result{cursor, (std::size_t)size};
      cursor.skip(size);
      return result;
    };

    fieldCursor_ = splice(fieldSize);
    auto sizeControl = splice(sizeControlSize);
    auto sizeData = splice(sizeDataSize);
    sizeStream_.setControlInput(sizeControl);
    sizeStream_.setDataInput(sizeData);

    auto contentControl = splice(contentControlSize);
    auto contentData = splice(contentDataSize);
    contentStream_.setControlInput(contentControl);
    contentStream_.setDataInput(contentData);

    // We've skipped all preceding items;
    stringCursor_ = splice(stringSize);

    if (!cursor.isAtEnd()) {
      protocol::TProtocolException::throwExceededSizeLimit();
    }
  }

  std::uint32_t nextSizeChunk() {
    return sizeStream_.nextChunk();
  }

  std::uint32_t nextContentChunk() {
    return contentStream_.nextChunk();
  }

  std::uint32_t nextContentChunk(BufferingNimbleDecoderState& state) {
    return contentStream_.nextChunk(state);
  }

  void skipStringBytes(std::size_t size) {
    stringCursor_.skip(size);
  }

  void nextBinary(unsigned char* buf, std::size_t size) {
    stringCursor_.pull(buf, size);
  }

  void nextBinary(char* buf, std::size_t size) {
    stringCursor_.pull(buf, size);
  }

  template <class T>
  void nextBinary(T& str, std::size_t size) {
    str.reserve(size);
    str.clear();

    while (size > 0) {
      auto data = stringCursor_.peekBytes();
      auto data_avail = std::min(data.size(), size);
      if (data.empty()) {
        protocol::TProtocolException::throwExceededSizeLimit();
      }

      str.append(reinterpret_cast<const char*>(data.data()), data_avail);
      size -= data_avail;
      stringCursor_.skipNoAdvance(data_avail);
    }
  }

  void nextBinary(folly::IOBuf& buf, std::size_t size) {
    stringCursor_.clone(buf, size);
  }

  folly::ByteRange fieldRange() {
    return fieldCursor_.peekBytes();
  }

  void skipFieldBytes(std::size_t size) {
    fieldCursor_.skip(size);
  }

  std::uint8_t nextFieldByte() {
    return fieldCursor_.read<std::uint8_t>();
  }

  std::uint16_t nextFieldShort() {
    return fieldCursor_.read<std::uint16_t>();
  }

  BufferingNimbleDecoderState borrowState() {
    return contentStream_.borrowState();
  }

  void returnState(BufferingNimbleDecoderState state) {
    contentStream_.returnState(std::move(state));
  }

 private:
  BufferingNimbleDecoder<ChunkRepr::kRaw> sizeStream_;
  BufferingNimbleDecoder<ChunkRepr::kZigzag> contentStream_;

  folly::io::Cursor fieldCursor_;
  folly::io::Cursor stringCursor_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
