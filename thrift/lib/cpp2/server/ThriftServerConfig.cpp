/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/server/ThriftServerConfig.h>

namespace apache {
namespace thrift {

const size_t ThriftServerConfig::T_ASYNC_DEFAULT_WORKER_THREADS =
    std::thread::hardware_concurrency();

ThriftServerConfig::ThriftServerConfig(
    const ThriftServerInitialConfig& initialConfig)
    : ThriftServerConfig() {
  if (auto& [value, isSet] = initialConfig.maxRequests_; isSet) {
    maxRequests_.setDefault(folly::observer::makeStaticObserver(value));
  }
  if (auto& [value, isSet] = initialConfig.queueTimeout_; isSet) {
    queueTimeout_.setDefault(folly::observer::makeStaticObserver(value));
  }
}

std::string ThriftServerConfig::getCPUWorkerThreadName() const {
  return poolThreadName_.get();
}

std::chrono::seconds ThriftServerConfig::getWorkersJoinTimeout() const {
  return workersJoinTimeout_.get();
}

int ThriftServerConfig::getListenBacklog() const {
  return listenBacklog_.get();
}

std::chrono::milliseconds ThriftServerConfig::getIdleTimeout() const {
  return timeout_.get();
}

size_t ThriftServerConfig::getNumIOWorkerThreads() const {
  return nWorkers_.get();
}

size_t ThriftServerConfig::getNumCPUWorkerThreads() const {
  auto nCPUWorkers = nPoolThreads_.get();
  return nCPUWorkers ? nCPUWorkers : T_ASYNC_DEFAULT_WORKER_THREADS;
}

const folly::sorted_vector_set<std::string>&
ThriftServerConfig::getMethodsBypassMaxRequestsLimit() const {
  return methodsBypassMaxRequestsLimit_.get();
}

uint32_t ThriftServerConfig::getMaxNumPendingConnectionsPerWorker() const {
  return maxNumPendingConnectionsPerWorker_.get();
}

uint64_t ThriftServerConfig::getMaxDebugPayloadMemoryPerRequest() const {
  return maxDebugPayloadMemoryPerRequest_.get();
}

uint64_t ThriftServerConfig::getMaxDebugPayloadMemoryPerWorker() const {
  return maxDebugPayloadMemoryPerWorker_.get();
}

uint16_t ThriftServerConfig::getMaxFinishedDebugPayloadsPerWorker() const {
  return maxFinishedDebugPayloadsPerWorker_.get();
}

const ServerAttributeDynamic<uint32_t>& ThriftServerConfig::getMaxConnections()
    const {
  return maxConnections_;
}

const ServerAttributeDynamic<uint32_t>& ThriftServerConfig::getMaxRequests()
    const {
  return maxRequests_;
}

const ServerAttributeDynamic<uint64_t>& ThriftServerConfig::getMaxResponseSize()
    const {
  return maxResponseSize_;
}

const ServerAttributeDynamic<bool>& ThriftServerConfig::getUseClientTimeout()
    const {
  return useClientTimeout_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getWriteBatchingInterval() const {
  return writeBatchingInterval_;
}

const ServerAttributeDynamic<size_t>& ThriftServerConfig::getWriteBatchingSize()
    const {
  return writeBatchingSize_;
}

const ServerAttributeDynamic<size_t>&
ThriftServerConfig::getWriteBatchingByteSize() const {
  return writeBatchingByteSize_;
}

const ServerAttributeDynamic<bool>& ThriftServerConfig::getEnableCodel() const {
  return enableCodel_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getTaskExpireTime() const {
  return taskExpireTime_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getStreamExpireTime() const {
  return streamExpireTime_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getQueueTimeout() const {
  return queueTimeout_;
}

const ServerAttributeDynamic<std::chrono::nanoseconds>&
ThriftServerConfig::getSocketQueueTimeout() const {
  return socketQueueTimeout_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getSocketWriteTimeout() const {
  return socketWriteTimeout_;
}

const ServerAttributeDynamic<size_t>&
ThriftServerConfig::getIngressMemoryLimit() const {
  return ingressMemoryLimit_;
}

const ServerAttributeDynamic<size_t>& ThriftServerConfig::getEgressMemoryLimit()
    const {
  return egressMemoryLimit_;
}

const ServerAttributeDynamic<size_t>&
ThriftServerConfig::getMinPayloadSizeToEnforceIngressMemoryLimit() const {
  return minPayloadSizeToEnforceIngressMemoryLimit_;
}

const ServerAttributeDynamic<size_t>&
ThriftServerConfig::getEgressBufferBackpressureThreshold() const {
  return egressBufferBackpressureThreshold_;
}

const ServerAttributeDynamic<double>&
ThriftServerConfig::getEgressBufferRecoveryFactor() const {
  return egressBufferRecoveryFactor_;
}

const ServerAttributeDynamic<std::chrono::milliseconds>&
ThriftServerConfig::getPolledServiceHealthLiveness() const {
  return polledServiceHealthLiveness_;
}

const ServerAttributeDynamic<bool>& ThriftServerConfig::isHeaderDisabled()
    const {
  return disableHeaderTransport_;
}

const ServerAttributeDynamic<folly::SocketOptionMap>&
ThriftServerConfig::getPerConnectionSocketOptions() const {
  return perConnectionSocketOptions_;
}

void ThriftServerConfig::setCPUWorkerThreadName(
    const std::string& cpuWorkerThreadName, AttributeSource source) {
  setStaticAttribute(poolThreadName_, std::string{cpuWorkerThreadName}, source);
}

void ThriftServerConfig::unsetCPUWorkerThreadName(AttributeSource source) {
  unsetStaticAttribute(poolThreadName_, source);
}

void ThriftServerConfig::setWorkersJoinTimeout(
    std::chrono::seconds timeout, AttributeSource source) {
  setStaticAttribute(workersJoinTimeout_, std::move(timeout), source);
}

void ThriftServerConfig::unsetWorkersJoinTimeout(AttributeSource source) {
  unsetStaticAttribute(workersJoinTimeout_, source);
}

void ThriftServerConfig::setMaxNumPendingConnectionsPerWorker(
    uint32_t num, AttributeSource source) {
  setStaticAttribute(
      maxNumPendingConnectionsPerWorker_, std::move(num), source);
}

void ThriftServerConfig::unsetMaxNumPendingConnectionsPerWorker(
    AttributeSource source) {
  unsetStaticAttribute(maxNumPendingConnectionsPerWorker_, source);
}

void ThriftServerConfig::setIdleTimeout(
    std::chrono::milliseconds timeout, AttributeSource source) {
  setStaticAttribute(timeout_, std::move(timeout), source);
}

void ThriftServerConfig::unsetIdleTimeout(AttributeSource source) {
  unsetStaticAttribute(timeout_, source);
}

void ThriftServerConfig::setNumIOWorkerThreads(
    size_t numIOWorkerThreads, AttributeSource source) {
  setStaticAttribute(nWorkers_, std::move(numIOWorkerThreads), source);
}

void ThriftServerConfig::unsetNumIOWorkerThreads(AttributeSource source) {
  unsetStaticAttribute(nWorkers_, source);
}

void ThriftServerConfig::setNumCPUWorkerThreads(
    size_t numCPUWorkerThreads, AttributeSource source) {
  setStaticAttribute(nPoolThreads_, std::move(numCPUWorkerThreads), source);
}

void ThriftServerConfig::unsetNumCPUWorkerThreads(AttributeSource source) {
  unsetStaticAttribute(nPoolThreads_, source);
}

void ThriftServerConfig::setListenBacklog(
    int listenBacklog, AttributeSource source) {
  setStaticAttribute(listenBacklog_, std::move(listenBacklog), source);
}

void ThriftServerConfig::unsetListenBacklog(AttributeSource source) {
  unsetStaticAttribute(listenBacklog_, source);
}

void ThriftServerConfig::setMethodsBypassMaxRequestsLimit(
    const std::vector<std::string>& methods, AttributeSource source) {
  setStaticAttribute(
      methodsBypassMaxRequestsLimit_,
      folly::sorted_vector_set<std::string>{methods.begin(), methods.end()},
      source);
}

void ThriftServerConfig::unsetMethodsBypassMaxRequestsLimit(
    AttributeSource source) {
  unsetStaticAttribute(methodsBypassMaxRequestsLimit_, source);
}

void ThriftServerConfig::setMaxDebugPayloadMemoryPerRequest(
    uint64_t limit, AttributeSource source) {
  setStaticAttribute(
      maxDebugPayloadMemoryPerRequest_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxDebugPayloadMemoryPerRequest(
    AttributeSource source) {
  unsetStaticAttribute(maxDebugPayloadMemoryPerRequest_, source);
}

void ThriftServerConfig::setMaxDebugPayloadMemoryPerWorker(
    uint64_t limit, AttributeSource source) {
  setStaticAttribute(maxDebugPayloadMemoryPerWorker_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxDebugPayloadMemoryPerWorker(
    AttributeSource source) {
  unsetStaticAttribute(maxDebugPayloadMemoryPerWorker_, source);
}

void ThriftServerConfig::setMaxFinishedDebugPayloadsPerWorker(
    uint16_t limit, AttributeSource source) {
  setStaticAttribute(
      maxFinishedDebugPayloadsPerWorker_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxFinishedDebugPayloadsPerWorker(
    AttributeSource source) {
  unsetStaticAttribute(maxFinishedDebugPayloadsPerWorker_, source);
}

void ThriftServerConfig::setMaxConnections(
    folly::observer::Observer<uint32_t> maxConnections,
    AttributeSource source) {
  maxConnections_.set(maxConnections, source);
}

void ThriftServerConfig::unsetMaxConnections(AttributeSource source) {
  maxConnections_.unset(source);
}

void ThriftServerConfig::setMaxRequests(
    folly::observer::Observer<uint32_t> maxRequests, AttributeSource source) {
  maxRequests_.set(maxRequests, source);
}

void ThriftServerConfig::unsetMaxRequests(AttributeSource source) {
  maxRequests_.unset(source);
}

void ThriftServerConfig::setMaxResponseSize(
    folly::observer::Observer<uint64_t> size, AttributeSource source) {
  maxResponseSize_.set(size, source);
}

void ThriftServerConfig::unsetMaxResponseSize(AttributeSource source) {
  maxResponseSize_.unset(source);
}

void ThriftServerConfig::setUseClientTimeout(
    folly::observer::Observer<bool> useClientTimeout, AttributeSource source) {
  useClientTimeout_.set(useClientTimeout, source);
}

void ThriftServerConfig::unsetUseClientTimeout(AttributeSource source) {
  useClientTimeout_.unset(source);
}

void ThriftServerConfig::setWriteBatchingInterval(
    folly::observer::Observer<std::chrono::milliseconds> interval,
    AttributeSource source) {
  writeBatchingInterval_.set(interval, source);
}

void ThriftServerConfig::unsetWriteBatchingInterval(AttributeSource source) {
  writeBatchingInterval_.unset(source);
}

void ThriftServerConfig::setWriteBatchingSize(
    folly::observer::Observer<size_t> batchingSize, AttributeSource source) {
  writeBatchingSize_.set(batchingSize, source);
}

void ThriftServerConfig::unsetWriteBatchingSize(AttributeSource source) {
  writeBatchingSize_.unset(source);
}

void ThriftServerConfig::setWriteBatchingByteSize(
    folly::observer::Observer<size_t> batchingByteSize,
    AttributeSource source) {
  writeBatchingByteSize_.set(batchingByteSize, source);
}

void ThriftServerConfig::unsetWriteBatchingByteSize(AttributeSource source) {
  writeBatchingByteSize_.unset(source);
}

void ThriftServerConfig::setEnableCodel(
    folly::observer::Observer<bool> enableCodel, AttributeSource source) {
  enableCodel_.set(enableCodel, source);
}

void ThriftServerConfig::unsetEnableCodel(AttributeSource source) {
  enableCodel_.unset(source);
}

void ThriftServerConfig::setTaskExpireTime(
    folly::observer::Observer<std::chrono::milliseconds> timeout,
    AttributeSource source) {
  taskExpireTime_.set(timeout, source);
}

void ThriftServerConfig::unsetTaskExpireTime(AttributeSource source) {
  taskExpireTime_.unset(source);
}

void ThriftServerConfig::setStreamExpireTime(
    folly::observer::Observer<std::chrono::milliseconds> timeout,
    AttributeSource source) {
  streamExpireTime_.set(timeout, source);
}

void ThriftServerConfig::unsetStreamExpireTime(AttributeSource source) {
  streamExpireTime_.unset(source);
}

void ThriftServerConfig::setQueueTimeout(
    folly::observer::Observer<std::chrono::milliseconds> timeout,
    AttributeSource source) {
  queueTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetQueueTimeout(AttributeSource source) {
  queueTimeout_.unset(source);
}

void ThriftServerConfig::setSocketQueueTimeout(
    folly::observer::Observer<std::chrono::nanoseconds> timeout,
    AttributeSource source) {
  socketQueueTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetSocketQueueTimeout(AttributeSource source) {
  socketQueueTimeout_.unset(source);
}

void ThriftServerConfig::setSocketWriteTimeout(
    folly::observer::Observer<std::chrono::milliseconds> timeout,
    AttributeSource source) {
  socketWriteTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetSocketWriteTimeout(AttributeSource source) {
  socketWriteTimeout_.unset(source);
}

void ThriftServerConfig::setIngressMemoryLimit(
    folly::observer::Observer<size_t> ingressMemoryLimit,
    AttributeSource source) {
  ingressMemoryLimit_.set(ingressMemoryLimit, source);
}

void ThriftServerConfig::unsetIngressMemoryLimit(AttributeSource source) {
  ingressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setEgressMemoryLimit(
    folly::observer::Observer<size_t> max, AttributeSource source) {
  egressMemoryLimit_.set(max, source);
}

void ThriftServerConfig::unsetEgressMemoryLimit(AttributeSource source) {
  egressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setMinPayloadSizeToEnforceIngressMemoryLimit(
    folly::observer::Observer<size_t> minPayloadSizeToEnforceIngressMemoryLimit,
    AttributeSource source) {
  minPayloadSizeToEnforceIngressMemoryLimit_.set(
      minPayloadSizeToEnforceIngressMemoryLimit, source);
}

void ThriftServerConfig::unsetMinPayloadSizeToEnforceIngressMemoryLimit(
    AttributeSource source) {
  minPayloadSizeToEnforceIngressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setEgressBufferBackpressureThreshold(
    folly::observer::Observer<size_t> thresholdInBytes,
    AttributeSource source) {
  egressBufferBackpressureThreshold_.set(thresholdInBytes, source);
}

void ThriftServerConfig::unsetEgressBufferBackpressureThreshold(
    AttributeSource source) {
  egressBufferBackpressureThreshold_.unset(source);
}

void ThriftServerConfig::setEgressBufferRecoveryFactor(
    folly::observer::Observer<double> recoveryFactor, AttributeSource source) {
  auto clampedRecoveryFactor = folly::observer::makeStaticObserver(
      std::clamp(**recoveryFactor, 0.0, 1.0));
  egressBufferRecoveryFactor_.set(clampedRecoveryFactor, source);
}

void ThriftServerConfig::unsetEgressBufferRecoveryFactor(
    AttributeSource source) {
  egressBufferRecoveryFactor_.unset(source);
}

void ThriftServerConfig::setPolledServiceHealthLiveness(
    folly::observer::Observer<std::chrono::milliseconds> liveness,
    AttributeSource source) {
  polledServiceHealthLiveness_.set(liveness, source);
}

void ThriftServerConfig::unsetPolledServiceHealthLiveness(
    AttributeSource source) {
  polledServiceHealthLiveness_.unset(source);
}

void ThriftServerConfig::disableLegacyTransports(
    folly::observer::Observer<bool> value, AttributeSource source) {
  disableHeaderTransport_.set(value, source);
}

void ThriftServerConfig::unsetdisableLegacyTransports(AttributeSource source) {
  disableHeaderTransport_.unset(source);
}

void ThriftServerConfig::setPerConnectionSocketOptions(
    folly::observer::Observer<folly::SocketOptionMap> options,
    AttributeSource source) {
  perConnectionSocketOptions_.set(std::move(options), source);
}

void ThriftServerConfig::unsetPerConnectionSocketOptions(
    AttributeSource source) {
  perConnectionSocketOptions_.unset(source);
}

} // namespace thrift
} // namespace apache
