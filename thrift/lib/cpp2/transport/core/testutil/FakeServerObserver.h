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

#pragma once

#include <thrift/lib/cpp/server/TServerObserver.h>
#include <atomic>

namespace apache {
namespace thrift {

class FakeServerObserver : public apache::thrift::server::TServerObserver {
 public:
  std::atomic<size_t> connAccepted_;
  std::atomic<size_t> connDropped_;
  std::atomic<size_t> connRejected_;
  std::atomic<size_t> activeConns_;
  std::atomic<size_t> taskKilled_;
  std::atomic<size_t> taskTimeout_;
  std::atomic<size_t> queueTimeout_;
  std::atomic<size_t> serverOverloaded_;
  std::atomic<size_t> receivedRequest_;
  std::atomic<size_t> queuedRequests_;
  std::atomic<size_t> shadowQueueTimeout_;
  std::atomic<size_t> sentReply_;
  std::atomic<size_t> activeRequests_;
  std::atomic<size_t> callCompleted_;
  std::atomic<size_t> protocolError_;

  void connAccepted() override {
    ++connAccepted_;
  }

  void connDropped() override {
    ++connDropped_;
  }

  void connRejected() override {
    ++connRejected_;
  }

  void activeConnections(int32_t numConnections) override {
    activeConns_ = numConnections;
  }

  void saslError() override {}

  void saslFallBack() override {}

  void saslComplete() override {}

  void tlsError() override {}

  void tlsComplete() override {}

  void tlsFallback() override {}

  void tlsResumption() override {}

  void taskKilled() override {
    // TODO: T24439845 - Implement Failure Injection
    ++taskKilled_;
  }

  void taskTimeout() override {
    ++taskTimeout_;
  }

  void serverOverloaded() override {
    // TODO: T24439936 - Implement LOADSHEDDING
    ++serverOverloaded_;
  }

  void receivedRequest() override {
    ++receivedRequest_;
  }

  void queuedRequests(int32_t numRequests) override {
    queuedRequests_ = numRequests;
  }

  void queueTimeout() override {
    ++queueTimeout_;
  }

  void shadowQueueTimeout() override {
    ++shadowQueueTimeout_;
  }

  void sentReply() override {
    ++sentReply_;
  }

  void activeRequests(int32_t numRequests) override {
    activeRequests_ = numRequests;
  }

  void callCompleted(const CallTimestamps& /*runtimes*/) override {
    ++callCompleted_;
  }

  void protocolError() override {
    ++protocolError_;
  }
};
} // namespace thrift
} // namespace apache
