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

#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/async/DelayedDestruction.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscriber.h>

#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
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

  static Ptr newChannel(async::TAsyncTransport::UniquePtr socket);

  uint32_t sendRequest(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  uint32_t sendOnewayRequest(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header) override;

  uint32_t sendStreamRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<transport::THeader>) override;

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
    inflightState_->setMaxInflightRequests(n);
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

  class InflightState {
   public:
    explicit InflightState(folly::Function<void()> onDetachable)
        : onDetachable_(std::move(onDetachable)) {}

    bool incPendingRequests() {
      if (inflightRequests_ >= maxInflightRequests_) {
        return false;
      }
      ++inflightRequests_;
      return true;
    }

    void decPendingRequests() {
      if (!--inflightRequests_ && onDetachable_) {
        onDetachable_();
      }
    }

    uint32_t inflightRequests() const {
      return inflightRequests_;
    }
    uint32_t maxInflightRequests() const {
      return maxInflightRequests_;
    }
    void setMaxInflightRequests(uint32_t n) {
      maxInflightRequests_ = n;
    }
    void unsetOnDetachable() {
      onDetachable_ = nullptr;
    }

   private:
    uint32_t inflightRequests_{0};
    uint32_t maxInflightRequests_{std::numeric_limits<uint32_t>::max()};
    folly::Function<void()> onDetachable_;
  };

  const std::shared_ptr<InflightState> inflightState_{
      std::make_shared<InflightState>([this] {
        DCHECK(!evb_ || evb_->isInEventBaseThread());
        if (isDetachable()) {
          notifyDetachable();
        }
      })};

  explicit RocketClientChannel(async::TAsyncTransport::UniquePtr socket);

  RocketClientChannel(const RocketClientChannel&) = delete;
  RocketClientChannel& operator=(const RocketClientChannel&) = delete;

  virtual ~RocketClientChannel();

  void sendThriftRequest(
      RpcOptions& rpcOptions,
      RpcKind kind,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header);

  void sendSingleRequestNoResponse(
      const RequestRpcMetadata& metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb);

  void sendSingleRequestSingleResponse(
      const RequestRpcMetadata& metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb);

  void sendSingleRequestStreamResponse(
      const RequestRpcMetadata& metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb,
      std::chrono::milliseconds chunkTimeout);

  RequestRpcMetadata makeRequestRpcMetadata(
      RpcOptions& rpcOptions,
      RpcKind kind,
      ProtocolId protocolId,
      transport::THeader* header);

  folly::fibers::FiberManager& getFiberManager() const {
    DCHECK(evb_);
    return folly::fibers::getFiberManager(*evb_);
  }

 public:
  // Helper class that gives special handling to the first payload on the
  // response stream. Public only for testing purposes.
  class TakeFirst
      : public yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>,
        public yarpl::flowable::Subscriber<rocket::Payload> {
   public:
    TakeFirst(
        folly::EventBase& evb,
        std::unique_ptr<ThriftClientCallback> clientCallback,
        std::chrono::milliseconds chunkTimeout,
        std::weak_ptr<InflightState> inflightState);
    ~TakeFirst() override;
    void cancel();

   private:
    using T = rocket::Payload;
    using U = std::unique_ptr<folly::IOBuf>;

    bool awaitingFirstResponse_{true};
    bool completeBeforeSubscribed_{false};
    folly::exception_wrapper errorBeforeSubscribed_;

    std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber_;
    std::shared_ptr<yarpl::flowable::Subscription> subscription_;

    folly::EventBase& evb_;
    std::unique_ptr<ThriftClientCallback> clientCallback_;
    const std::chrono::milliseconds chunkTimeout_;
    std::weak_ptr<InflightState> inflightWeak_;

    void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<U>>) final;
    void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>) final;

    void onNext(T) final;
    void onError(folly::exception_wrapper ew) final;
    void onComplete() final;

   protected:
    virtual void onNormalFirstResponse(
        T&& firstPayload,
        std::shared_ptr<Flowable<U>> tail);
    virtual void onErrorFirstResponse(folly::exception_wrapper ew);
    virtual void onStreamTerminated();
  };
};

} // namespace thrift
} // namespace apache
