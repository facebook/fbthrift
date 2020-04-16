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

#include <memory>

#include <folly/Portability.h>

#include <folly/Function.h>

#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace folly {
class EventBase;
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {

class AsyncProcessor;
class RocketSinkClientCallback;
class RocketStreamClientCallback;

namespace rocket {

class Payload;
class RocketServerFrameContext;

// Object corresponding to rsocket REQUEST_RESPONSE request (single
// request-single response) handled by Thrift server
class ThriftServerRequestResponse final : public ThriftRequestCore {
 public:
  ThriftServerRequestResponse(
      RequestsRegistry::DebugStub& debugStubToInit,
      folly::EventBase& evb,
      server::ServerConfigs& serverConfigs,
      RequestRpcMetadata&& metadata,
      Cpp2ConnContext& connContext,
      RequestsRegistry& reqRegistry,
      std::unique_ptr<folly::IOBuf> debugPayload,
      intptr_t rootRequestContextId,
      RocketServerFrameContext&& context);

  void sendThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::MessageChannel::SendCallback*) noexcept override;

  void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept override;

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

  bool isStream() const override {
    return false;
  }

 private:
  folly::EventBase& evb_;
  RocketServerFrameContext context_;
};

// Object corresponding to rsocket REQUEST_FNF request (one-way request) handled
// by Thrift server
class ThriftServerRequestFnf final : public ThriftRequestCore {
 public:
  ThriftServerRequestFnf(
      RequestsRegistry::DebugStub& debugStubToInit,
      folly::EventBase& evb,
      server::ServerConfigs& serverConfigs,
      RequestRpcMetadata&& metadata,
      Cpp2ConnContext& connContext,
      RequestsRegistry& reqRegistry,
      std::unique_ptr<folly::IOBuf> debugPayload,
      intptr_t rootRequestContextId,
      RocketServerFrameContext&& context,
      folly::Function<void()> onComplete);

  ~ThriftServerRequestFnf() override;

  void sendThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::MessageChannel::SendCallback*) noexcept override;

  void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept override;

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

 private:
  folly::EventBase& evb_;
  RocketServerFrameContext context_;
  folly::Function<void()> onComplete_;
};

// Object corresponding to rsocket REQUEST_STREAM request (initial request to
// establish stream) handled by Thrift server
class ThriftServerRequestStream final : public ThriftRequestCore {
 public:
  ThriftServerRequestStream(
      RequestsRegistry::DebugStub& debugStubToInit,
      folly::EventBase& evb,
      server::ServerConfigs& serverConfigs,
      RequestRpcMetadata&& metadata,
      Cpp2ConnContext& connContext,
      RequestsRegistry& reqRegistry,
      std::unique_ptr<folly::IOBuf> debugPayload,
      intptr_t rootRequestContextId,
      RocketServerFrameContext&& context,
      RocketStreamClientCallback* clientCallback,
      std::shared_ptr<AsyncProcessor> cpp2Processor);

  void sendThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::MessageChannel::SendCallback*) noexcept override;

  using ThriftRequestCore::sendStreamReply;

  void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept override;

  bool sendStreamThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      StreamServerCallbackPtr) noexcept override;

  virtual void sendStreamThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      ::apache::thrift::detail::ServerStreamFactory&&) noexcept override;

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

  bool isStream() const override {
    return true;
  }

 private:
  folly::EventBase& evb_;
  RocketServerFrameContext context_;
  RocketStreamClientCallback* clientCallback_;

  const std::shared_ptr<AsyncProcessor> cpp2Processor_;
};

// Object corresponding to rsocket sink (REQUEST_CHANNEL) request (initial
// request to establish stream) handled by Thrift server
class ThriftServerRequestSink final : public ThriftRequestCore {
 public:
  ThriftServerRequestSink(
      RequestsRegistry::DebugStub& debugStubToInit,
      folly::EventBase& evb,
      server::ServerConfigs& serverConfigs,
      RequestRpcMetadata&& metadata,
      Cpp2ConnContext& connContext,
      RequestsRegistry& reqRegistry,
      std::unique_ptr<folly::IOBuf> debugPayload,
      intptr_t rootRequestContextId,
      RocketServerFrameContext&& context,
      RocketSinkClientCallback* clientCallback,
      std::shared_ptr<AsyncProcessor> cpp2Processor);

  void sendThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::MessageChannel::SendCallback*) noexcept override;

  void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept override;

#if FOLLY_HAS_COROUTINES
  void sendSinkThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::detail::SinkConsumerImpl&&) noexcept override;
#endif

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

  bool isSink() const override {
    return true;
  }

  bool isReplyChecksumNeeded() const override {
    return true;
  }

 private:
  folly::EventBase& evb_;
  RocketServerFrameContext context_;
  RocketSinkClientCallback* clientCallback_;

  const std::shared_ptr<AsyncProcessor> cpp2Processor_;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
