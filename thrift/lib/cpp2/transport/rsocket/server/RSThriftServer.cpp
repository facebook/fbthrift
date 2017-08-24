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

#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftServer.h>

#include <glog/logging.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

namespace apache {
namespace thrift {

using namespace apache::thrift;
using namespace rsocket;

using std::vector;
using apache::thrift::concurrency::PriorityThreadManager;
using apache::thrift::concurrency::Runnable;
using apache::thrift::concurrency::ThreadManager;
using folly::SocketAddress;

RSThriftServer::~RSThriftServer() {
  if (server_) {
    server_->shutdownAndWait();
  }
}

void RSThriftServer::serve() {
  LOG(INFO) << "RSThriftServer::serve()";
  setup();

  // Don't allow any more configs to be modified.
  configMutable_ = false;

  server_->start(
      [this](auto&) { return std::make_shared<RSResponder>(thrift_.get()); });

  VLOG(1) << "RSThriftServer::serve";
}

void RSThriftServer::stop() {
  VLOG(1) << "RSThriftServer::stop";
  server_->shutdownAndWait();
}

void RSThriftServer::stopListening() {
  // TODO - We don't have such a function in RSocket Server that
  // doesn't accept new connections but continue to serve the existing ones.
  // We might need to add this functionality.
  LOG(WARNING) << "stopListening is not implemented";
}

bool RSThriftServer::isOverloaded(const transport::THeader* /*header*/) {
  LOG(WARNING) << "isOverloaded is not implemented";
  return false;
}

uint64_t RSThriftServer::getNumDroppedConnections() const {
  LOG(WARNING) << "getNumDroppedConnections is not implemented";
  return 0;
}

void RSThriftServer::setup() {
  VLOG(1) << "RSThriftServer::setup";

  CHECK(cpp2Pfac_);
  initializeThreadManager();

  thrift_ = std::make_unique<ThriftProcessor>(
      cpp2Pfac_->getProcessor(), threadManager_.get());

  if (port_ == -1) {
    auto addr = getAddress();
    port_ = addr.getPort();
  }

  TcpConnectionAcceptor::Options opts(
      port_, static_cast<size_t>(getNumIOWorkerThreads()));

  server_ = RSocket::createServer(
      std::make_unique<TcpConnectionAcceptor>(std::move(opts)));
}

void RSThriftServer::initializeThreadManager() {
  if (!getThreadManager()) {
    int numCpuWorkerThreads = getNumCPUWorkerThreads();
    if (numCpuWorkerThreads == 0) {
      // Use as fallback.
      numCpuWorkerThreads = getNumIOWorkerThreads();
    }
    std::shared_ptr<ThreadManager> threadManager(
        PriorityThreadManager::newPriorityThreadManager(
            numCpuWorkerThreads,
            true /* stats */,
            getMaxRequests() + numCpuWorkerThreads /* max queue length */));
    threadManager->enableCodel(getEnableCodel());
    if (!getCPUWorkerThreadName().empty()) {
      threadManager->setNamePrefix(getCPUWorkerThreadName());
    }
    threadManager->start();
    setThreadManager(threadManager);
  }
  threadManager_->setExpireCallback([&](std::shared_ptr<Runnable> r) {
    EventTask* task = dynamic_cast<EventTask*>(r.get());
    if (task) {
      task->expired();
    }
  });
  threadManager_->setCodelCallback([&](std::shared_ptr<Runnable> /*r*/) {
    auto observer = getObserver();
    if (observer) {
      if (getEnableCodel()) {
        observer->queueTimeout();
      } else {
        observer->shadowQueueTimeout();
      }
    }
  });
}

} // namespace thrift
} // namespace apache
