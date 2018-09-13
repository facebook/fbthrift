/*
 * Copyright 2018-present Facebook, Inc.
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

#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>

namespace apache {
namespace thrift {
namespace rocket {

template <class T>
class Parser final : public folly::AsyncTransportWrapper::ReadCallback {
 public:
  explicit Parser(T& owner) : owner_(owner) {}

  // AsyncTransportWrapper::ReadCallback implementation
  FOLLY_NOINLINE void getReadBuffer(void** bufout, size_t* lenout) override;
  FOLLY_NOINLINE void readDataAvailable(size_t nbytes) noexcept override;
  FOLLY_NOINLINE void readEOF() noexcept override;
  FOLLY_NOINLINE void readErr(
      const folly::AsyncSocketException&) noexcept override;

 private:
  static constexpr size_t kMinBufferSize = 256;
  static constexpr size_t kMaxBufferSize = 4096;

  T& owner_;
  size_t bufferSize_{kMinBufferSize};
  folly::IOBuf readBuffer_{folly::IOBuf::CreateOp(), bufferSize_};
};

} // namespace rocket
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/transport/rocket/framing/Parser-inl.h>
