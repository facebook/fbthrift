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

#pragma once

#include <memory>

#include <folly/Function.h>
#include <folly/Random.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncSocket.h>

#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/util/ScopedServerThread.h>

namespace apache {
namespace thrift {

class AsyncProcessorFactory;
class BaseThriftServer;
class ThriftServer;

/**
 * ScopedServerInterfaceThread spawns a thrift cpp2 server in a new thread.
 *
 * The server is stopped automatically when the instance is destroyed.
 */
class ScopedServerInterfaceThread {
 public:
  using ServerConfigCb = folly::Function<void(ThriftServer&)>;
  using MakeChannelFunc =
      folly::Function<ClientChannel::Ptr(folly::AsyncSocket::UniquePtr)>;

  ScopedServerInterfaceThread(
      std::shared_ptr<AsyncProcessorFactory> apf,
      folly::SocketAddress const& addr,
      ServerConfigCb configCb = {});

  explicit ScopedServerInterfaceThread(
      std::shared_ptr<AsyncProcessorFactory> apf,
      const std::string& host = "::1",
      uint16_t port = 0,
      ServerConfigCb configCb = {});

  explicit ScopedServerInterfaceThread(std::shared_ptr<BaseThriftServer> ts);

  BaseThriftServer& getThriftServer() const;
  const folly::SocketAddress& getAddress() const;
  uint16_t getPort() const;

  template <class AsyncClientT>
  std::unique_ptr<AsyncClientT> newStickyClient(
      folly::Executor* callbackExecutor = nullptr,
      MakeChannelFunc channelFunc = makeRocketOrHeaderChannel) const;

  template <class AsyncClientT>
  std::unique_ptr<AsyncClientT> newClient(
      folly::Executor* callbackExecutor = nullptr,
      MakeChannelFunc channelFunc = makeRocketOrHeaderChannel) const;

 private:
  std::shared_ptr<BaseThriftServer> ts_;
  util::ScopedServerThread sst_;

  static ClientChannel::Ptr makeRocketOrHeaderChannel(
      folly::AsyncSocket::UniquePtr socket) {
    if (folly::Random::oneIn(2)) {
      return RocketClientChannel::newChannel(std::move(socket));
    }
    return HeaderClientChannel::newChannel(std::move(socket));
  }
};

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread-inl.h>
