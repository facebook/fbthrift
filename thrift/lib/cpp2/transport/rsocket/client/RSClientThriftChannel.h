/*
 * Copyright 2017-present Facebook, Inc.
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

#include <rsocket/RSocketRequester.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>

namespace apache {
namespace thrift {

class RSClientThriftChannel : public ThriftChannelIf {
 public:
  explicit RSClientThriftChannel(
      std::shared_ptr<rsocket::RSocketRequester> rsRequester);

  bool supportsHeaders() const noexcept override;

  void sendThriftRequest(
      std::unique_ptr<FunctionInfo> functionInfo,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

 protected:
  virtual void sendSingleRequestResponse(
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendThriftResponse(
      uint32_t,
      std::unique_ptr<std::map<std::string, std::string>>,
      std::unique_ptr<folly::IOBuf>) noexcept override {
    LOG(FATAL) << "Server side function is called in client side.";
  }

  void cancel(uint32_t) noexcept override {
    LOG(FATAL) << "not implemented";
  }

  void cancel(ThriftClientCallback*) noexcept override {
    LOG(FATAL) << "not implemented";
  }

  folly::EventBase* getEventBase() noexcept override {
    LOG(FATAL) << "not implemented";
  }

  void setInput(uint32_t, SubscriberRef) noexcept override {
    LOG(FATAL) << "not implemented";
  }

  SubscriberRef getOutput(uint32_t) noexcept override {
    LOG(FATAL) << "not implemented";
  }

 protected:
  std::shared_ptr<rsocket::RSocketRequester> rsRequester_;
};
}
}
