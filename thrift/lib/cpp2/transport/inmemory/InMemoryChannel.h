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

#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>

#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

namespace apache {
namespace thrift {

class InMemoryChannel : public ThriftChannelIf {
 public:
  InMemoryChannel(ThriftProcessor* processor, folly::EventBase* evb);
  virtual ~InMemoryChannel() override = default;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload) noexcept override;

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

  void setInput(int32_t seqId, SubscriberRef sink) noexcept override;

  SubscriberRef getOutput(int32_t seqId) noexcept override;

 private:
  // The thrift processor used to execute RPCs.
  // Owned by InMemoryConnection.
  ThriftProcessor* processor_{nullptr};
  std::unique_ptr<ThriftClientCallback> callback_;
  folly::EventBase* evb_;
};

} // namespace thrift
} // namespace apache
