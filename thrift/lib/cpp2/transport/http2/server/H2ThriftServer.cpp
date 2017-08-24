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

#include <thrift/lib/cpp2/transport/http2/server/H2ThriftServer.h>

#include <glog/logging.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandlerFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache {
namespace thrift {

using std::vector;
using apache::thrift::concurrency::PriorityThreadManager;
using apache::thrift::concurrency::Runnable;
using apache::thrift::concurrency::ThreadManager;
using folly::SocketAddress;
using proxygen::HTTPServer;
using proxygen::HTTPServerOptions;
using proxygen::RequestHandlerChain;

void H2ThriftServer::serve() {
  setup();

  // Don't allow any more configs to be modified.
  configMutable_ = false;

  // Start the Proxygen server
  //
  // Return only after server_ stops running;
  server_->start();
}

void H2ThriftServer::stop() {
  server_->stop();
}

void H2ThriftServer::stopListening() {
  server_->stopListening();
}

bool H2ThriftServer::isOverloaded(const transport::THeader* /*header*/) {
  LOG(WARNING) << "isOverloaded is not implemented";
  return false;
}

uint64_t H2ThriftServer::getNumDroppedConnections() const {
  LOG(WARNING) << "getNumDroppedConnections is not implemented";
  return 0;
}

void H2ThriftServer::setup() {
  CHECK(cpp2Pfac_);
  initializeThreadManager();
  processor_ = std::make_unique<ThriftProcessor>(
      cpp2Pfac_->getProcessor(), threadManager_.get());

  // Set the Proxygen server options.
  HTTPServerOptions options;
  options.threads = static_cast<size_t>(getNumIOWorkerThreads());
  options.idleTimeout = getIdleTimeout();
  options.shutdownOn = {SIGINT, SIGTERM};
  options.handlerFactories =
      RequestHandlerChain()
          .addThen<ThriftRequestHandlerFactory>(processor_.get())
          .build();

  // Set the server location.
  // TODO: support TLS.
  SocketAddress addr;
  if (port_ == -1) {
    addr = getAddress();
  } else {
    addr = SocketAddress("::", port_, true);
  }
  vector<HTTPServer::IPConfig> IPs = {{addr, HTTPServer::Protocol::HTTP2}};

  // Create the Proxygen server.
  server_ = std::make_unique<HTTPServer>(std::move(options));
  server_->bind(IPs);

  // Run the preServe event.
  if (eventHandler_) {
    eventHandler_->preServe(&IPs[0].address);
  }
}

void H2ThriftServer::initializeThreadManager() {
  // Setup thread manager.
  // TODO: Right now this is copied directly from other TServer
  // implementations.  This needs to be updated after we understand its
  // details better.
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
