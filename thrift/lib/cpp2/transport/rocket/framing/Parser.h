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
            THRIFT_FLAG(rocket_parser_resize_period_seconds)) {}

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

  void timeoutExpired() noexcept override;

  const folly::IOBuf& getReadBuffer() const { return readBuffer_; }

  void setReadBuffer(folly::IOBuf&& buffer) { readBuffer_ = std::move(buffer); }

  size_t getReadBufferSize() const { return bufferSize_; }

  void setReadBufferSize(size_t size) { bufferSize_ = size; }

  void resizeBuffer();

  static constexpr size_t kMinBufferSize{256};
  static constexpr size_t kMaxBufferSize{4096};

 private:
  static constexpr std::chrono::milliseconds kDefaultBufferResizeInterval{
      std::chrono::seconds(3)};

  T& owner_;
  size_t bufferSize_{kMinBufferSize};
  folly::IOBuf readBuffer_{folly::IOBuf::CreateOp(), bufferSize_};
  std::chrono::steady_clock::time_point lastResizeTime_{
      std::chrono::steady_clock::now()};
  const std::chrono::milliseconds resizeBufferTimeout_;
  const int64_t periodicResizeBufferTimeout_;
  bool enablePageAlignment_{false};
  bool aligning_{false};
  size_t currentFrameLength_{0};
  // only used by readBufferAvailable API
  folly::IOBufQueue bufQueue_{folly::IOBufQueue::cacheChainLength()};
};

} // namespace rocket
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/transport/rocket/framing/Parser-inl.h>
