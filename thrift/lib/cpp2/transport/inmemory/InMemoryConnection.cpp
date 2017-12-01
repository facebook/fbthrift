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

#include <thrift/lib/cpp2/transport/inmemory/InMemoryConnection.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/transport/inmemory/InMemoryChannel.h>

DEFINE_int32(thrift_num_cpu_threads, 1, "Number of cpu threads");

namespace apache {
namespace thrift {

InMemoryConnection::InMemoryConnection(
    std::shared_ptr<AsyncProcessorFactory> pFac,
    const apache::thrift::server::ServerConfigs& serverConfigs) {
  CHECK_GT(FLAGS_thrift_num_cpu_threads, 0);
  threadManager_ = PriorityThreadManager::newPriorityThreadManager(
      FLAGS_thrift_num_cpu_threads);
  threadManager_->start();
  processor_ =
      std::make_unique<ThriftProcessor>(pFac->getProcessor(), serverConfigs);
  processor_->setThreadManager(threadManager_.get());
  pFac_ = pFac;
}

std::shared_ptr<ThriftChannelIf> InMemoryConnection::getChannel(
    RequestRpcMetadata*) {
  return std::make_shared<InMemoryChannel>(
      processor_.get(), runner_.getEventBase());
}

void InMemoryConnection::setMaxPendingRequests(uint32_t) {
  // not implemented
}

void InMemoryConnection::setCloseCallback(ThriftClient*, CloseCallback*) {
  // not implemented
}

folly::EventBase* InMemoryConnection::getEventBase() const {
  return runner_.getEventBase();
}

apache::thrift::async::TAsyncTransport* InMemoryConnection::getTransport() {
  LOG(FATAL) << "Method should not be called";
}

bool InMemoryConnection::good() {
  LOG(FATAL) << "Method should not be called";
}

ClientChannel::SaturationStatus InMemoryConnection::getSaturationStatus() {
  LOG(FATAL) << "Method should not be called";
}

void InMemoryConnection::attachEventBase(folly::EventBase*) {
  LOG(FATAL) << "Method should not be called";
}

void InMemoryConnection::detachEventBase() {
  LOG(FATAL) << "Method should not be called";
}

bool InMemoryConnection::isDetachable() {
  LOG(FATAL) << "Method should not be called";
}

bool InMemoryConnection::isSecurityActive() {
  return false;
}

uint32_t InMemoryConnection::getTimeout() {
  LOG(FATAL) << "Method should not be called";
}

void InMemoryConnection::setTimeout(uint32_t) {
  LOG(FATAL) << "Method should not be called";
}

void InMemoryConnection::closeNow() {
  LOG(FATAL) << "Method should not be called";
}

CLIENT_TYPE InMemoryConnection::getClientType() {
  LOG(FATAL) << "Method should not be called";
}

} // namespace thrift
} // namespace apache
