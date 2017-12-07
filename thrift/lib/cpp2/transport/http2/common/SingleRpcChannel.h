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

#include <thrift/lib/cpp2/transport/http2/common/H2Channel.h>

#include <folly/FixedString.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <string>

namespace apache {
namespace thrift {

class SingleRpcChannel : public H2Channel {
 public:
  SingleRpcChannel(
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor,
      bool legacySupport);

  SingleRpcChannel(H2ClientConnection* toHttp2, bool legacySupport);

  virtual ~SingleRpcChannel() override;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload) noexcept override;

  virtual void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

  void onH2StreamBegin(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  void onH2BodyFrame(std::unique_ptr<folly::IOBuf> contents) noexcept override;

  void onH2StreamEnd() noexcept override;

  void onH2StreamClosed(proxygen::ProxygenError) noexcept override;

  void setNotYetStable() noexcept;

 private:
  // The server side handling code for onH2StreamEnd().
  virtual void onThriftRequest() noexcept;

  // The client side handling code for onH2StreamEnd().
  void onThriftResponse() noexcept;

  void extractHeaderInfo(RequestRpcMetadata* metadata) noexcept;

  // Called from onThriftRequest() to send an error response.
  void sendThriftErrorResponse(
      const std::string& message,
      ProtocolId protoId = ProtocolId::COMPACT,
      const std::string& name = "process") noexcept;

  // The thrift processor used to execute RPCs (server side only).
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_{nullptr};

  // Event base on which all methods in this object must be invoked.
  folly::EventBase* evb_;

  // Set to true to support the legacy HTTP2 protocol.
  bool legacySupport_;

  // Transaction object for use on client side to communicate with the
  // Proxygen layer.
  proxygen::HTTPTransaction* httpTransaction_;

  // Callback for client side.
  std::unique_ptr<ThriftClientCallback> callback_;

  std::unique_ptr<proxygen::HTTPMessage> headers_;
  std::unique_ptr<folly::IOBuf> contents_;

  bool receivedThriftRPC_{false};
  bool receivedH2Stream_{false};

  // This flag is initialized when the channel is constructed to
  // decide whether or not the connection should be set to stable
  // (i.e., we know that the server does not have to read the header
  // any more to determine the channel version).  This flag is set to
  // true if the connection is not yet stable and negotiation has
  // completed on the client side.  If this channel completes a
  // successful RPC, it means the server now knows that the client has
  // completed negotiation, and so it can set the connection to be
  // stable.
  bool shouldMakeStable_;
};

} // namespace thrift
} // namespace apache
