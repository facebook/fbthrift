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
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSRequester.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

namespace detail {
class RSConnectionStatus : public rsocket::RSocketConnectionEvents {
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
  ChannelCounters();
  void setMaxPendingRequests(uint32_t);
  uint32_t getMaxPendingRequests();
  uint32_t getPendingRequests();
  bool incPendingRequests();
  void decPendingRequests();

 private:
  uint32_t maxPendingRequests_;
  uint32_t pendingRequests_{0u};
};
} // namespace detail

class RSocketClientChannel : public ClientChannel,
                             public detail::ChannelCounters {
 public:
  using Ptr = std::
      unique_ptr<RSocketClientChannel, folly::DelayedDestruction::Destructor>;

  static Ptr newChannel(
      apache::thrift::async::TAsyncTransport::UniquePtr socket,
      bool isSecure = false);

  void setProtocolId(uint16_t protocolId);
  void setHTTPHost(const std::string& host);
  void setHTTPUrl(const std::string& url);

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
  bool isSecurityActive() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;
  void setMaxPendingRequests(uint32_t num);

  static std::unique_ptr<folly::IOBuf> serializeMetadata(
      const RequestRpcMetadata& requestMetadata);

  static std::unique_ptr<ResponseRpcMetadata> deserializeMetadata(
      const folly::IOBuf& buffer);

 private:
  RSocketClientChannel(
      apache::thrift::async::TAsyncTransport::UniquePtr socket,
      bool isSecure = false);

  RSocketClientChannel(const RSocketClientChannel&) = delete;
  RSocketClientChannel& operator=(const RSocketClientChannel&) = delete;

  virtual ~RSocketClientChannel();

  std::unique_ptr<RequestRpcMetadata> createRequestRpcMetadata(
      RpcOptions& rpcOptions,
      RpcKind kind,
      apache::thrift::ProtocolId protocolId,
      transport::THeader* header);

  uint32_t sendRequestHelper(
      RpcOptions& rpcOptions,
      RpcKind kind,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) noexcept;

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendSingleRequestNoResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendSingleRequestResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendStreamRequestStreamResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendSingleRequestStreamResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  std::shared_ptr<RSRequester> getRequester();

 private:
  // The default timeout for a Thrift RPC.
  static const std::chrono::milliseconds kDefaultRpcTimeout;

  folly::EventBase* evb_;
  bool isSecure_;

  std::string httpHost_;
  std::string httpUrl_;
  uint16_t protocolId_{apache::thrift::protocol::T_BINARY_PROTOCOL};

  std::shared_ptr<detail::RSConnectionStatus> connectionStatus_;
  std::shared_ptr<RSRequester> rsRequester_;
  std::chrono::milliseconds timeout_{ThriftClientCallback::kDefaultTimeout};

  std::unique_ptr<apache::thrift::detail::ChannelCounters> channelCounters_;
};

} // namespace thrift
} // namespace apache
