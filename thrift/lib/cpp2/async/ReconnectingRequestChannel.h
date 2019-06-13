/*
 * Copyright 2018-present Facebook, Inc.
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

#include <future>

#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache {
namespace thrift {

// Simple RequestChannel wrapper, which automatically re-creates underlying
// RequestChannel in case of any transport exception.
class ReconnectingRequestChannel : public apache::thrift::RequestChannel {
 public:
  using Impl = apache::thrift::RequestChannel;
  using ImplPtr = std::shared_ptr<Impl>;
  using ImplCreator = folly::Function<ImplPtr(folly::EventBase&)>;

  static std::unique_ptr<
      ReconnectingRequestChannel,
      folly::DelayedDestruction::Destructor>
  newChannel(folly::EventBase& evb, ImplCreator implCreator) {
    return {new ReconnectingRequestChannel(evb, std::move(implCreator)), {}};
  }

  void sendRequestResponse(
      apache::thrift::RpcOptions& options,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      RequestClientCallback::Ptr cob) override;

  void sendRequestNoResponse(
      apache::thrift::RpcOptions&,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override {
    LOG(FATAL) << "Not supported";
  }

  void setCloseCallback(apache::thrift::CloseCallback*) override {
    LOG(FATAL) << "Not supported";
  }

  folly::EventBase* getEventBase() const override {
    return &evb_;
  }

  uint16_t getProtocolId() override {
    return impl().getProtocolId();
  }

 protected:
  ~ReconnectingRequestChannel() override = default;

 private:
  ReconnectingRequestChannel(folly::EventBase& evb, ImplCreator implCreator)
      : implCreator_(std::move(implCreator)), evb_(evb) {}

  Impl& impl();

  class RequestCallback;

  ImplPtr impl_;
  ImplCreator implCreator_;
  folly::EventBase& evb_;
};
} // namespace thrift
} // namespace apache
