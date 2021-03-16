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

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <folly/SocketAddress.h>

#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

using namespace std;
using namespace folly;
using namespace apache::thrift::concurrency;

namespace apache {
namespace thrift {

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<AsyncProcessorFactory> apf,
    SocketAddress const& addr,
    ServerConfigCb configCb) {
  auto tf = make_shared<PosixThreadFactory>(PosixThreadFactory::ATTACHED);
  auto tm = ThreadManager::newSimpleThreadManager(1, false);
  tm->threadFactory(move(tf));
  tm->start();
  auto ts = make_shared<ThriftServer>();
  ts->setAddress(addr);
  // Allow plaintext on loopback so the plaintext clients created by default by
  // the newClient methods can still connect.
  ts->setAllowPlaintextOnLoopback(true);
  ts->setProcessorFactory(move(apf));
  ts->setNumIOWorkerThreads(1);
  ts->setNumCPUWorkerThreads(1);
  ts->setThreadManager(tm);
  // The default behavior is to keep N recent requests per IO worker in memory.
  // In unit-tests, this defers memory reclamation and potentially masks
  // use-after-free bugs. Because this facility is used mostly in tests, it is
  // better not to keep any recent requests in memory.
  ts->setMaxFinishedDebugPayloadsPerWorker(0);
  if (configCb) {
    configCb(*ts);
  }
  ts_ = ts;
  sst_.start(ts_, [ts]() { ts->getEventBaseManager()->clearEventBase(); });
}

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<AsyncProcessorFactory> apf,
    const string& host,
    uint16_t port,
    ServerConfigCb configCb)
    : ScopedServerInterfaceThread(
          move(apf),
          SocketAddress(host, port),
          move(configCb)) {}

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<BaseThriftServer> bts) {
  ts_ = bts;
  sst_.start(ts_);
}

BaseThriftServer& ScopedServerInterfaceThread::getThriftServer() const {
  return *ts_;
}

const SocketAddress& ScopedServerInterfaceThread::getAddress() const {
  return *sst_.getAddress();
}

uint16_t ScopedServerInterfaceThread::getPort() const {
  return getAddress().getPort();
}

RequestChannel::Ptr ScopedServerInterfaceThread::newChannel(
    folly::Executor* callbackExecutor,
    MakeChannelFunc makeChannel,
    std::weak_ptr<folly::IOExecutor> executor) const {
  return PooledRequestChannel::newChannel(
      callbackExecutor,
      std::move(executor),
      [makeChannel = std::move(makeChannel),
       address = getAddress()](folly::EventBase& eb) mutable {
        return makeChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&eb, address)));
      });
}

namespace {
struct TestClientRunner {
  ScopedServerInterfaceThread runner;
  RequestChannel::Ptr channel;

  explicit TestClientRunner(std::shared_ptr<AsyncProcessorFactory> apf)
      : runner(std::move(apf)) {}
};
} // namespace

std::shared_ptr<RequestChannel>
ScopedServerInterfaceThread::makeTestClientChannel(
    std::shared_ptr<AsyncProcessorFactory> apf,
    folly::Executor* callbackExecutor,
    ScopedServerInterfaceThread::FaultInjectionFunc injectFault) {
  auto runner = std::make_shared<TestClientRunner>(std::move(apf));
  auto innerChannel = runner->runner.newChannel(
      callbackExecutor, RocketClientChannel::newChannel);
  if (injectFault) {
    runner->channel.reset(new apache::thrift::detail::FaultInjectionChannel(
        std::move(innerChannel), std::move(injectFault)));
  } else {
    runner->channel = std::move(innerChannel);
  }
  auto* channel = runner->channel.get();
  return folly::to_shared_ptr_aliasing(std::move(runner), channel);
}
} // namespace thrift
} // namespace apache
