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

namespace apache {
namespace thrift {

class SingleRpcChannel : public H2ChannelIf {
 public:
  SingleRpcChannel(
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor);

  SingleRpcChannel(
      proxygen::HTTPTransaction* toHttp2,
      const std::string& httpHost,
      const std::string& httpUrl);

  ~SingleRpcChannel() override;

  bool supportsHeaders() const noexcept override;

  void sendThriftResponse(
      uint32_t seqId,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload) noexcept override;

  void cancel(uint32_t seqId) noexcept override;

  void sendThriftRequest(
      std::unique_ptr<FunctionInfo> functionInfo,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  void cancel(ThriftClientCallback* callback) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

  void setInput(uint32_t seqId, SubscriberRef sink) noexcept override;

  SubscriberRef getOutput(uint32_t seqId) noexcept override;

  void onH2StreamBegin(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  void onH2BodyFrame(std::unique_ptr<folly::IOBuf> contents) noexcept override;

  void onH2StreamEnd() noexcept override;

  void onH2StreamClosed() noexcept override;

 private:
  // TODO: Temporary method until we add envelope to payload.
  bool isOneWay() noexcept;

  // The thrift processor used to execute RPCs (server side only).
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_;

  // Header information for RPCs (client side only).
  std::string httpHost_;
  std::string httpUrl_;
  // Callback for client side.
  std::unique_ptr<ThriftClientCallback> callback_;

  std::unique_ptr<std::map<std::string, std::string>> headers_;
  std::unique_ptr<folly::IOBuf> contents_;
  bool receivedH2Stream_{false};
  bool receivedThriftRPC_{false};
  // Only used for checks.
  // TODO: delete this and get the event base from the underlying connection.
  // Because the event base on the connection could change.
  folly::EventBase* evb_;
};

} // namespace thrift
} // namespace apache
