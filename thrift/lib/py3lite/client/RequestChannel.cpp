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

#include <thrift/lib/py3lite/client/RequestChannel.h>

#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace thrift {
namespace py3lite {
namespace client {

using namespace apache::thrift;

folly::Future<RequestChannel_ptr> createThriftChannelTCP(
    const std::string& host,
    uint16_t port,
    uint32_t connect_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto,
    const std::string& endpoint) {
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
    } else if (client_t == THRIFT_HTTP_CLIENT_TYPE) {
      return HeaderClientChannel::newChannel(
          std::move(socket),
          HeaderClientChannel::Options()
              .useAsHttpClient(host, endpoint)
              .setProtocolId(proto));
    } else {
      throw std::runtime_error("Unsupported client type");
    }
  });
  return future;
}

RequestChannel_ptr sync_createThriftChannelTCP(
    const std::string& host,
    uint16_t port,
    uint32_t connect_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto,
    const std::string& endpoint) {
  auto future = createThriftChannelTCP(
      host, port, connect_timeout, client_t, proto, endpoint);
  return std::move(future.wait().value());
}

folly::Future<RequestChannel_ptr> createThriftChannelUnix(
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
  return future;
}

RequestChannel_ptr sync_createThriftChannelUnix(
    const std::string& path,
    uint32_t connect_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto) {
  auto future = createThriftChannelUnix(path, connect_timeout, client_t, proto);
  return std::move(future.wait().value());
}

} // namespace client
} // namespace py3lite
} // namespace thrift
