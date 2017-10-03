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

#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>

#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

namespace apache {
namespace thrift {

class InMemoryConnection : public ClientConnectionIf {
 public:
  InMemoryConnection(std::shared_ptr<AsyncProcessorFactory> pFac);
  virtual ~InMemoryConnection() override = default;

  InMemoryConnection(const InMemoryConnection&) = delete;
  InMemoryConnection& operator=(const InMemoryConnection&) = delete;

  std::shared_ptr<ThriftChannelIf> getChannel() override;
  void setMaxPendingRequests(uint32_t num) override;
  folly::EventBase* getEventBase() const override;

  apache::thrift::async::TAsyncTransport* getTransport() override;
  bool good() override;
  ClientChannel::SaturationStatus getSaturationStatus() override;
  void attachEventBase(folly::EventBase* evb) override;
  void detachEventBase() override;
  bool isDetachable() override;
  bool isSecurityActive() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;

 private:
  folly::ScopedEventBaseThread runner_;
  std::shared_ptr<concurrency::ThreadManager> threadManager_;
  std::unique_ptr<ThriftProcessor> processor_;
};

} // namespace thrift
} // namespace apache
