/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/lib/cpp2/async/ProtectionHandler.h>
#include <wangle/channel/Handler.h>
#include <wangle/channel/StaticPipeline.h>

namespace apache { namespace thrift {

class FramingHandler : public folly::wangle::BytesToBytesHandler {
 public:
  ~FramingHandler() override {}

  /**
   * If q contains enough data, read it (removing it from q, but retaining
   * following data), unframe it and pass it to the next pipeline stage.
   *
   * If q doesn't contain enough data, set the read buffer settings
   * to the data remaining to get a full frame, then return *without*
   * calling the next pipeline stage.
   */
  void read(Context* ctx, folly::IOBufQueue& q) override;

  folly::Future<folly::Unit> write(
    Context* ctx,
    std::unique_ptr<folly::IOBuf> buf) override;

  virtual std::pair<std::unique_ptr<folly::IOBuf>, size_t>
  removeFrame(folly::IOBufQueue* q) = 0;

  /**
   * Wrap and IOBuf in any headers/footers
   */
  virtual std::unique_ptr<folly::IOBuf>
  addFrame(std::unique_ptr<folly::IOBuf> buf) = 0;

  void setProtectionHandler(ProtectionHandler* h) {
    protectionHandler_ = h;
  }

  void setReadBufferSize(size_t readBufferSize) {
    readBufferSize_ = std::max(
      readBufferSize, static_cast<size_t>(DEFAULT_BUFFER_SIZE));
  }

  folly::Future<folly::Unit> close(Context* ctx) override;

 private:
  enum BUFFER_SIZE { DEFAULT_BUFFER_SIZE = 2048, };

  ProtectionHandler* protectionHandler_{nullptr};

  size_t readBufferSize_{DEFAULT_BUFFER_SIZE};

  bool closing_{false};
};

}} // namespace
