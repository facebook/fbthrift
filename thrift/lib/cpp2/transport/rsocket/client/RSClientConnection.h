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

namespace apache {
namespace thrift {

class RSConnectionStatus : public rsocket::RSocketConnectionEvents {
 public:
  bool isConnected() const {
    return isConnected_;
  }

 private:
  void onConnected() override {
    isConnected_ = true;
  }

  void onDisconnected(const folly::exception_wrapper& ew) override {
    VLOG(1) << "Connection is disconnected: " << ew.what();
    isConnected_ = false;
  }

  void onClosed(const folly::exception_wrapper& ew) override {
    VLOG(1) << "Connection is closed: " << ew.what();
    isConnected_ = false;
  }

  bool isConnected_{false};
};

class RSClientConnection : public ClientConnectionIf {
 public:
  explicit RSClientConnection(
      apache::thrift::async::TAsyncTransport::UniquePtr socket,
      bool isSecure = false);

  std::shared_ptr<ThriftChannelIf> getChannel(
      RequestRpcMetadata* metadata) override;
  void setMaxPendingRequests(uint32_t num) override;
  // TODO: Fuat, please implement this.
  void setCloseCallback(ThriftClient*, CloseCallback*) override {}
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

 protected:
  folly::EventBase* evb_;

  std::shared_ptr<rsocket::RSocketClient> rsClient_;
  std::shared_ptr<rsocket::RSocketRequester> rsRequester_;
  std::shared_ptr<RSClientThriftChannel> channel_;

  std::chrono::milliseconds timeout_;
  bool isSecure_;
  std::shared_ptr<RSConnectionStatus> connectionStatus_;

  apache::thrift::detail::ChannelCounters counters_;
};
} // namespace thrift
} // namespace apache
