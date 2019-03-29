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

#include <memory>

#include <folly/Function.h>

#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace folly {
class EventBase;
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {

class AsyncProcessor;

namespace rocket {

class Payload;

class RocketServerFrameContext;
class RocketServerStreamSubscriber;

// Object corresponding to rsocket REQUEST_RESPONSE request (single
// request-single response) handled by Thrift server
class ThriftServerRequestResponse final : public ThriftRequestCore {
 public:
  ThriftServerRequestResponse(
      folly::EventBase& evb,
      const server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      RocketServerFrameContext&& context);

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept override;

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

  bool isStream() const override {
    return false;
  }

  bool isReplyChecksumNeeded() const override {
    return true;
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
      folly::EventBase& evb,
      const server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      RocketServerFrameContext&& context,
      folly::Function<void()> onComplete);

  ~ThriftServerRequestFnf() override;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept override;

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
      folly::EventBase& evb,
      const server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      std::shared_ptr<RocketServerStreamSubscriber> subscriber,
      std::shared_ptr<AsyncProcessor> cpp2Processor);

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept override;

  folly::EventBase* getEventBase() noexcept override {
    return &evb_;
  }

  bool isStream() const override {
    return true;
  }

  bool isReplyChecksumNeeded() const override {
    return true;
  }

 private:
  folly::EventBase& evb_;
  std::shared_ptr<RocketServerStreamSubscriber> subscriber_;
  const std::shared_ptr<AsyncProcessor> cpp2Processor_;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
