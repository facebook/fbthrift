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

#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <utility>

#include <folly/ExceptionString.h>
#include <folly/ExceptionWrapper.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocketException.h>
#include <folly/io/async/DelayedDestruction.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/FrameType.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

namespace apache {
namespace thrift {
namespace rocket {

template <class T>
void Parser<T>::getReadBuffer(void** bufout, size_t* lenout) {
  DCHECK(!readBuffer_.isChained());
  if (LIKELY(!aligning_)) {
    resizeBuffer();
    readBuffer_.unshareOne();

    if (readBuffer_.length() == 0) {
      DCHECK(readBuffer_.capacity() > 0);
      // If we read everything, reset pointers to 0 and reuse the buffer
      readBuffer_.clear();
    } else if (readBuffer_.headroom() > 0) {
      // Move partially read data to the beginning
      readBuffer_.retreat(readBuffer_.headroom());
    }
  }
  *bufout = readBuffer_.writableTail();
  *lenout = readBuffer_.tailroom();
}

template <class T>
void Parser<T>::readDataAvailable(size_t nbytes) noexcept {
  folly::DelayedDestruction::DestructorGuard dg(&this->owner_);

  try {
    readBuffer_.append(nbytes);

    while (!readBuffer_.empty()) {
      if (readBuffer_.length() < Serializer::kMinimumFrameHeaderLength) {
        return;
      }

      folly::io::Cursor cursor(&readBuffer_);
      const size_t totalFrameSize = Serializer::kBytesForFrameOrMetadataLength +
          readFrameOrMetadataSize(cursor);

      readStreamId(cursor);
      uint8_t frameType;
      std::tie(frameType, std::ignore) = readFrameTypeAndFlagsUnsafe(cursor);
      if (UNLIKELY(
              static_cast<FrameType>(frameType) == FrameType::EXT &&
              !aligning_)) {
        if (readBuffer_.length() < Serializer::kBytesForFrameOrMetadataLength +
                ExtFrame::frameHeaderSize()) {
          return;
        }

        ExtFrameType extType = readExtFrameType(cursor);
        if (UNLIKELY(extType == ExtFrameType::ALIGNED_PAGE)) {
          enablePageAlignment_ = true;
          if (alignTo4k(
                  readBuffer_,
                  Serializer::kBytesForFrameOrMetadataLength +
                      ExtFrame::frameHeaderSize(),
                  totalFrameSize)) {
            aligning_ = true;
          }
        }
      }

      if (readBuffer_.length() < totalFrameSize) {
        if (readBuffer_.length() + readBuffer_.tailroom() < totalFrameSize) {
          DCHECK(!readBuffer_.isChained());
          readBuffer_.unshareOne();
          bufferSize_ = std::max<size_t>(bufferSize_, totalFrameSize);
          readBuffer_.reserve(
              0 /* minHeadroom */,
              bufferSize_ - readBuffer_.length() /* minTailroom */);
        }
        return;
      }

      // Otherwise, we have a full frame to handle.
      const size_t bytesToClone =
          totalFrameSize - Serializer::kBytesForFrameOrMetadataLength;
      cursor.reset(&readBuffer_);
      readFrameOrMetadataSize(cursor);
      std::unique_ptr<folly::IOBuf> frame;
      cursor.clone(frame, bytesToClone);
      readBuffer_.trimStart(totalFrameSize);
      aligning_ = false;
      owner_.handleFrame(std::move(frame));
    }
  } catch (...) {
    auto exceptionStr =
        folly::exceptionStr(std::current_exception()).toStdString();
    LOG(ERROR) << "Bad frame received, closing connection: " << exceptionStr;
    owner_.close(transport::TTransportException(exceptionStr));
  }
}

template <class T>
void Parser<T>::readEOF() noexcept {
  folly::DelayedDestruction::DestructorGuard dg(&this->owner_);

  owner_.close(transport::TTransportException(
      transport::TTransportException::TTransportExceptionType::END_OF_FILE,
      "Channel got EOF. Check for server hitting connection limit, "
      "server connection idle timeout, and server crashes."));
}

template <class T>
void Parser<T>::readErr(const folly::AsyncSocketException& ex) noexcept {
  folly::DelayedDestruction::DestructorGuard dg(&this->owner_);

  owner_.close(transport::TTransportException(ex));
}

template <class T>
void Parser<T>::resizeBuffer() {
  if (bufferSize_ <= kMaxBufferSize || readBuffer_.length() >= kMaxBufferSize) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();

  // if enablePageAlignment_ we need to always shrink the buffer otherwise
  // parser may receive too much data and need to costly aligns them later
  if (enablePageAlignment_ || now - resizeBufferTimer_ > resizeBufferTimeout_) {
    // resize readBuffer_ to kMaxBufferSize
    readBuffer_ = folly::IOBuf(
        folly::IOBuf::CopyBufferOp(),
        readBuffer_.data(),
        readBuffer_.length(),
        /* headroom */ 0,
        /* tailroom */ kMaxBufferSize - readBuffer_.length());
    resizeBufferTimer_ = now;
    bufferSize_ = kMaxBufferSize;
  }
}

template <class T>
constexpr size_t Parser<T>::kMinBufferSize;
template <class T>
constexpr size_t Parser<T>::kMaxBufferSize;
template <class T>
constexpr std::chrono::milliseconds Parser<T>::kDefaultBufferResizeInterval;

} // namespace rocket
} // namespace thrift
} // namespace apache
