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

#include <folly/wangle/channel/ChannelHandler.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

namespace apache { namespace thrift {

class FramingHandler : public folly::wangle::BytesToBytesHandler {
 public:
  virtual std::pair<std::unique_ptr<folly::IOBuf>, size_t>
  removeFrame(folly::IOBufQueue* q) = 0;

  virtual std::unique_ptr<folly::IOBuf> addFrame(
      std::unique_ptr<folly::IOBuf> buf) = 0;

  void read(Context* ctx, folly::IOBufQueue& q) override {
    for (auto res = removeFrame(&q); res.first; res = removeFrame(&q)) {
      bufQueue_.append(std::move(res.first));
      ctx->fireRead(bufQueue_);
      if (done) {
        return;
      }
    }
  };

  folly::Future<void> write(
      Context* ctx,
      std::unique_ptr<folly::IOBuf> buf) override {
    return ctx->fireWrite(addFrame(std::move(buf)));
  }

  void cancel() {
    done = true;
  }

 private:
  folly::IOBufQueue bufQueue_;
  bool done{false};
};

}}
