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

#include <folly/ExceptionWrapper.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <yarpl/flowable/Subscriber.h>

namespace apache {
namespace thrift {

class RSThriftChannelBase : public ThriftChannelIf {
 public:
  explicit RSThriftChannelBase(folly::EventBase* evb) : evb_(evb) {}

  virtual ~RSThriftChannelBase() = default;

  bool supportsHeaders() const noexcept override {
    return false;
  }

  virtual void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override {
    LOG(FATAL) << "No response is allowed";
  }

  virtual void sendErrorWrapped(
      folly::exception_wrapper,
      std::string,
      MessageChannel::SendCallback* /*cb*/ = nullptr) noexcept {
    LOG(FATAL) << "No response is allowed";
  }

  folly::EventBase* getEventBase() noexcept override {
    return evb_;
  }

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      std::unique_ptr<ThriftClientCallback>) noexcept override {
    LOG(FATAL) << "Server should not call this function.";
  }

  void cancel(int32_t) noexcept override {
    LOG(FATAL) << "Server can call this function but it is not implemented";
  }

  void cancel(ThriftClientCallback*) noexcept override {
    LOG(FATAL) << "Server should not call this function.";
  }

  void setInput(int32_t, SubscriberRef) noexcept override {
    LOG(FATAL) << "Use StreamingInput/StreamingInputOutput";
  }

  SubscriberRef getOutput(int32_t) noexcept override {
    LOG(FATAL) << "Use StreamingOutput/StreamingInputOutput";
  }

 protected:
  folly::EventBase* evb_;
};

} // namespace thrift
} // namespace apache
