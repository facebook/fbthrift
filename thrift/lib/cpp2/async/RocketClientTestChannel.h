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

#include <folly/Optional.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace apache {
namespace thrift {

class RocketClientTestChannel final : public RequestChannel {
 public:
  using Ptr = std::unique_ptr<
      RocketClientTestChannel,
      folly::DelayedDestruction::Destructor>;

  static Ptr newChannel(folly::AsyncTransport::UniquePtr socket) {
    auto channel = RocketClientChannel::newChannel(std::move(socket));
    return RocketClientTestChannel::Ptr(
        new RocketClientTestChannel(std::move(channel)));
  }

  void sendRequestResponse(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& serializedRequest,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cb) override {
    injectCompressionConfig(header);
    channel_->sendRequestResponse(
        rpcOptions,
        std::move(methodName),
        std::move(serializedRequest),
        std::move(header),
        std::move(cb));
  }

  void sendRequestNoResponse(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& serializedRequest,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cb) override {
    injectCompressionConfig(header);
    channel_->sendRequestNoResponse(
        rpcOptions,
        std::move(methodName),
        std::move(serializedRequest),
        std::move(header),
        std::move(cb));
  }

  void setCloseCallback(CloseCallback* cb) override {
    channel_->setCloseCallback(cb);
  }

  folly::EventBase* getEventBase() const override {
    return channel_->getEventBase();
  }

  uint16_t getProtocolId() override {
    return channel_->getProtocolId();
  }

  void setDesiredCompressionConfig(CompressionConfig compressionConfig) {
    compressionConfig_ = compressionConfig;
  }

 private:
  void injectCompressionConfig(std::shared_ptr<transport::THeader>& header) {
    if (compressionConfig_) {
      header->setDesiredCompressionConfig(*compressionConfig_);
    }
  }

  explicit RocketClientTestChannel(RocketClientChannel::Ptr&& channel)
      : channel_(std::move(channel)) {}

  RocketClientChannel::Ptr channel_;
  folly::Optional<CompressionConfig> compressionConfig_;
};

} // namespace thrift
} // namespace apache
