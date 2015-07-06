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

#include <wangle/channel/Handler.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/async/SaslEndpoint.h>

namespace apache { namespace thrift {

// This handler may only be used in a single Pipeline
class ProtectionHandler : public folly::wangle::BytesToBytesHandler {
 public:
  enum class ProtectionState {
    UNKNOWN,
    NONE,
    INPROGRESS,
    VALID,
    INVALID,
    WAITING,
  };

  ProtectionHandler()
    : protectionState_(ProtectionState::UNKNOWN)
    , saslEndpoint_(nullptr)
  {}

  void setProtectionState(ProtectionState protectionState,
                          SaslEndpoint* saslEndpoint = nullptr) {
    protectionState_ = protectionState;
    saslEndpoint_ = saslEndpoint;
    protectionStateChanged();
  }

  ProtectionState getProtectionState() {
    return protectionState_;
  }

  SaslEndpoint* getSaslEndpoint() {
    return saslEndpoint_;
  }

  virtual void protectionStateChanged();

  ~ProtectionHandler() override {}

  /**
   * If q contains enough data, read it (removing it from q, but retaining
   * following data), decrypt it and return as result.first.
   * result.second is set to 0.
   *
   * If q doesn't contain enough data, return an empty unique_ptr in
   * result.first and return the requested amount of bytes in result.second.
   */
  void read(Context* ctx, folly::IOBufQueue& q) override;

  /**
   * Encrypt an IOBuf
   */
  folly::Future<folly::Unit> write(
    Context* ctx,
    std::unique_ptr<folly::IOBuf> buf) override;


  folly::Future<folly::Unit> close(Context* ctx) override;

 private:
  ProtectionState protectionState_;
  SaslEndpoint* saslEndpoint_;
  folly::IOBufQueue queue_{folly::IOBufQueue::cacheChainLength()};

  bool closing_{false};

 protected:
  folly::IOBufQueue inputQueue_{folly::IOBufQueue::cacheChainLength()};
};

}} // namespace
