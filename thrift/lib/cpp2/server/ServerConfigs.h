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

#include <atomic>
#include <chrono>
#include <string>
#include <variant>

#include <folly/lang/Assume.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

using PreprocessResult = std::variant<
    std::monostate, // Allow request through
    AppClientException,
    AppServerException,
    AppOverloadedException>;

class Cpp2ConnContext;
class AdaptiveConcurrencyController;

namespace server {

struct PreprocessParams;

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
  virtual server::TServerObserver* getObserver() const = 0;

  // @see BaseThriftServer::getAdaptiveConcurrencyController function.
  virtual AdaptiveConcurrencyController& getAdaptiveConcurrencyController() = 0;
  virtual const AdaptiveConcurrencyController&
  getAdaptiveConcurrencyController() const = 0;

  // @see BaseThriftServer::getNumIOWorkerThreads function.
  virtual size_t getNumIOWorkerThreads() const = 0;

  // @see BaseThriftServer::getStreamExpireTime function.
  virtual std::chrono::milliseconds getStreamExpireTime() const = 0;

  // @see BaseThriftServer::getLoad function.
  virtual int64_t getLoad(
      const std::string& counter = "", bool check_custom = true) const = 0;

  // @see ThriftServer::checkOverload function.
  virtual folly::Optional<std::string> checkOverload(
      const transport::THeader::StringToStringMap* readHeaders,
      const std::string* method) const = 0;

  // @see ThriftServer::preprocess function.
  virtual PreprocessResult preprocess(const PreprocessParams& params) const = 0;

  // @see ThriftServer::getTosReflect function.
  virtual bool getTosReflect() const = 0;

  // @see ThriftServer::getListenerTos function.
  virtual uint32_t getListenerTos() const = 0;

  /**
   * Disables tracking of number of active requests in the server.
   *
   * This is useful for applications that do high throughput real-time work,
   * where the requests processing is done inline.
   *
   * Must be called before spinning up the server.
   *
   * WARNING: This will also disable maxActiveRequests load-shedding logic.
   */
  void disableActiveRequestsTracking() {
    disableActiveRequestsTracking_ = true;
  }

  bool isActiveRequestsTrackingDisabled() const {
    return disableActiveRequestsTracking_;
  }
  void incActiveRequests() {
    if (!isActiveRequestsTrackingDisabled()) {
      ++activeRequests_;
    }
  }

  void decActiveRequests() {
    if (!isActiveRequestsTrackingDisabled()) {
      --activeRequests_;
    }
  }

  int32_t getActiveRequests() const {
    if (!isActiveRequestsTrackingDisabled()) {
      return activeRequests_.load(std::memory_order_relaxed);
    } else {
      return 0;
    }
  }

  enum class RequestHandlingCapability { NONE, INTERNAL_METHODS_ONLY, ALL };
  /**
   * Determines which requests the server can handle in its current state.
   */
  virtual RequestHandlingCapability shouldHandleRequests() const {
    return RequestHandlingCapability::ALL;
  }

  bool shouldHandleRequestForMethod(const std::string& methodName) const {
    switch (shouldHandleRequests()) {
      case RequestHandlingCapability::ALL:
        return true;
      case RequestHandlingCapability::INTERNAL_METHODS_ONLY:
        return getInternalMethods().count(methodName) > 0;
      case RequestHandlingCapability::NONE:
        return false;
      default:
        LOG(DFATAL) << "Invalid RequestHandlingCapability";
        folly::assume_unreachable();
    }
  }

  bool getEnabled() const { return enabled_.load(); }

  void setEnabled(bool enabled) { enabled_ = enabled; }

  const std::unordered_set<std::string>& getInternalMethods() const {
    return internalMethods_;
  }

  void setInternalMethods(std::unordered_set<std::string> internalMethods) {
    internalMethods_ = std::move(internalMethods);
  }

  bool getRejectRequestsUntilStarted() const {
    return rejectRequestsUntilStarted_;
  }

  void setRejectRequestsUntilStarted(bool rejectRequestsUntilStarted) {
    rejectRequestsUntilStarted_ = rejectRequestsUntilStarted;
  }

 private:
  std::atomic<int32_t> activeRequests_{0};

  bool disableActiveRequestsTracking_{false};
  bool rejectRequestsUntilStarted_{false};
  std::atomic<bool> enabled_{true};
  std::unordered_set<std::string> internalMethods_;
};

} // namespace server
} // namespace thrift
} // namespace apache
