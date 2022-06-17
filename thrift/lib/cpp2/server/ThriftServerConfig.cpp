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

// TODO (malbashir): remove include when Static and dynamic tag types are
// removed
#include <thrift/lib/cpp2/server/ServerConfigs.h>
#include <thrift/lib/cpp2/server/ThriftServerConfig.h>

namespace apache {
namespace thrift {

const size_t ThriftServerConfig::T_ASYNC_DEFAULT_WORKER_THREADS =
    std::thread::hardware_concurrency();

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
    const std::string& cpuWorkerThreadName,
    AttributeSource source,
    StaticAttributeTag) {
  setStaticAttribute(poolThreadName_, std::string{cpuWorkerThreadName}, source);
}

void ThriftServerConfig::unsetCPUWorkerThreadName(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(poolThreadName_, source);
}

void ThriftServerConfig::setWorkersJoinTimeout(
    std::chrono::seconds timeout, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(workersJoinTimeout_, std::move(timeout), source);
}

void ThriftServerConfig::unsetWorkersJoinTimeout(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(workersJoinTimeout_, source);
}

void ThriftServerConfig::setMaxNumPendingConnectionsPerWorker(
    uint32_t num, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(
      maxNumPendingConnectionsPerWorker_, std::move(num), source);
}

void ThriftServerConfig::unsetMaxNumPendingConnectionsPerWorker(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(maxNumPendingConnectionsPerWorker_, source);
}

void ThriftServerConfig::setIdleTimeout(
    std::chrono::milliseconds timeout,
    AttributeSource source,
    StaticAttributeTag) {
  setStaticAttribute(timeout_, std::move(timeout), source);
}

void ThriftServerConfig::unsetIdleTimeout(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(timeout_, source);
}

void ThriftServerConfig::setNumIOWorkerThreads(
    size_t numIOWorkerThreads, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(nWorkers_, std::move(numIOWorkerThreads), source);
}

void ThriftServerConfig::unsetNumIOWorkerThreads(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(nWorkers_, source);
}

void ThriftServerConfig::setNumCPUWorkerThreads(
    size_t numCPUWorkerThreads, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(nPoolThreads_, std::move(numCPUWorkerThreads), source);
}

void ThriftServerConfig::unsetNumCPUWorkerThreads(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(nPoolThreads_, source);
}

void ThriftServerConfig::setListenBacklog(
    int listenBacklog, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(listenBacklog_, std::move(listenBacklog), source);
}

void ThriftServerConfig::unsetListenBacklog(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(listenBacklog_, source);
}

void ThriftServerConfig::setMethodsBypassMaxRequestsLimit(
    const std::vector<std::string>& methods,
    AttributeSource source,
    StaticAttributeTag) {
  setStaticAttribute(
      methodsBypassMaxRequestsLimit_,
      folly::sorted_vector_set<std::string>{methods.begin(), methods.end()},
      source);
}

void ThriftServerConfig::unsetMethodsBypassMaxRequestsLimit(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(methodsBypassMaxRequestsLimit_, source);
}

void ThriftServerConfig::setMaxDebugPayloadMemoryPerRequest(
    uint64_t limit, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(
      maxDebugPayloadMemoryPerRequest_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxDebugPayloadMemoryPerRequest(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(maxDebugPayloadMemoryPerRequest_, source);
}

void ThriftServerConfig::setMaxDebugPayloadMemoryPerWorker(
    uint64_t limit, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(maxDebugPayloadMemoryPerWorker_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxDebugPayloadMemoryPerWorker(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(maxDebugPayloadMemoryPerWorker_, source);
}

void ThriftServerConfig::setMaxFinishedDebugPayloadsPerWorker(
    uint16_t limit, AttributeSource source, StaticAttributeTag) {
  setStaticAttribute(
      maxFinishedDebugPayloadsPerWorker_, std::move(limit), source);
}

void ThriftServerConfig::unsetMaxFinishedDebugPayloadsPerWorker(
    AttributeSource source, StaticAttributeTag) {
  unsetStaticAttribute(maxFinishedDebugPayloadsPerWorker_, source);
}

void ThriftServerConfig::setMaxConnections(
    uint32_t maxConnections, AttributeSource source, DynamicAttributeTag) {
  maxConnections_.set(maxConnections, source);
}

void ThriftServerConfig::unsetMaxConnections(
    AttributeSource source, DynamicAttributeTag) {
  maxConnections_.unset(source);
}

void ThriftServerConfig::setMaxRequests(
    uint32_t maxRequests, AttributeSource source, DynamicAttributeTag) {
  maxRequests_.set(maxRequests, source);
}

void ThriftServerConfig::unsetMaxRequests(
    AttributeSource source, DynamicAttributeTag) {
  maxRequests_.unset(source);
}

void ThriftServerConfig::setMaxResponseSize(
    uint64_t size, AttributeSource source, DynamicAttributeTag) {
  maxResponseSize_.set(size, source);
}

void ThriftServerConfig::unsetMaxResponseSize(
    AttributeSource source, DynamicAttributeTag) {
  maxResponseSize_.unset(source);
}

void ThriftServerConfig::setUseClientTimeout(
    bool useClientTimeout, AttributeSource source, DynamicAttributeTag) {
  useClientTimeout_.set(useClientTimeout, source);
}

void ThriftServerConfig::unsetUseClientTimeout(
    AttributeSource source, DynamicAttributeTag) {
  useClientTimeout_.unset(source);
}

void ThriftServerConfig::setWriteBatchingInterval(
    std::chrono::milliseconds interval,
    AttributeSource source,
    DynamicAttributeTag) {
  writeBatchingInterval_.set(interval, source);
}

void ThriftServerConfig::unsetWriteBatchingInterval(
    AttributeSource source, DynamicAttributeTag) {
  writeBatchingInterval_.unset(source);
}

void ThriftServerConfig::setWriteBatchingSize(
    size_t batchingSize, AttributeSource source, DynamicAttributeTag) {
  writeBatchingSize_.set(batchingSize, source);
}

void ThriftServerConfig::unsetWriteBatchingSize(
    AttributeSource source, DynamicAttributeTag) {
  writeBatchingSize_.unset(source);
}

void ThriftServerConfig::setWriteBatchingByteSize(
    size_t batchingByteSize, AttributeSource source, DynamicAttributeTag) {
  writeBatchingByteSize_.set(batchingByteSize, source);
}

void ThriftServerConfig::unsetWriteBatchingByteSize(
    AttributeSource source, DynamicAttributeTag) {
  writeBatchingByteSize_.unset(source);
}

void ThriftServerConfig::setEnableCodel(
    bool enableCodel, AttributeSource source, DynamicAttributeTag) {
  enableCodel_.set(enableCodel, source);
}

void ThriftServerConfig::unsetEnableCodel(
    AttributeSource source, DynamicAttributeTag) {
  enableCodel_.unset(source);
}

void ThriftServerConfig::setTaskExpireTime(
    std::chrono::milliseconds timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  taskExpireTime_.set(timeout, source);
}

void ThriftServerConfig::unsetTaskExpireTime(
    AttributeSource source, DynamicAttributeTag) {
  taskExpireTime_.unset(source);
}

void ThriftServerConfig::setStreamExpireTime(
    std::chrono::milliseconds timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  streamExpireTime_.set(timeout, source);
}

void ThriftServerConfig::unsetStreamExpireTime(
    AttributeSource source, DynamicAttributeTag) {
  streamExpireTime_.unset(source);
}

void ThriftServerConfig::setQueueTimeout(
    std::chrono::milliseconds timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  queueTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetQueueTimeout(
    AttributeSource source, DynamicAttributeTag) {
  queueTimeout_.unset(source);
}

void ThriftServerConfig::setSocketQueueTimeout(
    folly::observer::Observer<std::chrono::nanoseconds> timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  socketQueueTimeout_.set(timeout, source);
}

void ThriftServerConfig::setSocketQueueTimeout(
    std::chrono::nanoseconds timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  socketQueueTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetSocketQueueTimeout(
    AttributeSource source, DynamicAttributeTag) {
  socketQueueTimeout_.unset(source);
}

void ThriftServerConfig::setSocketWriteTimeout(
    std::chrono::milliseconds timeout,
    AttributeSource source,
    DynamicAttributeTag) {
  socketWriteTimeout_.set(timeout, source);
}

void ThriftServerConfig::unsetSocketWriteTimeout(
    AttributeSource source, DynamicAttributeTag) {
  socketWriteTimeout_.unset(source);
}

void ThriftServerConfig::setIngressMemoryLimit(
    size_t ingressMemoryLimit, AttributeSource source, DynamicAttributeTag) {
  ingressMemoryLimit_.set(ingressMemoryLimit, source);
}

void ThriftServerConfig::unsetIngressMemoryLimit(
    AttributeSource source, DynamicAttributeTag) {
  ingressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setEgressMemoryLimit(
    size_t max, AttributeSource source, DynamicAttributeTag) {
  egressMemoryLimit_.set(max, source);
}

void ThriftServerConfig::unsetEgressMemoryLimit(
    AttributeSource source, DynamicAttributeTag) {
  egressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setMinPayloadSizeToEnforceIngressMemoryLimit(
    size_t minPayloadSizeToEnforceIngressMemoryLimit,
    AttributeSource source,
    DynamicAttributeTag) {
  minPayloadSizeToEnforceIngressMemoryLimit_.set(
      minPayloadSizeToEnforceIngressMemoryLimit, source);
}

void ThriftServerConfig::unsetMinPayloadSizeToEnforceIngressMemoryLimit(
    AttributeSource source, DynamicAttributeTag) {
  minPayloadSizeToEnforceIngressMemoryLimit_.unset(source);
}

void ThriftServerConfig::setEgressBufferBackpressureThreshold(
    size_t thresholdInBytes, AttributeSource source, DynamicAttributeTag) {
  egressBufferBackpressureThreshold_.set(thresholdInBytes, source);
}

void ThriftServerConfig::unsetEgressBufferBackpressureThreshold(
    AttributeSource source, DynamicAttributeTag) {
  egressBufferBackpressureThreshold_.unset(source);
}

void ThriftServerConfig::setEgressBufferRecoveryFactor(
    double recoveryFactor, AttributeSource source, DynamicAttributeTag) {
  recoveryFactor = std::max(0.0, std::min(1.0, recoveryFactor));
  egressBufferRecoveryFactor_.set(recoveryFactor, source);
}

void ThriftServerConfig::unsetEgressBufferRecoveryFactor(
    AttributeSource source, DynamicAttributeTag) {
  egressBufferRecoveryFactor_.unset(source);
}

void ThriftServerConfig::setPolledServiceHealthLiveness(
    std::chrono::milliseconds liveness,
    AttributeSource source,
    DynamicAttributeTag) {
  polledServiceHealthLiveness_.set(liveness, source);
}

void ThriftServerConfig::unsetPolledServiceHealthLiveness(
    AttributeSource source, DynamicAttributeTag) {
  polledServiceHealthLiveness_.unset(source);
}

void ThriftServerConfig::disableLegacyTransports(
    bool value, AttributeSource source, DynamicAttributeTag) {
  disableHeaderTransport_.set(value, source);
}

void ThriftServerConfig::unsetdisableLegacyTransports(
    AttributeSource source, DynamicAttributeTag) {
  disableHeaderTransport_.unset(source);
}

void ThriftServerConfig::setPerConnectionSocketOptions(
    folly::SocketOptionMap options,
    AttributeSource source,
    DynamicAttributeTag) {
  perConnectionSocketOptions_.set(std::move(options), source);
}

void ThriftServerConfig::unsetPerConnectionSocketOptions(
    AttributeSource source, DynamicAttributeTag) {
  perConnectionSocketOptions_.unset(source);
}

} // namespace thrift
} // namespace apache
