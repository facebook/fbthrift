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

#include <thrift/lib/py3lite/client/SyncClient.h>

#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace thrift {
namespace py3lite {
namespace sync_client {

using namespace apache::thrift;

/**
 * Create a thrift channel by connecting to a host:port over TCP.
 */
RequestChannel_ptr createThriftChannelTCP(
    const std::string& host,
    uint16_t port,
    uint32_t connect_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto) {
  auto eb = folly::getGlobalIOExecutor()->getEventBase();
  auto future = folly::via(eb, [=]() -> RequestChannel_ptr {
    auto socket =
        folly::AsyncSocket::newSocket(eb, host, port, connect_timeout);
    if (client_t == THRIFT_HEADER_CLIENT_TYPE) {
      return HeaderClientChannel::newChannel(
          std::move(socket),
          HeaderClientChannel::Options()
              .setClientType(THRIFT_HEADER_CLIENT_TYPE)
              .setProtocolId(proto));
    } else if (client_t == THRIFT_ROCKET_CLIENT_TYPE) {
      auto chan = RocketClientChannel::newChannel(std::move(socket));
      chan->setProtocolId(proto);
      return chan;
    } else {
      throw std::runtime_error("Unsupported client type");
    }
  });
  return std::move(future.wait().value());
}

/**
 * Create a thrift channel by connecting to a Unix domain socket.
 */
RequestChannel_ptr createThriftChannelUnix(
    const std::string& path,
    uint32_t connect_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto) {
  auto eb = folly::getGlobalIOExecutor()->getEventBase();
  auto future = folly::via(eb, [=]() -> RequestChannel_ptr {
    auto socket = folly::AsyncSocket::newSocket(
        eb, folly::SocketAddress::makeFromPath(path), connect_timeout);
    if (client_t == THRIFT_HEADER_CLIENT_TYPE) {
      return HeaderClientChannel::newChannel(
          std::move(socket),
          HeaderClientChannel::Options()
              .setClientType(THRIFT_HEADER_CLIENT_TYPE)
              .setProtocolId(proto));
    } else if (client_t == THRIFT_ROCKET_CLIENT_TYPE) {
      auto chan = RocketClientChannel::newChannel(std::move(socket));
      chan->setProtocolId(proto);
      return chan;
    } else {
      throw std::runtime_error("Unsupported client type");
    }
  });
  return std::move(future.wait().value());
}

} // namespace sync_client
} // namespace py3lite
} // namespace thrift
