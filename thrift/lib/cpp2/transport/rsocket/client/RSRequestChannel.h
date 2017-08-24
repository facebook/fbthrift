/*
 * Copyright 2014-present Facebook, Inc.
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

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <rsocket/RSocket.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include "rsocket/RSocketRequester.h"

namespace apache {
namespace thrift {
class RSRequestChannel : public RequestChannel {
 public:
  explicit RSRequestChannel(
      std::shared_ptr<ThriftClient> thriftClient,
      folly::SocketAddress address,
      folly::EventBase* evb);

  uint32_t sendRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  uint32_t sendOnewayRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  void setCloseCallback(CloseCallback*) override {
    LOG(INFO) << "Unimplemented - setCloseCallback";
  }

  folly::EventBase* getEventBase() const override {
    return evb_;
  }

  void setProtocolId(uint16_t protocolId) {
    protocolId_ = protocolId;
  }

  uint16_t getProtocolId() override {
    return protocolId_;
  }

 private:
  std::shared_ptr<ThriftClient> thriftClient_;
  folly::EventBase* evb_;
  uint16_t protocolId_;
  folly::ScopedEventBaseThread rsClientWorker_;
  std::shared_ptr<rsocket::RSocketClient> rsClient_;
  std::shared_ptr<rsocket::RSocketRequester> rsRequester_;
};
}
}
