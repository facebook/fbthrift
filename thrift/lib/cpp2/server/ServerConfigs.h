/*
 * Copyright 2014-present Facebook, Inc.
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

#ifndef SERVER_CONFIGS_H_
#define SERVER_CONFIGS_H_ 1

#include <chrono>

#include <thrift/lib/cpp/server/TServerObserver.h>

namespace apache {
namespace thrift {
namespace server {

/**
 * This class provides a set of abstract functions that the ThriftProcessor can
 * utilize to access the functions of ThriftServer.
 * The main aim of this interface is to break the possible cyclic dependency
 * between ThriftServer and ThriftProcessor.
 */
class ServerConfigs {
 public:
  virtual ~ServerConfigs() = default;

  /**
   * @see BaseThriftServer::getMaxResponseSize function.
   */
  virtual uint64_t getMaxResponseSize() const = 0;

  /**
   * @see BaseThriftServer::getTaskExpireTimeForRequest function.
   */
  virtual bool getTaskExpireTimeForRequest(
      std::chrono::milliseconds clientQueueTimeoutMs,
      std::chrono::milliseconds clientTimeoutMs,
      std::chrono::milliseconds& queueTimeout,
      std::chrono::milliseconds& taskTimeout) const = 0;

  // @see BaseThriftServer::getObserver function.
  virtual const std::shared_ptr<apache::thrift::server::TServerObserver>&
  getObserver() const = 0;

  // @see BaseThriftServer::setNumIOWorkerThreads function.
  virtual void setNumIOWorkerThreads(size_t numIOWorkerThreads) = 0;

  // @see BaseThriftServer::getNumIOWorkerThreads function.
  virtual size_t getNumIOWorkerThreads() const = 0;
};
} // namespace server
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_SERVER_H_
