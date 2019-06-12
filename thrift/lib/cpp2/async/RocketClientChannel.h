/*
 * Copyright 2015-present Facebook, Inc.
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

#include <chrono>
#include <limits>
#include <memory>

#include <folly/ScopeGuard.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/async/DelayedDestruction.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscriber.h>

#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace folly {
class EventBase;
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {

class ContextStack;
class RequestCallback;
class RpcOptions;
class StreamClientCallback;
class ThriftClientCallback;

namespace rocket {
class Payload;
class RocketClient;
} // namespace rocket

namespace transport {
class THeader;
} // namespace transport

class RocketClientChannel final : public ClientChannel {
 public:
  using Ptr = std::
      unique_ptr<RocketClientChannel, folly::DelayedDestruction::Destructor>;

  static Ptr newChannel(
      async::TAsyncTransport::UniquePtr socket,
      RequestSetupMetadata meta = RequestSetupMetadata());

  void sendRequestResponse(
      RpcOptions& rpcOptions,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cb) override;

  void sendRequestNoResponse(
      RpcOptions& rpcOptions,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr cb) override;

  void sendRequestStream(
      RpcOptions& rpcOptions,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* clientCallback) override;

  using RequestChannel::sendRequestStream;

  folly::EventBase* getEventBase() const override {
    return evb_;
  }

  uint16_t getProtocolId() override {
    return protocolId_;
  }

  void setProtocolId(uint16_t protocolId) {
    protocolId_ = protocolId;
  }

  async::TAsyncTransport* FOLLY_NULLABLE getTransport() override;
  bool good() override;

  size_t inflightRequestsAndStreams() const;

  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;
  void setOnDetachable(folly::Function<void()> onDetachable) override;
  void unsetOnDetachable() override;

  uint32_t getTimeout() override {
    return timeout_.count();
  }
  void setTimeout(uint32_t timeoutMs) override;

  CLIENT_TYPE getClientType() override {
    // TODO Create a new client type
    return THRIFT_HTTP_CLIENT_TYPE;
  }

  void setMaxPendingRequests(uint32_t n) {
    maxInflightRequestsAndStreams_ = n;
  }
  SaturationStatus getSaturationStatus() override;

  void closeNow() override;
  void setCloseCallback(CloseCallback* closeCallback) override;

 private:
  static constexpr std::chrono::milliseconds kDefaultRpcTimeout{500};

  folly::EventBase* evb_{nullptr};
  std::shared_ptr<rocket::RocketClient> rclient_;
  uint16_t protocolId_{apache::thrift::protocol::T_BINARY_PROTOCOL};
  std::chrono::milliseconds timeout_{kDefaultRpcTimeout};

  uint32_t maxInflightRequestsAndStreams_{std::numeric_limits<uint32_t>::max()};
  struct Shared {
    uint32_t inflightRequests{0};
  };
  const std::shared_ptr<Shared> shared_{std::make_shared<Shared>()};

  RocketClientChannel(
      async::TAsyncTransport::UniquePtr socket,
      RequestSetupMetadata meta);

  RocketClientChannel(const RocketClientChannel&) = delete;
  RocketClientChannel& operator=(const RocketClientChannel&) = delete;

  virtual ~RocketClientChannel();

  void sendThriftRequest(
      RpcOptions& rpcOptions,
      RpcKind kind,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      RequestClientCallback::Ptr cb);

  void sendSingleRequestNoResponse(
      const RequestRpcMetadata& metadata,
      std::unique_ptr<folly::IOBuf> buf,
      RequestClientCallback::Ptr cb);

  void sendSingleRequestSingleResponse(
      const RequestRpcMetadata& metadata,
      std::chrono::milliseconds timeout,
      std::unique_ptr<folly::IOBuf> buf,
      RequestClientCallback::Ptr cb);

  folly::fibers::FiberManager& getFiberManager() const {
    DCHECK(evb_);
    return folly::fibers::getFiberManager(*evb_);
  }

  rocket::SetupFrame makeSetupFrame(RequestSetupMetadata meta);

  auto inflightGuard() {
    ++shared_->inflightRequests;
    return folly::makeGuard([shared = shared_] { --shared->inflightRequests; });
  }

};

} // namespace thrift
} // namespace apache
