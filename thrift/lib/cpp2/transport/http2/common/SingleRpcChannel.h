/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/http2/common/H2ChannelIf.h>

#include <folly/FixedString.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <string>

namespace apache {
namespace thrift {

// These will go away once we sent the RPC metadata directly.  "src"
// stands for SingleRpcChannel.
constexpr auto kProtocolKey = folly::makeFixedString("src_protocol");
constexpr auto kRpcNameKey = folly::makeFixedString("src_rpc_name");
constexpr auto kRpcKindKey = folly::makeFixedString("src_rpc_kind");
constexpr auto kErrorKindKey = folly::makeFixedString("src_error_kind");
constexpr auto kErrorMessageKey = folly::makeFixedString("src_error_reason");

// This will go away once we have deprecated all uses of the existing
// HTTP2 support (HTTPClientChannel and the matching server
// implementation).
constexpr auto kHttpClientChannelKey =
    folly::makeFixedString("src_httpclientchannel");

class SingleRpcChannel : public H2ChannelIf {
 public:
  SingleRpcChannel(
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor);

  SingleRpcChannel(
      H2ClientConnection* toHttp2,
      const std::string& httpHost,
      const std::string& httpUrl);

  ~SingleRpcChannel() override;

  bool supportsHeaders() const noexcept override;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload) noexcept override;

  void cancel(int32_t seqId) noexcept override;

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  void cancel(ThriftClientCallback* callback) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

  void setInput(int32_t seqId, SubscriberRef sink) noexcept override;

  SubscriberRef getOutput(int32_t seqId) noexcept override;

  void onH2StreamBegin(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  void onH2BodyFrame(std::unique_ptr<folly::IOBuf> contents) noexcept override;

  void onH2StreamEnd() noexcept override;

  void onH2StreamClosed(proxygen::ProxygenError) noexcept override;

 private:
  // The service side handling code for onH2StreamEnd().
  void onThriftRequest() noexcept;

  // The client side handling code for onH2StreamEnd().
  void onThriftResponse() noexcept;

  // TODO: This method will go away once we serialize the metadata directly.
  bool extractEnvelopeInfoFromHeader(RequestRpcMetadata* metadata) noexcept;

  void extractHeaderInfo(RequestRpcMetadata* metadata) noexcept;

  // Called from onThriftRequest() to send an error response.
  void sendErrorThriftResponse(
      ErrorKind error,
      const std::string& message) noexcept;

  // The thrift processor used to execute RPCs (server side only).
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_{nullptr};

  // Header information for RPCs (client side only).
  std::string httpHost_;
  std::string httpUrl_;
  // Callback for client side.
  std::unique_ptr<ThriftClientCallback> callback_;
  // Transaction object for use on client side to communicate with the
  // Proxygen layer.
  proxygen::HTTPTransaction* httpTransaction_;

  std::unique_ptr<std::map<std::string, std::string>> headers_;
  std::unique_ptr<folly::IOBuf> contents_;
  bool receivedH2Stream_{false};
  bool receivedThriftRPC_{false};
  // Event base on which all methods in this object must be invoked.
  folly::EventBase* evb_;
};

} // namespace thrift
} // namespace apache
