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

#include <folly/Function.h>
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

  // Recommended. Uses the global IO executor and does not support future_ /
  // unprefixed (callback) methods.
  static RequestChannel::Ptr newChannel(
      ImplCreator implCreator, size_t numThreads = 1) {
    return {
        new PooledRequestChannel(
            nullptr,
            globalExecutorProvider(numThreads),
            std::move(implCreator)),
        {}};
  }

  // Uses the global IO executor + supports future_ / callback methods
  static RequestChannel::Ptr newChannel(
      folly::Executor* callbackExecutor,
      ImplCreator implCreator,
      size_t numThreads = 1) {
    return {
        new PooledRequestChannel(
            callbackExecutor,
            globalExecutorProvider(numThreads),
            std::move(implCreator)),
        {}};
  }

  // Does not support future_ / unprefixed (callback) methods
  // (but does support semifuture_ / co_).
  static RequestChannel::Ptr newSyncChannel(
      std::weak_ptr<folly::IOExecutor> executor, ImplCreator implCreator) {
    return {
        new PooledRequestChannel(
            nullptr, wrapWeakPtr(std::move(executor)), std::move(implCreator)),
        {}};
  }

  static RequestChannel::Ptr newChannel(
      folly::Executor* callbackExecutor,
      std::weak_ptr<folly::IOExecutor> executor,
      ImplCreator implCreator) {
    return {
        new PooledRequestChannel(
            callbackExecutor,
            wrapWeakPtr(std::move(executor)),
            std::move(implCreator)),
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
  using EventBaseProvider = folly::Function<folly::Executor::KeepAlive<
      folly::EventBase>()>; // may return invalid keepalive

  PooledRequestChannel(
      folly::Executor* callbackExecutor,
      EventBaseProvider getEventBase,
      ImplCreator implCreator)
      : implCreator_(std::move(implCreator)),
        callbackExecutor_(callbackExecutor),
        getEventBase_(std::move(getEventBase)),
        impl_(std::make_shared<folly::EventBaseLocal<ImplPtr>>()) {}

  Impl& impl(folly::EventBase& evb);

  folly::Executor::KeepAlive<folly::EventBase> getEvb(
      const RpcOptions& options);

  static EventBaseProvider wrapWeakPtr(
      std::weak_ptr<folly::IOExecutor> executor);
  static EventBaseProvider globalExecutorProvider(size_t numThreads);

  ImplCreator implCreator_;

  folly::Executor* callbackExecutor_{nullptr};
  EventBaseProvider getEventBase_;

  std::shared_ptr<folly::EventBaseLocal<ImplPtr>> impl_;

  std::atomic<uint16_t> protocolId_{0};
};
} // namespace thrift
} // namespace apache
