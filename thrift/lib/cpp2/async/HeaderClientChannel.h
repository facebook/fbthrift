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

#ifndef THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
#define THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_ 1
#include <deque>
#include <limits>
#include <memory>
#include <unordered_map>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/util/THttpParser.h>
#include <thrift/lib/cpp2/async/ChannelCallbacks.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannelTrait.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

/**
 * HeaderClientChannel
 *
 * This is a channel implementation that reads and writes
 * messages encoded using THeaderProtocol.
 */
class HeaderClientChannel : public ClientChannel,
                            public HeaderChannelTrait,
                            public MessageChannel::RecvCallback,
                            public ChannelCallbacks,
                            virtual public folly::DelayedDestruction {
 protected:
  ~HeaderClientChannel() override {}

 public:
  using ReleasableAsyncTransportPtr = std::unique_ptr<
      folly::AsyncTransport,
      folly::AsyncSocket::ReleasableDestructor>;

  explicit HeaderClientChannel(ReleasableAsyncTransportPtr transport);

  template <typename Transport>
  explicit HeaderClientChannel(
      std::unique_ptr<Transport, folly::AsyncTransport::Destructor> transport)
      : HeaderClientChannel(ReleasableAsyncTransportPtr(transport.release())) {}
  template <typename Transport>
  explicit HeaderClientChannel(
      std::unique_ptr<Transport, folly::AsyncSocket::ReleasableDestructor>
          transport)
      : HeaderClientChannel(ReleasableAsyncTransportPtr(std::move(transport))) {
  }

  struct WithRocketUpgrade {
    bool enabled{true};
  };
  struct WithoutRocketUpgrade {
    /* implicit */ operator WithRocketUpgrade() const {
      return WithRocketUpgrade{false};
    }
  };

  HeaderClientChannel(
      WithRocketUpgrade rocketUpgrade, ReleasableAsyncTransportPtr transport);
  template <typename Transport>
  explicit HeaderClientChannel(
      WithRocketUpgrade rocketUpgrade,
      std::unique_ptr<Transport, folly::AsyncTransport::Destructor> transport)
      : HeaderClientChannel(
            rocketUpgrade, ReleasableAsyncTransportPtr(transport.release())) {}
  template <typename Transport>
  explicit HeaderClientChannel(
      WithRocketUpgrade rocketUpgrade,
      std::unique_ptr<Transport, folly::AsyncSocket::ReleasableDestructor>
          transport)
      : HeaderClientChannel(
            rocketUpgrade, ReleasableAsyncTransportPtr(std::move(transport))) {}

  explicit HeaderClientChannel(const std::shared_ptr<Cpp2Channel>& cpp2Channel);

  typedef std::
      unique_ptr<HeaderClientChannel, folly::DelayedDestruction::Destructor>
          Ptr;

  template <typename TransportPtr>
  static Ptr newChannel(TransportPtr transport) {
    return Ptr(new HeaderClientChannel(std::move(transport)));
  }

  template <typename TransportPtr>
  static Ptr newChannel(
      WithRocketUpgrade rocketUpgrade, TransportPtr transport) {
    return Ptr(new HeaderClientChannel(
        std::move(rocketUpgrade), std::move(transport)));
  }

  virtual void sendMessage(
      Cpp2Channel::SendCallback* callback,
      std::unique_ptr<folly::IOBuf> buf,
      apache::thrift::transport::THeader* header) {
    cpp2Channel_->sendMessage(callback, std::move(buf), header);
  }

  void closeNow() override;

  // DelayedDestruction methods
  void destroy() override;

  folly::AsyncTransport* getTransport() override {
    if (isUpgradedToRocket()) {
      return rocketChannel_->getTransport();
    }
    return cpp2Channel_->getTransport();
  }

  /**
   * Steal the transport (AsyncSocket) from this channel.
   * NOTE: This is for transport upgrade from header to rocket for non-TLS
   * services.
   */
  folly::AsyncTransport::UniquePtr stealTransport() {
    auto transportShared = cpp2Channel_->getTransportShared();
    cpp2Channel_->setTransport(nullptr);
    cpp2Channel_->closeNow();
    assert(transportShared.use_count() == 1);
    auto deleter = std::get_deleter<folly::AsyncSocket::ReleasableDestructor>(
        transportShared);
    deleter->release();
    return folly::AsyncTransport::UniquePtr(transportShared.get());
  }

  /**
   * Sets the optional RequestSetupMetadata for transport upgrade from header
   * to rocket. If this is provided, then the upgrade mechanism will call
   * `RocketClientChannel::newChannelWithMetadata` instead of
   * `RocketClientChannel::newChannel`.
   * NOTE: This is for transport upgrade from header to rocket for non-TLS
   * services.
   */
  void setRocketUpgradeSetupMetadata(
      apache::thrift::RequestSetupMetadata rocketRequestSetupMetadata) {
    CHECK(
        upgradeState_.load(std::memory_order_acquire) ==
        RocketUpgradeState::INIT);
    rocketRequestSetupMetadata_ =
        std::make_unique<apache::thrift::RequestSetupMetadata>(
            std::move(rocketRequestSetupMetadata));
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    cpp2Channel_->setReadBufferSize(readBufferSize);
  }

  // Client interface from RequestChannel
  using RequestChannel::sendRequestNoResponse;
  using RequestChannel::sendRequestResponse;

  void sendRequestResponse(
      const RpcOptions&,
      ManagedStringView&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override;

  void sendRequestNoResponse(
      const RpcOptions&,
      ManagedStringView&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override;

  void setCloseCallback(CloseCallback*) override;

  // Interface from MessageChannel::RecvCallback
  void messageReceived(
      std::unique_ptr<folly::IOBuf>&&,
      std::unique_ptr<apache::thrift::transport::THeader>&&) override;
  void messageChannelEOF() override;
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override;

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms) override;
  uint32_t getTimeout() override {
    if (isUpgradedToRocket()) {
      return rocketChannel_->getTimeout();
    }
    return getTransport()->getSendTimeout();
  }

  // If a Close Callback is set, should we reregister callbacks for it
  // alone?  Basically, this means that loop() will return if the only thing
  // outstanding is close callbacks.
  void setKeepRegisteredForClose(bool keepRegisteredForClose) {
    keepRegisteredForClose_ = keepRegisteredForClose;
    setBaseReceivedCallback();
  }

  bool getKeepRegisteredForClose() { return keepRegisteredForClose_; }

  folly::EventBase* getEventBase() const override {
    if (isUpgradedToRocket()) {
      return rocketChannel_->getEventBase();
    }
    return cpp2Channel_->getEventBase();
  }

  /**
   * Set the channel up in HTTP CLIENT mode. host can be an empty string.
   *
   * Note: this needs to be called before sending first request due to the
   * possibility of the channel upgrading itself to rocket.
   */
  void useAsHttpClient(const std::string& host, const std::string& uri);

  bool good() override;

  SaturationStatus getSaturationStatus() override {
    if (isUpgradedToRocket()) {
      return rocketChannel_->getSaturationStatus();
    }
    return SaturationStatus(0, std::numeric_limits<uint32_t>::max());
  }

  // event base methods
  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;

  uint16_t getProtocolId() override;
  void setProtocolId(uint16_t protocolId) {
    if (isUpgradedToRocket()) {
      rocketChannel_->setProtocolId(protocolId);
    } else {
      protocolId_ = protocolId;
    }
  }

  bool expireCallback(uint32_t seqId);

  CLIENT_TYPE getClientType() override {
    if (isUpgradedToRocket()) {
      return rocketChannel_->getClientType();
    }
    return HeaderChannelTrait::getClientType();
  }

  void setOnDetachable(folly::Function<void()> onDetachable) override {
    if (isUpgradedToRocket()) {
      rocketChannel_->setOnDetachable(std::move(onDetachable));
    } else {
      ClientChannel::setOnDetachable(std::move(onDetachable));
    }
  }
  void unsetOnDetachable() override {
    if (isUpgradedToRocket()) {
      rocketChannel_->unsetOnDetachable();
    } else {
      ClientChannel::unsetOnDetachable();
    }
  }

  class ClientFramingHandler : public FramingHandler {
   public:
    explicit ClientFramingHandler(HeaderClientChannel& channel)
        : channel_(channel) {}

    std::tuple<
        std::unique_ptr<folly::IOBuf>,
        size_t,
        std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::transport::THeader* header) override;

   private:
    HeaderClientChannel& channel_;
  };

  // Remove a callback from the recvCallbacks_ map.
  void eraseCallback(uint32_t seqId, TwowayCallback<HeaderClientChannel>* cb);

  void setConnectionAgentName(std::string_view name);

 protected:
  bool clientSupportHeader() override;

 private:
  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);
  void attachMetadataOnce(apache::thrift::transport::THeader* header);

  // Transport upgrade from header to rocket for raw header client. If
  // successful, this HeaderClientChannel will manage a RocketClientChannel
  // internally and send/receive messages through the rocket channel.
  void tryUpgradeTransportToRocket(std::chrono::milliseconds timeout);

  std::shared_ptr<apache::thrift::util::THttpClientParser> httpClientParser_;

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  uint32_t sendSeqId_;

  std::unordered_map<uint32_t, TwowayCallback<HeaderClientChannel>*>
      recvCallbacks_;
  std::deque<uint32_t> recvCallbackOrder_;
  CloseCallback* closeCallback_;

  uint32_t timeout_;

  bool keepRegisteredForClose_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  uint16_t protocolId_;

  std::string agentName_;
  bool firstRequest_{true};

  // If true, on first request this HeaderClientChannel will try to upgrade to
  // use rocket transport.
  bool upgradeToRocket_;
  // If rocket transport upgrade is enabled, HeaderClientChannel manages a
  // rocket channel internally and uses this rocket channel for all
  // requests/response handling.
  RocketClientChannel::Ptr rocketChannel_;
  // The metadata that will be passed to the RocketClientChannel during
  // transport upgrade.
  std::unique_ptr<RequestSetupMetadata> rocketRequestSetupMetadata_;

  enum class RocketUpgradeState {
    NO_UPGRADE = 0,
    INIT = 1,
    IN_PROGRESS = 2,
    DONE = 3
  };
  std::atomic<RocketUpgradeState> upgradeState_;

  bool isUpgradedToRocket() const {
    return upgradeState_.load(std::memory_order_acquire) ==
        RocketUpgradeState::DONE &&
        rocketChannel_;
  }

  // A class to hold the necessary data for a header request
  // (SerializedRequest, THeader, RpcOptions, callback, etc.) so that the
  // request can be queued and sent at a later point.
  class HeaderRequestContext {
   public:
    HeaderRequestContext(
        const RpcOptions& rpcOptions,
        ManagedStringView&& methodName,
        SerializedRequest&& serializedRequest,
        std::shared_ptr<apache::thrift::transport::THeader> header,
        RequestClientCallback::Ptr cb,
        bool oneWay)
        : rpcOptions_(rpcOptions),
          methodName_(std::move(methodName)),
          serializedRequest_(std::move(serializedRequest)),
          header_(std::move(header)),
          callback_(std::move(cb)),
          oneWay_(oneWay) {}

    const RpcOptions rpcOptions_;
    ManagedStringView methodName_;
    SerializedRequest serializedRequest_;
    std::shared_ptr<apache::thrift::transport::THeader> header_;
    RequestClientCallback::Ptr callback_;
    bool oneWay_;
  };

  /**
   * A container to hold the header requests that come in while transport
   * upgrade from header to rocket is in progress.
   *
   * If transport upgrade for raw client is enabled, this HeaderClientChannel
   * tries to upgrade the transport from header to rocket upon first request
   * by sending a special request (upgradeToRocket). If upgrade succeeds, all
   * requests on the channel (including the ones that come in while upgrade
   * was in progress) should be sent using rocket transport.
   */
  std::deque<HeaderRequestContext> pendingRequests_;

  class RocketUpgradeCallback;
  friend class TransportUpgradeTest;
  friend class TransportUpgradeTest_RawClientRocketUpgradeOneway_Test;
  friend class TransportUpgradeTest_RawClientNoUpgrade_Test;
  friend class TransportUpgradeTest_RawClientRocketUpgradeTimeout_Test;
};

} // namespace thrift
} // namespace apache

#endif // THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
