/*
 * Copyright 2015 Facebook, Inc.
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
#pragma once

#include <memory>
#include <chrono>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

namespace apache { namespace thrift {

template<typename Interface>
struct TestThriftServerFactory {
  public:
    std::shared_ptr<ThriftServer> create() {
      auto server = std::make_shared<apache::thrift::ThriftServer>();
      server->setNWorkerThreads(1);
      if (useSimpleThreadManager_) {
        auto threadFactory =
            std::make_shared<apache::thrift::concurrency::PosixThreadFactory>();
        auto threadManager =
            apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(
                1, 5, false, 50);
        threadManager->threadFactory(threadFactory);
        threadManager->start();
        server->setThreadManager(threadManager);
      } else if (exe_) {
        server->setThreadManager(exe_);
      }

    server->setPort(0);
    server->setSaslEnabled(true);
    if (useStubSaslServer_) {
      server->setSaslServerFactory(
          [] (folly::EventBase* evb) {
            return std::unique_ptr<apache::thrift::SaslServer>(
                new apache::thrift::StubSaslServer(evb));
          });
    }

    if (idleTimeoutMs_ != 0) {
      server->setIdleTimeout(std::chrono::milliseconds(idleTimeoutMs_));
    }

    if (duplex_) {
      server->setDuplex(true);
    }

    server->setInterface(std::unique_ptr<Interface>(new Interface));
    return server;
  }

  TestThriftServerFactory& useSimpleThreadManager(bool use) {
    useSimpleThreadManager_ = use;
    return *this;
  }

  TestThriftServerFactory& useThreadManager(
      std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe) {
    exe_ = exe;
    return *this;
  }

  TestThriftServerFactory& useStubSaslServer(bool use) {
    useStubSaslServer_ = use;
    return *this;
  }

  TestThriftServerFactory& idleTimeoutMs (uint32_t idle) {
    idleTimeoutMs_ = idle;
    return *this;
  }

  TestThriftServerFactory& duplex(bool duplex) {
    duplex_ = duplex;
    return *this;
  }

  private:
    bool useSimpleThreadManager_{true};
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe_{nullptr};
    bool useStubSaslServer_{true};
    uint32_t idleTimeoutMs_{0};
    bool duplex_{false};
};

}} // apache::thrift
