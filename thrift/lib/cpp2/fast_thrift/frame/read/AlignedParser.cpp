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

#include <thrift/lib/cpp2/fast_thrift/frame/read/AlignedParser.h>

#include <algorithm>
#include <cstdint>

#include <folly/io/Cursor.h>

#include <thrift/lib/cpp2/fast_thrift/frame/FrameDescriptor.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/FrameParser.h>

namespace apache::thrift::fast_thrift::frame::read {

namespace {

// Length prefix plus the base header. Every frame starts with these.
constexpr size_t kHeaderSize = kMetadataLengthSize + kBaseHeaderSize;

// A frame that carries metadata has three more header bytes for the metadata
// length. One buffer holds the header in either case.
constexpr size_t kHeaderBufferSize = kHeaderSize + kMetadataLengthSize;

} // namespace

AlignedParser::AlignedParser(
    size_t minBufferSize, size_t maxBufferSize, size_t maxFrameSize) noexcept
    : minBufferSize_(minBufferSize),
      maxBufferSize_(maxBufferSize),
      maxFrameSize_(maxFrameSize),
      remainingHeader_(kHeaderSize) {}

channel_pipeline::BytesPtr AlignedParser::newBuffer(size_t capacity) noexcept {
  return bufFactory_ ? (*bufFactory_)(capacity)
                     : folly::IOBuf::create(capacity);
}

bool AlignedParser::hasOwnBuffers() const noexcept {
  // startOwnBuffers and onMetadataLengthBytes both measure from
  // kBaseHeaderSize. A frame type that carries extra header bytes would push
  // both by that many, so it stays on the plain path until someone handles it.
  return (frameType_ == FrameType::REQUEST_RESPONSE ||
          frameType_ == FrameType::PAYLOAD) &&
      getDescriptor(frameType_).headerSize == kBaseHeaderSize;
}

bool AlignedParser::needsAlignment() const noexcept {
  return frameType_ == FrameType::REQUEST_RESPONSE;
}

channel_pipeline::BytesPtr AlignedParser::newDataBuffer() noexcept {
  // Without the shift there is nothing to leave room for, so the buffer is
  // exactly the size of the data.
  if (!needsAlignment()) {
    return newBuffer(remainingData_);
  }

  // Room for the data plus the shift below, which is under kAlignment bytes.
  channel_pipeline::BytesPtr buf = newBuffer(remainingData_ + kAlignment);

  // The blob sits kBytesBeforeFirstField into the data field, so move the
  // start until the blob lands on a kAlignment boundary. A buffer that already
  // starts on a boundary gives a shift of 6, which is the classic case. We
  // measure it instead of assuming it, so the parser works with any factory.
  const uintptr_t start = reinterpret_cast<uintptr_t>(buf->writableTail());
  const size_t shift =
      (kAlignment - ((start + kBytesBeforeFirstField) % kAlignment)) %
      kAlignment;
  buf->advance(shift);
  return buf;
}

void AlignedParser::getReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  switch (state_) {
    case State::AwaitingHeader:
    case State::AwaitingMetadataLength:
      headerReadBuffer(bufReturn, lenReturn);
      return;
    case State::AwaitingMetadata:
      metadataReadBuffer(bufReturn, lenReturn);
      return;
    case State::AwaitingData:
      dataReadBuffer(bufReturn, lenReturn);
      return;
    case State::AwaitingBody:
      bodyReadBuffer(bufReturn, lenReturn);
      return;
  }
}

void AlignedParser::headerReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  if (!header_) {
    header_ = newBuffer(kHeaderBufferSize);
  }
  *bufReturn = header_->writableTail();
  // The factory decides the real size, so offer what the buffer can take.
  *lenReturn = std::min(header_->tailroom(), remainingHeader_);
}

void AlignedParser::metadataReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  if (!metadata_) {
    metadata_ = newBuffer(remainingMetadata_);
  }
  *bufReturn = metadata_->writableTail();
  *lenReturn = std::min(metadata_->tailroom(), remainingMetadata_);
}

void AlignedParser::dataReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  if (!data_) {
    data_ = newDataBuffer();
  }
  *bufReturn = data_->writableTail();
  *lenReturn = std::min(data_->tailroom(), remainingData_);
}

void AlignedParser::bodyReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  // preallocate keeps the current tail when it has minBufferSize_ bytes of
  // room. A shorter tail gets allocationSize bytes.
  const size_t allocationSize =
      std::max(frameLength_ + kMetadataLengthSize, maxBufferSize_);
  const auto [buf, room] =
      body_.preallocate(minBufferSize_, allocationSize, allocationSize);
  *bufReturn = buf;
  // Do not offer room past the end of this frame. We do not know how big the
  // next frame is until we have read its header.
  *lenReturn = std::min(room, remainingBody_);
}

AlignedParser::Step AlignedParser::onBytes(size_t len) noexcept {
  switch (state_) {
    case State::AwaitingHeader:
      return onHeaderBytes(len);
    case State::AwaitingMetadataLength:
      return onMetadataLengthBytes(len);
    case State::AwaitingMetadata:
      return onMetadataBytes(len);
    case State::AwaitingData:
      return onDataBytes(len);
    case State::AwaitingBody:
      return onBodyBytes(len);
  }
  return Step::Bad;
}

