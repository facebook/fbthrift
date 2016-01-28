/*
 * Copyright 2016 Facebook, Inc.
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
#include <thrift/lib/cpp/transport/THeader.h>
#include <wangle/channel/Handler.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

namespace apache { namespace thrift {

using BufAndHeader = std::pair<
    std::unique_ptr<folly::IOBuf>,
    std::unique_ptr<apache::thrift::transport::THeader>>;

class SaslNegotiationHandler : public wangle::InboundHandler<BufAndHeader> {
 public:
  ~SaslNegotiationHandler() override {}

  void read(Context* ctx,  BufAndHeader bufAndHeader) override;

  void setProtectionHandler(ProtectionHandler* h) {
    protectionHandler_ = h;
  }

  virtual bool handleSecurityMessage(
      std::unique_ptr<folly::IOBuf>&& buf,
      std::unique_ptr<apache::thrift::transport::THeader>&& header) = 0;

 protected:
  ProtectionHandler* protectionHandler_{nullptr};
};

class DummySaslNegotiationHandler : public SaslNegotiationHandler {
 public:
  void read(Context* ctx, BufAndHeader bufAndHeader) override {
    ctx->fireRead(std::move(bufAndHeader));
  }

  bool handleSecurityMessage(
      std::unique_ptr<folly::IOBuf>&& buf,
      std::unique_ptr<apache::thrift::transport::THeader>&& header) {
    return false;
  }
};

}}
