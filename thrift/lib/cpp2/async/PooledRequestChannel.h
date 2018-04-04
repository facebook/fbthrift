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
          std::shared_ptr<folly::IOExecutor> executor,
          ImplCreator implCreator) {
    return {new PooledRequestChannel(
                nullptr, std::move(executor), std::move(implCreator)),
            {}};
  }

  static std::
      unique_ptr<PooledRequestChannel, folly::DelayedDestruction::Destructor>
      newChannel(
          folly::Executor* callbackExecutor,
          std::shared_ptr<folly::IOExecutor> executor,
          ImplCreator implCreator) {
    return {new PooledRequestChannel(
                callbackExecutor, std::move(executor), std::move(implCreator)),
            {}};
  }

  uint32_t sendRequest(
      RpcOptions& options,
      std::unique_ptr<RequestCallback> cob,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  uint32_t sendOnewayRequest(
      RpcOptions& options,
      std::unique_ptr<RequestCallback> cob,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  uint32_t sendStreamRequest(
      RpcOptions& options,
      std::unique_ptr<RequestCallback> cob,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  uint32_t sendRequestSync(
      RpcOptions& options,
      std::unique_ptr<RequestCallback> cob,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  void setCloseCallback(CloseCallback*) override {
    LOG(FATAL) << "Not supported";
  }

  folly::EventBase* getEventBase() const override {
    return nullptr;
  }

  uint16_t getProtocolId() override;

 protected:
  ~PooledRequestChannel() override = default;

  uint32_t sendRequestImpl(
      RpcKind rpcKind,
      RpcOptions& options,
      std::unique_ptr<RequestCallback> cob,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header);

 private:
  std::shared_ptr<folly::EventBase> getNextEventBase();

  PooledRequestChannel(
      folly::Executor* callbackExecutor,
      std::weak_ptr<folly::IOExecutor> executor,
      ImplCreator implCreator)
      : implCreator_(std::move(implCreator)),
        callbackExecutor_(callbackExecutor),
        executor_(std::move(executor)) {}

  Impl& impl(folly::EventBase& evb);

  ImplCreator implCreator_;

  folly::Executor* callbackExecutor_{nullptr};
  std::shared_ptr<folly::IOExecutor> executor_;
  std::atomic<size_t> nextEvbId_{0};

  folly::EventBaseLocal<Impl> impl_;

  folly::once_flag protocolIdInitFlag_;
  uint16_t protocolId_;
};
} // namespace thrift
} // namespace apache
