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

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp/util/VarintUtils.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleEncoder.h>
#include <thrift/lib/cpp2/protocol/nimble/NimbleTypes.h>

namespace apache {
namespace thrift {
namespace detail {

class Encoder {
 public:
  Encoder() : fieldAppender_(&fieldData_, 4096) {
    sizeStream_.setControlOutput(&sizeControl_);
    sizeStream_.setDataOutput(&sizeData_);
    contentStream_.setControlOutput(&contentControl_);
    contentStream_.setDataOutput(&contentData_);
  }
  ~Encoder() = default;
  Encoder(const Encoder&) = delete;
  Encoder& operator=(const Encoder&) = delete;

  void encodeSizeChunk(std::uint32_t chunk) {
    sizeStream_.encodeChunk(chunk);
  }

  void encodeContentChunk(std::uint32_t chunk) {
    contentStream_.encodeChunk(chunk);
  }

  // Eventually, this should probably have an IOBuf-taking variant, too.
  void encodeBinary(const void* buf, std::size_t size) {
    binaryData_.append(buf, size);
  }

  std::unique_ptr<folly::IOBuf> finalize() {
    sizeStream_.finalize();
    contentStream_.finalize();

    // An enum field (with yet-to-be determined contents).
    std::uint64_t messageFlags = 0;

    auto fieldData = fieldData_.move();
    auto sizeControl = sizeControl_.move();
    auto sizeData = sizeData_.move();
    auto contentControl = contentControl_.move();
    auto contentData = contentData_.move();
    auto binaryData = binaryData_.move();

    auto chainLengthBytes = [](auto& iobufptr) {
      // IOBufQueue::move() can return null if no data was ever written to it.
      return iobufptr ? iobufptr->computeChainDataLength() : 0;
    };

    auto sizeHeader = folly::IOBuf::create(0);
    folly::io::Appender writer(sizeHeader.get(), 32);
    apache::thrift::util::writeVarint(writer, messageFlags);
    apache::thrift::util::writeVarint(writer, chainLengthBytes(fieldData));
    apache::thrift::util::writeVarint(writer, chainLengthBytes(sizeControl));
    apache::thrift::util::writeVarint(writer, chainLengthBytes(sizeData));
    apache::thrift::util::writeVarint(writer, chainLengthBytes(contentControl));
    apache::thrift::util::writeVarint(writer, chainLengthBytes(contentData));
    apache::thrift::util::writeVarint(writer, chainLengthBytes(binaryData));

    auto prependToSizeHeader = [&](auto ioBufUPtr) {
      if (ioBufUPtr != nullptr) {
        sizeHeader->prependChain(std::move(ioBufUPtr));
      }
    };

    prependToSizeHeader(std::move(fieldData));
    prependToSizeHeader(std::move(sizeControl));
    prependToSizeHeader(std::move(sizeData));
    prependToSizeHeader(std::move(contentControl));
    prependToSizeHeader(std::move(contentData));
    prependToSizeHeader(std::move(binaryData));

    return sizeHeader;
  }

  void encodeFieldBytes(nimble::FieldBytes bytes) {
    fieldAppender_.push(bytes.bytes, bytes.len);
  }

 private:
  folly::IOBufQueue fieldData_;
  folly::io::QueueAppender fieldAppender_;

  folly::IOBufQueue sizeControl_;
  folly::IOBufQueue sizeData_;
  // We expect size stream data to be exclusively positives, and so don't
  // bother zigzagging it.
  BufferingNimbleEncoder<ChunkRepr::kRaw> sizeStream_;

  folly::IOBufQueue contentControl_;
  folly::IOBufQueue contentData_;
  // Content stream data, on the other hand, may contain negatives.
  BufferingNimbleEncoder<ChunkRepr::kZigzag> contentStream_;

  // String and binary data is encoded as raw bytes.
  folly::IOBufQueue binaryData_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
