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

#include <folly/futures/Future.h>
#include <rsocket/RSocketRequester.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>

namespace apache {
namespace thrift {

class RSClientThriftChannel : public ThriftChannelIf {
 public:
  using FlowableRef = yarpl::Reference<
      yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>>;

  explicit RSClientThriftChannel(
      std::shared_ptr<rsocket::RSocketRequester> rsRequester);

  virtual ~RSClientThriftChannel() = default;

  bool supportsHeaders() const noexcept override;

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  static std::unique_ptr<folly::IOBuf> serializeMetadata(
      const RequestRpcMetadata& requestMetadata);

  static std::unique_ptr<ResponseRpcMetadata> deserializeMetadata(
      const folly::IOBuf& buffer);

 protected:
  void setInput(
      int32_t,
      ThriftChannelIf::SubscriberRef input) noexcept override {
    input_ = input;
  }

  SubscriberRef getOutput(int32_t) noexcept override {
    auto future = outputPromise_.getFuture();
    return future.get();
  }

  void sendSingleRequestNoResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendSingleRequestResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void channelRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload) noexcept;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override {
    LOG(FATAL) << "Server side function is called in client side.";
  }

  void cancel(int32_t) noexcept override {
    LOG(FATAL) << "not implemented";
  }

  void cancel(ThriftClientCallback*) noexcept override {
    LOG(FATAL) << "not implemented";
  }

  folly::EventBase* getEventBase() noexcept override {
    LOG(FATAL) << "not implemented";
  }

 protected:
  std::shared_ptr<rsocket::RSocketRequester> rsRequester_;

  ThriftChannelIf::SubscriberRef input_;
  folly::Promise<ThriftChannelIf::SubscriberRef> outputPromise_;
};
} // namespace thrift
} // namespace apache
