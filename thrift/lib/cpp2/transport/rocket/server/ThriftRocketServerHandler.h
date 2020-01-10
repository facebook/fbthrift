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
#include <vector>

#include <folly/Function.h>
#include <folly/SocketAddress.h>

#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/SetupFrameHandler.h>

namespace folly {
class AsyncTransportWrapper;
} // namespace folly

namespace apache {
namespace thrift {

class AsyncProcessor;
class Cpp2ConnContext;
class Cpp2Worker;
class RequestRpcMetadata;
class ThriftRequestCore;

namespace concurrency {
class ThreadManager;
} // namespace concurrency

namespace server {
class ServerConfigs;
} // namespace server

namespace rocket {

class Payload;
class RequestFnfFrame;
class RequestResponseFrame;
class RequestStreamFrame;
class SetupFrame;

class RocketServerFrameContext;

class ThriftRocketServerHandler : public RocketServerHandler {
 public:
  ThriftRocketServerHandler(
      std::shared_ptr<Cpp2Worker> worker,
      const folly::SocketAddress& clientAddress,
      const folly::AsyncTransportWrapper* transport,
      const std::vector<std::unique_ptr<SetupFrameHandler>>& handlers);

  void handleSetupFrame(SetupFrame&& frame, RocketServerFrameContext&& context)
      final;
  void handleRequestResponseFrame(
      RequestResponseFrame&& frame,
      RocketServerFrameContext&& context) final;
  void handleRequestFnfFrame(
      RequestFnfFrame&& frame,
      RocketServerFrameContext&& context) final;
  void handleRequestStreamFrame(
      RequestStreamFrame&& frame,
      RocketStreamClientCallback* clientCallback) final;
  void handleRequestChannelFrame(
      RequestChannelFrame&& frame,
      RocketSinkClientCallback* clientCallback) final;

  apache::thrift::server::TServerObserver::SamplingStatus shouldSample();

 private:
  const std::shared_ptr<Cpp2Worker> worker_;
  const folly::SocketAddress clientAddress_;
  const std::shared_ptr<Cpp2ConnContext> connContext_;
  const std::vector<std::unique_ptr<SetupFrameHandler>>& setupFrameHandlers_;

  std::shared_ptr<AsyncProcessor> cpp2Processor_;
  std::shared_ptr<concurrency::ThreadManager> threadManager_;
  server::ServerConfigs* serverConfigs_ = nullptr;
  std::shared_ptr<ActiveRequestsRegistry> activeRequestsRegistry_;
  folly::EventBase* eventBase_;

  uint32_t sampleRate_{0};
  static thread_local uint32_t sample_;

  template <class F>
  void handleRequestCommon(Payload&& payload, F&& makeRequest);

  FOLLY_NOINLINE void handleRequestWithBadMetadata(
      std::unique_ptr<ThriftRequestCore> request);
  FOLLY_NOINLINE void handleRequestWithBadChecksum(
      std::unique_ptr<ThriftRequestCore> request);
  FOLLY_NOINLINE void handleRequestOverloadedServer(
      std::unique_ptr<ThriftRequestCore> request);
  FOLLY_NOINLINE void handleServerShutdown(
      std::unique_ptr<ThriftRequestCore> request);
};

} // namespace rocket
} // namespace thrift
} // namespace apache
