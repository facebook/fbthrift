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
#include <thrift/lib/cpp2/async/SaslEndpoint.h>

namespace apache { namespace thrift {

class ProtectionHandler : public folly::wangle::BytesToBytesHandler {
 public:
  enum class ProtectionState {
    UNKNOWN,
    NONE,
    INPROGRESS,
    VALID,
    INVALID,
  };

  SaslEndpoint* getSaslEndpoint() {
    return saslEndpoint_;
  }

  ProtectionState getProtectionState() {
    return state_;
  }

  void setProtectionState(ProtectionState state, SaslEndpoint* saslEndpoint) {
    state_ = state;
    saslEndpoint_ = saslEndpoint;
    protectionStateChanged();
  }

  virtual void protectionStateChanged() {}

  void read(Context* ctx, folly::IOBufQueue& q) override {
    auto e = folly::try_and_catch<std::exception>([&](){
      while (true) {
        if (state_ == ProtectionState::INVALID) {
          throw transport::TTransportException("protection is invalid");
        }
        if (state_ != ProtectionState::VALID) {
          // Not encrypted, passthrough
          ctx->fireRead(q);
          return;
        }

        if (!q.front() || q.front()->empty()) {
          return;
        }

        size_t remaining;
        CHECK(saslEndpoint_);
        auto unwrapped = saslEndpoint_->unwrap(&q, &remaining);
        if (unwrapped) {
          bufQueue_.append(std::move(unwrapped));
          ctx->fireRead(bufQueue_);
        } else {
          return;
        }
      }
    });
    if (e) {
      VLOG(5) << "Exception in ProtectionHandler::read(): " << e.what();
      ctx->fireReadException(std::move(e));
    }
  };

  folly::Future<void> write(
      Context* ctx,
      std::unique_ptr<folly::IOBuf> buf) override {
    if (state_ == ProtectionState::VALID) {
      CHECK(saslEndpoint_);
      buf = saslEndpoint_->wrap(std::move(buf));
    }
    return ctx->fireWrite(std::move(buf));
  }

 private:
  ProtectionState state_{ProtectionState::UNKNOWN};
  SaslEndpoint* saslEndpoint_{nullptr};
  folly::IOBufQueue bufQueue_;
};

}}