AlignedParser::Step AlignedParser::onHeaderBytes(size_t len) noexcept {
  header_->append(len);
  remainingHeader_ -= len;
  if (remainingHeader_ > 0) {
    return Step::NeedMore;
  }

  folly::io::Cursor cursor{header_.get()};
  frameLength_ = detail::readFrameOrMetadataSize(cursor);

  // The length a frame gives must at least cover the header itself.
  if (FOLLY_UNLIKELY(frameLength_ < kBaseHeaderSize)) {
    return Step::Bad;
  }
  // Refuse before anything is allocated. If we checked later, we would first
  // reserve the exact size we then turn down.
  if (FOLLY_UNLIKELY(frameLength_ > maxFrameSize_)) {
    return Step::Bad;
  }

  cursor.skip(kStreamIdSize);
  const auto [frameTypeRaw, flags] = detail::readFrameTypeAndFlags(cursor);
  frameType_ = static_cast<FrameType>(frameTypeRaw);
  if (hasOwnBuffers()) {
    return startOwnBuffers(flags);
  }

  body_.append(std::move(header_));
  remainingBody_ = frameLength_ - kBaseHeaderSize;
  if (remainingBody_ == 0) {
    // Nothing after the header. A CANCEL frame looks like this.
    return Step::FrameReady;
  }

  state_ = State::AwaitingBody;
  return Step::NeedMore;
}

AlignedParser::Step AlignedParser::startOwnBuffers(uint16_t flags) noexcept {
  if (flags & frame::detail::kMetadataBit) {
    // Three more header bytes hold the metadata length.
    if (FOLLY_UNLIKELY(frameLength_ < kBaseHeaderSize + kMetadataLengthSize)) {
      return Step::Bad;
    }
    remainingHeader_ = kMetadataLengthSize;
    state_ = State::AwaitingMetadataLength;
    return Step::NeedMore;
  }

  remainingData_ = frameLength_ - kBaseHeaderSize;
  if (remainingData_ == 0) {
    return Step::FrameReady;
  }
  state_ = State::AwaitingData;
  return Step::NeedMore;
}

AlignedParser::Step AlignedParser::onMetadataLengthBytes(size_t len) noexcept {
  header_->append(len);
  remainingHeader_ -= len;
  if (remainingHeader_ > 0) {
    return Step::NeedMore;
  }

  folly::io::Cursor cursor{header_.get()};
  cursor.skip(kHeaderSize);
  remainingMetadata_ = detail::readFrameOrMetadataSize(cursor);

  const size_t budget = frameLength_ - kBaseHeaderSize - kMetadataLengthSize;
  if (FOLLY_UNLIKELY(remainingMetadata_ > budget)) {
    return Step::Bad;
  }
  remainingData_ = budget - remainingMetadata_;

  if (remainingMetadata_ > 0) {
    state_ = State::AwaitingMetadata;
    return Step::NeedMore;
  }
  if (remainingData_ == 0) {
    return Step::FrameReady;
  }
  state_ = State::AwaitingData;
  return Step::NeedMore;
}

AlignedParser::Step AlignedParser::onMetadataBytes(size_t len) noexcept {
  metadata_->append(len);
  remainingMetadata_ -= len;
  if (remainingMetadata_ > 0) {
    return Step::NeedMore;
  }
  if (remainingData_ == 0) {
    return Step::FrameReady;
  }
  state_ = State::AwaitingData;
  return Step::NeedMore;
}

AlignedParser::Step AlignedParser::onDataBytes(size_t len) noexcept {
  data_->append(len);
  remainingData_ -= len;
  return remainingData_ == 0 ? Step::FrameReady : Step::NeedMore;
}

AlignedParser::Step AlignedParser::onBodyBytes(size_t len) noexcept {
  body_.postallocate(len);
  remainingBody_ -= len;
  return remainingBody_ == 0 ? Step::FrameReady : Step::NeedMore;
}

channel_pipeline::BytesPtr AlignedParser::takeFrame() noexcept {
  channel_pipeline::BytesPtr frame;
  if (hasOwnBuffers()) {
    frame = std::move(header_);
    // The length prefix does not go downstream.
    frame->trimStart(kMetadataLengthSize);
    if (metadata_) {
      frame->appendToChain(std::move(metadata_));
    }
    if (data_) {
      frame->appendToChain(std::move(data_));
    }
  } else {
    body_.trimStart(kMetadataLengthSize);
    frame = body_.split(frameLength_);
  }
  // Clear the framing state before the sink runs, because it may re-enter.
  startNextFrame();
  return frame;
}

void AlignedParser::startNextFrame() noexcept {
  state_ = State::AwaitingHeader;
  frameType_ = FrameType::RESERVED;
  remainingHeader_ = kHeaderSize;
  remainingMetadata_ = 0;
  remainingData_ = 0;
  remainingBody_ = 0;
  frameLength_ = 0;
}

void AlignedParser::setIOBufFactory(folly::IOBufFactory* factory) noexcept {
  bufFactory_ = factory;
  body_.setIOBufFactory(factory);
}

void AlignedParser::reset() noexcept {
  header_.reset();
  metadata_.reset();
  data_.reset();
  body_.reset();
  startNextFrame();
}

} // namespace apache::thrift::fast_thrift::frame::read
