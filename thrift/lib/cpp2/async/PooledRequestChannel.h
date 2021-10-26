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

#include <atomic>

#include <folly/executors/IOExecutor.h>
#include <folly/io/async/EventBaseLocal.h>

#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache {
namespace thrift {

// Simple RequestChannel wrapper. Keeps a pool of RequestChannels backed by
// running on different IO threads. Dispatches requests to these RequestChannels
// using round-robin.
class PooledRequestChannel : public RequestChannel {
 public:
  using Impl = RequestChannel;
  using ImplPtr = std::shared_ptr<Impl>;
  using ImplCreator = folly::Function<ImplPtr(folly::EventBase&)>;

  static std::
      unique_ptr<PooledRequestChannel, folly::DelayedDestruction::Destructor>
      newSyncChannel(
          std::weak_ptr<folly::IOExecutor> executor, ImplCreator implCreator) {
    return {
        new PooledRequestChannel(
            nullptr, std::move(executor), std::move(implCreator)),
        {}};
  }

  static std::
      unique_ptr<PooledRequestChannel, folly::DelayedDestruction::Destructor>
      newChannel(
          folly::Executor* callbackExecutor,
          std::weak_ptr<folly::IOExecutor> executor,
          ImplCreator implCreator) {
    return {
        new PooledRequestChannel(
            callbackExecutor, std::move(executor), std::move(implCreator)),
        {}};
  }

  void sendRequestResponse(
      RpcOptions&& options,
      MethodMetadata&& methodMetadata,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cob) override;

  void sendRequestNoResponse(
      RpcOptions&& options,
      MethodMetadata&& methodMetadata,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cob) override;

  void sendRequestStream(
      RpcOptions&& options,
      MethodMetadata&& methodMetadata,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* cob) override;

  void sendRequestSink(
      RpcOptions&& options,
      MethodMetadata&& methodMetadata,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      SinkClientCallback* cob) override;

  using RequestChannel::sendRequestNoResponse;
  using RequestChannel::sendRequestResponse;
  using RequestChannel::sendRequestSink;
  using RequestChannel::sendRequestStream;

  void setCloseCallback(CloseCallback*) override {
    LOG(FATAL) << "Not supported";
  }

  folly::EventBase* getEventBase() const override { return nullptr; }

  uint16_t getProtocolId() override;

  // may be called from any thread
  void terminateInteraction(InteractionId id) override;

  // may be called from any thread
  InteractionId createInteraction(ManagedStringView&& name) override;

 protected:
  template <typename SendFunc>
  void sendRequestImpl(
      SendFunc&& sendFunc, folly::Executor::KeepAlive<folly::EventBase>&& evb);

 private:
  PooledRequestChannel(
      folly::Executor* callbackExecutor,
      std::weak_ptr<folly::IOExecutor> executor,
      ImplCreator implCreator)
      : implCreator_(std::move(implCreator)),
        callbackExecutor_(callbackExecutor),
        executor_(std::move(executor)),
        impl_(std::make_shared<folly::EventBaseLocal<ImplPtr>>()) {}

  Impl& impl(folly::EventBase& evb);

  folly::Executor::KeepAlive<folly::EventBase> getEvb(
      const RpcOptions& options);

  ImplCreator implCreator_;

  folly::Executor* callbackExecutor_{nullptr};
  std::weak_ptr<folly::IOExecutor> executor_;

  std::shared_ptr<folly::EventBaseLocal<ImplPtr>> impl_;

  std::atomic<uint16_t> protocolId_{0};
};
} // namespace thrift
} // namespace apache
