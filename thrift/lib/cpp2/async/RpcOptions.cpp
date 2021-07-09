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

#include <thrift/lib/cpp2/async/RpcOptions.h>

#include <thrift/lib/cpp2/async/Interaction.h>

namespace apache {
namespace thrift {

RpcOptions& RpcOptions::setTimeout(std::chrono::milliseconds timeout) {
  timeout_ = timeout;
  return *this;
}

std::chrono::milliseconds RpcOptions::getTimeout() const {
  return timeout_;
}

RpcOptions& RpcOptions::setPriority(RpcOptions::PRIORITY priority) {
  priority_ = static_cast<uint8_t>(priority);
  return *this;
}

RpcOptions::PRIORITY RpcOptions::getPriority() const {
  return static_cast<RpcOptions::PRIORITY>(priority_);
}

RpcOptions& RpcOptions::setClientOnlyTimeouts(bool val) {
  clientOnlyTimeouts_ = val;
  return *this;
}

bool RpcOptions::getClientOnlyTimeouts() const {
  return clientOnlyTimeouts_;
}

RpcOptions& RpcOptions::setEnableChecksum(bool val) {
  enableChecksum_ = val;
  return *this;
}

bool RpcOptions::getEnableChecksum() const {
  return enableChecksum_;
}

RpcOptions& RpcOptions::setChunkTimeout(
    std::chrono::milliseconds chunkTimeout) {
  chunkTimeout_ = chunkTimeout;
  return *this;
}

std::chrono::milliseconds RpcOptions::getChunkTimeout() const {
  return chunkTimeout_;
}

RpcOptions& RpcOptions::setChunkBufferSize(int32_t chunkBufferSize) {
  CHECK_EQ(bufferOptions_.memSize, 0)
      << "Only one of setMemoryBufferSize and setChunkBufferSize should be called";
  bufferOptions_.chunkSize = chunkBufferSize;
  return *this;
}

RpcOptions& RpcOptions::setMemoryBufferSize(
    size_t targetBytes, int32_t initialChunks) {
  CHECK_EQ(bufferOptions_.chunkSize, 100)
      << "Only one of setMemoryBufferSize and setChunkBufferSize should be called";
  CHECK_GT(targetBytes, 0);
  bufferOptions_.memSize = targetBytes;
  bufferOptions_.chunkSize = initialChunks;
  return *this;
}

int32_t RpcOptions::getChunkBufferSize() const {
  return bufferOptions_.chunkSize;
}

const BufferOptions& RpcOptions::getBufferOptions() const {
  return bufferOptions_;
}

RpcOptions& RpcOptions::setQueueTimeout(
    std::chrono::milliseconds queueTimeout) {
  queueTimeout_ = queueTimeout;
  return *this;
}

std::chrono::milliseconds RpcOptions::getQueueTimeout() const {
  return queueTimeout_;
}

RpcOptions& RpcOptions::setOverallTimeout(
    std::chrono::milliseconds overallTimeout) {
  overallTimeout_ = overallTimeout;
  return *this;
}

std::chrono::milliseconds RpcOptions::getOverallTimeout() const {
  return overallTimeout_;
}

RpcOptions& RpcOptions::setProcessingTimeout(
    std::chrono::milliseconds processingTimeout) {
  processingTimeout_ = processingTimeout;
  return *this;
}

std::chrono::milliseconds RpcOptions::getProcessingTimeout() const {
  return processingTimeout_;
}

RpcOptions& RpcOptions::setRoutingKey(std::string routingKey) {
  routingKey_ = std::move(routingKey);
  return *this;
}

const std::string& RpcOptions::getRoutingKey() const {
  return routingKey_;
}

RpcOptions& RpcOptions::setShardId(std::string shardId) {
  shardId_ = std::move(shardId);
  return *this;
}

const std::string& RpcOptions::getShardId() const {
  return shardId_;
}

void RpcOptions::setReadHeaders(
    transport::THeader::StringToStringMap&& readHeaders) {
  readHeaders_ = std::move(readHeaders);
}

const transport::THeader::StringToStringMap& RpcOptions::getReadHeaders()
    const {
  return readHeaders_;
}

void RpcOptions::setWriteHeader(
    const std::string& key, const std::string& value) {
  writeHeaders_[key] = value;
}

const transport::THeader::StringToStringMap& RpcOptions::getWriteHeaders()
    const {
  return writeHeaders_;
}

transport::THeader::StringToStringMap RpcOptions::releaseWriteHeaders() {
  transport::THeader::StringToStringMap headers;
  writeHeaders_.swap(headers);
  return headers;
}

RpcOptions& RpcOptions::setEnablePageAlignment(bool enablePageAlignment) {
  enablePageAlignment_ = enablePageAlignment;
  return *this;
}

bool RpcOptions::getEnablePageAlignment() const {
  return enablePageAlignment_;
}

RpcOptions& RpcOptions::setInteractionId(const InteractionId& id) {
  interactionId_ = id;
  DCHECK_GT(interactionId_, 0);
  return *this;
}

int64_t RpcOptions::getInteractionId() const {
  return interactionId_;
}

RpcOptions& RpcOptions::setLoggingContext(std::string loggingContext) {
  loggingContext_ = std::move(loggingContext);
  return *this;
}

const std::string& RpcOptions::getLoggingContext() const {
  return loggingContext_;
}

RpcOptions& RpcOptions::setRoutingData(std::shared_ptr<void> data) {
  routingData_ = std::move(data);
  return *this;
}

const std::shared_ptr<void>& RpcOptions::getRoutingData() const {
  return routingData_;
}

} // namespace thrift
} // namespace apache
