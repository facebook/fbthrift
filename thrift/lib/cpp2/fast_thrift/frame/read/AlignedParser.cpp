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

#include <folly/io/Cursor.h>

#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/FrameParser.h>

namespace apache::thrift::fast_thrift::frame::read {

namespace {

// Length prefix plus the base header. Every frame starts with these.
constexpr size_t kHeaderSize = kMetadataLengthSize + kBaseHeaderSize;

} // namespace

AlignedParser::AlignedParser(
    size_t minBufferSize, size_t maxBufferSize, size_t maxFrameSize) noexcept
    : minBufferSize_(minBufferSize),
      maxBufferSize_(maxBufferSize),
      maxFrameSize_(maxFrameSize),
      remainingHeader_(kHeaderSize) {}

void AlignedParser::getReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  switch (state_) {
    case State::AwaitingHeader:
      headerReadBuffer(bufReturn, lenReturn);
      return;
    case State::AwaitingBody:
      bodyReadBuffer(bufReturn, lenReturn);
      return;
  }
}

void AlignedParser::headerReadBuffer(
    void** bufReturn, size_t* lenReturn) noexcept {
  if (!header_) {
    header_ = bufFactory_ ? (*bufFactory_)(kHeaderSize)
                          : folly::IOBuf::create(kHeaderSize);
  }
  *bufReturn = header_->writableTail();
  *lenReturn = remainingHeader_;
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

  body_.append(std::move(header_));
  remainingBody_ = frameLength_ - kBaseHeaderSize;
  if (remainingBody_ == 0) {
    // Nothing after the header. A CANCEL frame looks like this.
    return Step::FrameReady;
  }

  state_ = State::AwaitingBody;
  return Step::NeedMore;
}

AlignedParser::Step AlignedParser::onBodyBytes(size_t len) noexcept {
  body_.postallocate(len);
  remainingBody_ -= len;
  return remainingBody_ == 0 ? Step::FrameReady : Step::NeedMore;
}

channel_pipeline::BytesPtr AlignedParser::takeFrame() noexcept {
  // The length prefix does not go downstream.
  body_.trimStart(kMetadataLengthSize);
  channel_pipeline::BytesPtr frame = body_.split(frameLength_);
  // Clear the framing state before the sink runs, because it may re-enter.
  startNextFrame();
  return frame;
}

void AlignedParser::startNextFrame() noexcept {
  state_ = State::AwaitingHeader;
  remainingHeader_ = kHeaderSize;
  remainingBody_ = 0;
  frameLength_ = 0;
}

void AlignedParser::setIOBufFactory(folly::IOBufFactory* factory) noexcept {
  bufFactory_ = factory;
  body_.setIOBufFactory(factory);
}

void AlignedParser::reset() noexcept {
  header_.reset();
  body_.reset();
  startNextFrame();
}

} // namespace apache::thrift::fast_thrift::frame::read
