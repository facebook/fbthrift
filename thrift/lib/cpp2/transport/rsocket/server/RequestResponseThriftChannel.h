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

#include <rsocket/RSocket.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <yarpl/Single.h>

namespace apache {
namespace thrift {

class RequestResponseThriftChannel : public ThriftChannelIf {
 public:
  explicit RequestResponseThriftChannel(
      folly::EventBase* evb,
      yarpl::Reference<yarpl::single::SingleObserver<rsocket::Payload>>
          subscriber)
      : evb_(evb), subscriber_(subscriber) {}

  bool supportsHeaders() const noexcept override {
    return true;
  }

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override;

  // TODO - this methods isn't get called even for error case!
  void sendErrorWrapped(
      folly::exception_wrapper /* ex */,
      std::string exCode,
      apache::thrift::MessageChannel::SendCallback* /* cb */ = nullptr) {
    VLOG(3) << "sendErrorWrapped";
    subscriber_->onSubscribe(yarpl::single::SingleSubscriptions::empty());
    subscriber_->onError(std::runtime_error(std::move(exCode)));
  }

  folly::EventBase* getEventBase() noexcept override {
    return evb_;
  }

  static std::unique_ptr<folly::IOBuf> serializeMetadata(
      const ResponseRpcMetadata& responseMetadata);

  static std::unique_ptr<RequestRpcMetadata> deserializeMetadata(
      const folly::IOBuf& buffer);

 private:
  void cancel(int32_t /*seqId*/) noexcept override {
    LOG(FATAL) << "cancel() not yet implemented";
  }

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      std::unique_ptr<ThriftClientCallback>) noexcept override {
    LOG(DFATAL) << "Server should not call this function.";
  }

  void cancel(ThriftClientCallback* /*callback*/) noexcept override {
    LOG(FATAL) << "cancel() not yet implemented";
  }

  void setInput(int32_t, SubscriberRef) noexcept override {
    LOG(FATAL) << "RequestResponse should not support streaming.";
  }

  SubscriberRef getOutput(int32_t) noexcept override {
    LOG(FATAL) << "RequestResponse should not support streaming.";
  }

 private:
  folly::EventBase* evb_;
  yarpl::Reference<yarpl::single::SingleObserver<rsocket::Payload>> subscriber_;
};
} // namespace thrift
} // namespace apache
