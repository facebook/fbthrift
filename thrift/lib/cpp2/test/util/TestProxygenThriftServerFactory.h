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

#include <chrono>
#include <memory>

#include <thrift/lib/cpp2/server/proxygen/ProxygenThriftServer.h>
#include <thrift/lib/cpp2/test/util/TestServerFactory.h>

namespace apache {
namespace thrift {

template <typename Interface>
struct TestProxygenThriftServerFactory : public TestServerFactory {
 public:
  std::shared_ptr<BaseThriftServer> create() override {
    auto server = std::make_shared<apache::thrift::ProxygenThriftServer>();
    server->setNumIOWorkerThreads(1);
    size_t windowSize = 32 * 1024 * 1024;
    server->setInitialReceiveWindow(windowSize);
    if (useSimpleThreadManager_) {
      auto threadFactory =
          std::make_shared<apache::thrift::concurrency::PosixThreadFactory>();
      auto threadManager =
          apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(
              1, false);
      threadManager->threadFactory(threadFactory);
      threadManager->start();
      server->setThreadManager(threadManager);
    } else if (exe_) {
      server->setThreadManager(exe_);
    }

    server->setPort(0);

    if (idleTimeoutMs_ != 0) {
      server->setIdleTimeout(std::chrono::milliseconds(idleTimeoutMs_));
    }

    if (serverEventHandler_) {
      server->setServerEventHandler(serverEventHandler_);
    }

    server->setInterface(std::make_unique<Interface>());
    return server;
  }

  TestProxygenThriftServerFactory& useSimpleThreadManager(bool use) override {
    useSimpleThreadManager_ = use;
    return *this;
  }

  TestProxygenThriftServerFactory& useThreadManager(
      std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe)
      override {
    exe_ = exe;
    return *this;
  }

  TestProxygenThriftServerFactory& idleTimeoutMs(uint32_t idle) {
    idleTimeoutMs_ = idle;
    return *this;
  }

 private:
  bool useSimpleThreadManager_{true};
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe_{nullptr};
  uint32_t idleTimeoutMs_{0};
};
} // namespace thrift
} // namespace apache
