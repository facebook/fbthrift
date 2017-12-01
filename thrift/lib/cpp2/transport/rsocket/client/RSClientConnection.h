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

#include <rsocket/RSocket.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSRequester.h>

namespace apache {
namespace thrift {
class RSConnectionStatus;

class RSClientConnection : public ClientConnectionIf {
 public:
  explicit RSClientConnection(
      apache::thrift::async::TAsyncTransport::UniquePtr socket,
      bool isSecure = false);

  std::shared_ptr<ThriftChannelIf> getChannel(
      RequestRpcMetadata* metadata) override;
  void setMaxPendingRequests(uint32_t num) override;
  void setCloseCallback(ThriftClient*, CloseCallback* cb) override;
  folly::EventBase* getEventBase() const override;
  apache::thrift::async::TAsyncTransport* FOLLY_NULLABLE
  getTransport() override;
  bool good() override;
  ClientChannel::SaturationStatus getSaturationStatus() override;
  void attachEventBase(folly::EventBase* evb) override;
  void detachEventBase() override;
  bool isDetachable() override;
  bool isSecurityActive() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;

 private:
  folly::EventBase* evb_;

  std::shared_ptr<RSRequester> rsRequester_;
  std::shared_ptr<RSClientThriftChannel> channel_;

  std::chrono::milliseconds timeout_;
  bool isSecure_;
  std::shared_ptr<RSConnectionStatus> connectionStatus_;

  apache::thrift::detail::ChannelCounters counters_;
};
} // namespace thrift
} // namespace apache
