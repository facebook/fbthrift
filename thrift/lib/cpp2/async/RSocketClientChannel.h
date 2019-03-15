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

#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <rsocket/RSocketConnectionEvents.h>
#include <rsocket/statemachine/RSocketStateMachine.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ChannelCallbacks.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class RSocketClientChannel;

namespace detail {
class RSConnectionStatus final : public rsocket::RSocketConnectionEvents {
 public:
  void setCloseCallback(CloseCallback* ccb);

  bool isConnected() const;

 private:
  void onConnected() override;
  void onDisconnected(const folly::exception_wrapper&) override;
  void onClosed(const folly::exception_wrapper&) override;

  void closed();

  bool isConnected_{false};
  CloseCallback* closeCallback_{nullptr};
};

class ChannelCounters {
 public:
  explicit ChannelCounters(folly::Function<void()> onDetachable);
  void setMaxPendingRequests(uint32_t);
  uint32_t getMaxPendingRequests();
  uint32_t getPendingRequests();
  bool incPendingRequests();
  void decPendingRequests();

  void unsetOnDetachable();

 private:
  uint32_t maxPendingRequests_;
  uint32_t pendingRequests_{0u};
  folly::Function<void()> onDetachable_;
};

class TakeFirst final
    : public yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>,
      public yarpl::flowable::Subscriber<rsocket::Payload> {
  using T = rsocket::Payload;
  using U = std::unique_ptr<folly::IOBuf>;

 public:
  TakeFirst(
      folly::Function<void()>,
      folly::Function<void(std::pair<T, std::shared_ptr<Flowable<U>>>)>,
      folly::Function<void(folly::exception_wrapper)>,
      folly::Function<void()>);
  virtual ~TakeFirst();

  void cancel();

 protected:
  void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<U>>) override;
  void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>) override;
  void onNext(T) override;
  void onError(folly::exception_wrapper) override;
  void onComplete() override;
  void onTerminal();

 protected:
  folly::Function<void()> onRequestSent_;
  folly::Function<void(std::pair<T, std::shared_ptr<Flowable<U>>>)> onResponse_;
  folly::Function<void(folly::exception_wrapper)> onError_;
  folly::Function<void()> onTerminal_;

  bool isFirstResponse_{true};
  bool completed_{false};
  folly::exception_wrapper error_;

  std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber_;
  std::shared_ptr<yarpl::flowable::Subscription> subscription_;
};
} // namespace detail

class RSocketClientChannel final : public ClientChannel,
                                   public ChannelCallbacks {
 public:
  using Ptr = std::
      unique_ptr<RSocketClientChannel, folly::DelayedDestruction::Destructor>;

  using ChannelCallbacks::OnewayCallback;

  static Ptr newChannel(async::TAsyncTransport::UniquePtr socket);

  void setProtocolId(uint16_t protocolId);

  uint32_t sendRequest(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) override;

  uint32_t sendOnewayRequest(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) override;

  uint32_t sendStreamRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  folly::EventBase* getEventBase() const override;

  uint16_t getProtocolId() override;

  void setCloseCallback(CloseCallback* cb) override;

  apache::thrift::async::TAsyncTransport* FOLLY_NULLABLE
  getTransport() override;
  bool good() override;
  SaturationStatus getSaturationStatus() override;
  void attachEventBase(folly::EventBase* eventBase) override;
  void detachEventBase() override;
  bool isDetachable() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;
  void setMaxPendingRequests(uint32_t num);

  static std::unique_ptr<folly::IOBuf> serializeMetadata(
      const RequestRpcMetadata& requestMetadata);

 private:
  explicit RSocketClientChannel(async::TAsyncTransport::UniquePtr socket);

  RSocketClientChannel(const RSocketClientChannel&) = delete;
  RSocketClientChannel& operator=(const RSocketClientChannel&) = delete;

  virtual ~RSocketClientChannel();

  void sendThriftRequest(
      RpcOptions& rpcOptions,
      RpcKind kind,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) noexcept;

  void sendSingleRequestNoResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb) noexcept;

  void sendSingleRequestSingleResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb) noexcept;

  void sendSingleRequestStreamResponse(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<RequestCallback> cb) noexcept;

 public:
  static const uint16_t kMajorVersion;
  static const uint16_t kMinorVersion;

 private:
  // The default timeout for a Thrift RPC.
  static constexpr std::chrono::milliseconds kDefaultRpcTimeout{
      std::chrono::milliseconds(500)};

  folly::EventBase* evb_;

  uint16_t protocolId_{apache::thrift::protocol::T_BINARY_PROTOCOL};

  std::shared_ptr<detail::RSConnectionStatus> connectionStatus_;
  std::shared_ptr<rsocket::RSocketStateMachine> stateMachine_;
  std::chrono::milliseconds timeout_{RSocketClientChannel::kDefaultRpcTimeout};

  std::shared_ptr<apache::thrift::detail::ChannelCounters> channelCounters_;
};

} // namespace thrift
} // namespace apache
