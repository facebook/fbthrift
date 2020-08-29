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
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/SetupFrameHandler.h>

namespace folly {
class AsyncTransport;
} // namespace folly

namespace apache {
namespace thrift {

class AsyncProcessor;
class Cpp2ConnContext;
class Cpp2Worker;
class RequestRpcMetadata;
class ThriftRequestCore;
using ThriftRequestCoreUniquePtr =
    std::unique_ptr<ThriftRequestCore, RequestsRegistry::Deleter>;

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

class RocketServerConnection;
class RocketServerFrameContext;

class ThriftRocketServerHandler : public RocketServerHandler {
 public:
  ThriftRocketServerHandler(
      std::shared_ptr<Cpp2Worker> worker,
      const folly::SocketAddress& clientAddress,
      const folly::AsyncTransport* transport,
      const std::vector<std::unique_ptr<SetupFrameHandler>>& handlers);

  ~ThriftRocketServerHandler() override;

  void handleSetupFrame(SetupFrame&& frame, RocketServerConnection& context)
      final;
  void handleRequestResponseFrame(
      RequestResponseFrame&& frame,
      RocketServerFrameContext&& context) final;
  void handleRequestFnfFrame(
      RequestFnfFrame&& frame,
      RocketServerFrameContext&& context) final;
  void handleRequestStreamFrame(
      RequestStreamFrame&& frame,
      RocketServerFrameContext&& context,
      RocketStreamClientCallback* clientCallback) final;
  void handleRequestChannelFrame(
      RequestChannelFrame&& frame,
      RocketServerFrameContext&& context,
      RocketSinkClientCallback* clientCallback) final;

  apache::thrift::server::TServerObserver::SamplingStatus shouldSample();

  void requestComplete() final;

  void terminateInteraction(int64_t id) final;

 private:
  const std::shared_ptr<Cpp2Worker> worker_;
  const std::shared_ptr<void> connectionGuard_;
  const folly::SocketAddress clientAddress_;
  Cpp2ConnContext connContext_;
  const std::vector<std::unique_ptr<SetupFrameHandler>>& setupFrameHandlers_;

  std::shared_ptr<AsyncProcessor> cpp2Processor_;
  std::shared_ptr<concurrency::ThreadManager> threadManager_;
  server::ServerConfigs* serverConfigs_ = nullptr;
  RequestsRegistry* requestsRegistry_ = nullptr;
  folly::EventBase* eventBase_;

  uint32_t sampleRate_{0};
  static thread_local uint32_t sample_;

  int32_t version_{4};

  template <class F>
  void handleRequestCommon(Payload&& payload, F&& makeRequest);

  FOLLY_NOINLINE void handleRequestWithBadMetadata(
      ThriftRequestCoreUniquePtr request);
  FOLLY_NOINLINE void handleRequestWithBadChecksum(
      ThriftRequestCoreUniquePtr request);
  FOLLY_NOINLINE void handleRequestOverloadedServer(
      ThriftRequestCoreUniquePtr request,
      const std::string& errorCode);
  FOLLY_NOINLINE void handleAppError(
      ThriftRequestCoreUniquePtr request,
      const std::string& name,
      const std::string& message,
      bool isClientError);
  FOLLY_NOINLINE void handleDecompressionFailure(
      ThriftRequestCoreUniquePtr request,
      std::string&& reason);
  FOLLY_NOINLINE void handleServerShutdown(ThriftRequestCoreUniquePtr request);
};

} // namespace rocket
} // namespace thrift
} // namespace apache
