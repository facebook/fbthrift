/*
 * Copyright 2018-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestUtil.h>

DECLARE_int32(num_client_connections);
DECLARE_string(transport); // ConnectionManager depends on this flag.

namespace apache {
namespace thrift {

std::unique_ptr<ThriftServer> TestSetup::createServer(
    std::shared_ptr<AsyncProcessorFactory> processorFactory,
    uint16_t& port) {
  // override the default
  FLAGS_transport = "rsocket"; // client's transport
  observer_ = std::make_shared<FakeServerObserver>();

  auto server = std::make_unique<ThriftServer>();
  server->setObserver(observer_);
  server->setPort(0);
  server->setNumIOWorkerThreads(numIOThreads_);
  server->setNumCPUWorkerThreads(numWorkerThreads_);
  server->setProcessorFactory(processorFactory);

  server->addRoutingHandler(
      std::make_unique<apache::thrift::RSRoutingHandler>());

  auto eventHandler = std::make_shared<TestEventHandler>();
  server->setServerEventHandler(eventHandler);
  server->setup();

  // Get the port that the server has bound to
  port = eventHandler->waitForPortAssignment();
  return server;
}

std::unique_ptr<PooledRequestChannel, folly::DelayedDestruction::Destructor>
TestSetup::connectToServer(
    uint16_t port,
    folly::Function<void()> onDetachable) {
  CHECK_GT(port, 0) << "Check if the server has started already";
  return PooledRequestChannel::newChannel(
      evbThread_.getEventBase(),
      std::make_shared<folly::ScopedEventBaseThread>(),
      [port,
       onDetachable = std::move(onDetachable)](folly::EventBase& evb) mutable {
        auto rsocketChannel = RSocketClientChannel::newChannel(
            TAsyncSocket::UniquePtr(new TAsyncSocket(&evb, "::1", port)));
        if (onDetachable) {
          rsocketChannel->setOnDetachable(std::move(onDetachable));
        }
        return rsocketChannel;
      });
}

} // namespace thrift
} // namespace apache
