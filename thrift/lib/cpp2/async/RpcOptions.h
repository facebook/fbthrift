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
#include <optional>
#include <string>

#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ClientStreamBridge.h>

namespace apache {
namespace thrift {
class InteractionId;

/**
 * RpcOptions class to set per-RPC options (such as request timeout).
 */
class RpcOptions {
 public:
  typedef apache::thrift::concurrency::PRIORITY PRIORITY;

  /**
   * NOTE: This only sets the receive timeout, and not the send timeout on
   * transport. Probably you want to use HeaderClientChannel::setTimeout()
   */
  RpcOptions& setTimeout(std::chrono::milliseconds timeout);
  std::chrono::milliseconds getTimeout() const;

  RpcOptions& setPriority(PRIORITY priority);
  PRIORITY getPriority() const;

  // Do timeouts apply only on the client side?
  RpcOptions& setClientOnlyTimeouts(bool val);
  bool getClientOnlyTimeouts() const;

  RpcOptions& setEnableChecksum(bool val);
  bool getEnableChecksum() const;

  RpcOptions& setChunkTimeout(std::chrono::milliseconds chunkTimeout);
  std::chrono::milliseconds getChunkTimeout() const;

  // Only one of these may be set
  RpcOptions& setChunkBufferSize(int32_t chunkBufferSize);
  RpcOptions& setMemoryBufferSize(size_t targetBytes, int32_t initialChunks);
  int32_t getChunkBufferSize() const;
  const BufferOptions& getBufferOptions() const;

  RpcOptions& setQueueTimeout(std::chrono::milliseconds queueTimeout);
  std::chrono::milliseconds getQueueTimeout() const;

  RpcOptions& setOverallTimeout(std::chrono::milliseconds overallTimeout);
  std::chrono::milliseconds getOverallTimeout() const;

  RpcOptions& setProcessingTimeout(std::chrono::milliseconds processingTimeout);
  std::chrono::milliseconds getProcessingTimeout() const;

  /**
   * Set routing key for (e.g. consistent hashing based) routing.
   *
   * @param routingKey routing key, e.g. consistent hashing seed
   * @return reference to this object
   */
  RpcOptions& setRoutingKey(std::string routingKey);
  const std::string& getRoutingKey() const;

  /**
   * Set shard ID for sending request to sharding-based services.
   *
   * @param shardId shard ID to use for service discovery
   * @return reference to this object
   */
  RpcOptions& setShardId(std::string shardId);
  const std::string& getShardId() const;

  void setWriteHeader(const std::string& key, const std::string& value);
  const transport::THeader::StringToStringMap& getWriteHeaders() const;
  transport::THeader::StringToStringMap releaseWriteHeaders();

  void setReadHeaders(transport::THeader::StringToStringMap&& readHeaders);
  const transport::THeader::StringToStringMap& getReadHeaders() const;

  RpcOptions& setEnablePageAlignment(bool enablePageAlignment);
  bool getEnablePageAlignment() const;

  // Primarily used by generated code
  RpcOptions& setInteractionId(const InteractionId& id);
  int64_t getInteractionId() const;

  RpcOptions& setLoggingContext(std::string loggingContext);
  const std::string& getLoggingContext() const;

  RpcOptions& setRoutingData(std::shared_ptr<void> data);
  const std::shared_ptr<void>& getRoutingData() const;

 private:
  std::chrono::milliseconds timeout_{0};
  std::chrono::milliseconds chunkTimeout_{0};
  std::chrono::milliseconds queueTimeout_{0};
  std::chrono::milliseconds overallTimeout_{0};
  std::chrono::milliseconds processingTimeout_{0};
  uint8_t priority_{apache::thrift::concurrency::N_PRIORITIES};
  bool clientOnlyTimeouts_{false};
  bool enableChecksum_{false};
  bool enablePageAlignment_{false};
  BufferOptions bufferOptions_;
  int64_t interactionId_{0};

  std::string routingKey_;
  std::string shardId_;

  // For sending and receiving headers.
  std::optional<transport::THeader::StringToStringMap> writeHeaders_;
  std::optional<transport::THeader::StringToStringMap> readHeaders_;

  // Custom data about the request for logging and analysis.
  std::string loggingContext_;

  // Custom data passed back from the routing layer.
  std::shared_ptr<void> routingData_;
};

} // namespace thrift
} // namespace apache
