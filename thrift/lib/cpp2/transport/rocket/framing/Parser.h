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

#include <chrono>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/AsyncTransport.h>

#include <thrift/lib/cpp2/Flags.h>

THRIFT_FLAG_DECLARE_int64(rocket_parser_resize_period_seconds);
THRIFT_FLAG_DECLARE_bool(rocket_parser_dont_hold_buffer_enabled);

namespace apache {
namespace thrift {
namespace rocket {

template <class T>
class Parser final : public folly::AsyncTransport::ReadCallback,
                     public folly::HHWheelTimer::Callback {
 public:
  explicit Parser(
      T& owner,
      std::chrono::milliseconds resizeBufferTimeout =
          kDefaultBufferResizeInterval)
      : owner_(owner),
        resizeBufferTimeout_(resizeBufferTimeout),
        periodicResizeBufferTimeout_(
            THRIFT_FLAG(rocket_parser_resize_period_seconds)),
        newBufferLogicEnabled_(
            THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled)) {}

  ~Parser() override {
    if (currentFrameLength_) {
      owner_.decMemoryUsage(currentFrameLength_);
    }
  }

  // AsyncTransport::ReadCallback implementation
  FOLLY_NOINLINE void getReadBuffer(void** bufout, size_t* lenout) override;
  FOLLY_NOINLINE void readDataAvailable(size_t nbytes) noexcept override;
  FOLLY_NOINLINE void readEOF() noexcept override;
  FOLLY_NOINLINE void readErr(
      const folly::AsyncSocketException&) noexcept override;
  FOLLY_NOINLINE void readBufferAvailable(
      std::unique_ptr<folly::IOBuf> /*readBuf*/) noexcept override;

  bool isBufferMovable() noexcept override { return true; }

  // TODO: This should be removed once the new buffer logic controlled by
  // THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is stable.
  void timeoutExpired() noexcept override;

  // TODO: This should be removed once the new buffer logic controlled by
  // THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is stable.
  const folly::IOBuf& getReadBuffer() const { return readBuffer_; }

  // TODO: This should be removed once the new buffer logic controlled by
  // THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is stable.
  void setReadBuffer(folly::IOBuf&& buffer) { readBuffer_ = std::move(buffer); }

  size_t getReadBufferSize() const { return bufferSize_; }

  void setReadBufferSize(size_t size) { bufferSize_ = size; }

  // TODO: This should be removed once the new buffer logic controlled by
  // THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is stable.
  void resizeBuffer();

  size_t getReadBufLength() const {
    if (newBufferLogicEnabled_) {
      return readBufQueue_.chainLength();
    }
    return readBuffer_.computeChainDataLength();
  }

  bool getNewBufferLogicEnabled() const { return newBufferLogicEnabled_; }

  static constexpr size_t kMinBufferSize{256};
  static constexpr size_t kMaxBufferSize{4096};

 private:
  void getReadBufferOld(void** bufout, size_t* lenout);
  void getReadBufferNew(void** bufout, size_t* lenout);
  void readDataAvailableOld(size_t nbytes);
  void readDataAvailableNew(size_t nbytes);

  // TODO: This should be removed once the new buffer logic controlled by
  // THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is stable.
  static constexpr std::chrono::milliseconds kDefaultBufferResizeInterval{
      std::chrono::seconds(3)};

  T& owner_;
  size_t bufferSize_{kMinBufferSize};
  // TODO: readBuffer_, lastResizeTime_, resizeBufferTimeout_ and
  // periodicResizeBufferTimeout_ should be removed once the new buffer logic
  // controlled by THRIFT_FLAG(rocket_parser_dont_hold_buffer_enabled) is
  // stable.
  folly::IOBuf readBuffer_{folly::IOBuf::CreateOp(), bufferSize_};
  std::chrono::steady_clock::time_point lastResizeTime_{
      std::chrono::steady_clock::now()};
  const std::chrono::milliseconds resizeBufferTimeout_;
  const int64_t periodicResizeBufferTimeout_;
  bool enablePageAlignment_{false};
  bool aligning_{false};
  size_t currentFrameLength_{0};
  // used by readDataAvailable or readBufferAvailable API (only one will be
  // invoked for a given AsyncTransport)
  folly::IOBufQueue readBufQueue_{folly::IOBufQueue::cacheChainLength()};
  // Flag that controls if the parser should use the new buffer logic
  const bool newBufferLogicEnabled_;
};

} // namespace rocket
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/transport/rocket/framing/Parser-inl.h>
