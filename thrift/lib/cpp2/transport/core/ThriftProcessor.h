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

#include <folly/io/IOBuf.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <stdint.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/ServerConfigs.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * Server side Thrift processor.  Accepts calls from the channel,
 * calls the function handler, and finally calls back into the channel
 * object.
 *
 * Only one object of this type is created when initializing the
 * server.  This object handles all calls across all threads.
 */
class ThriftProcessor {
 public:
  explicit ThriftProcessor(
      std::unique_ptr<AsyncProcessor> cpp2Processor,
      const apache::thrift::server::ServerConfigs& serverConfigs);

  virtual ~ThriftProcessor() = default;

  ThriftProcessor(const ThriftProcessor&) = delete;
  ThriftProcessor& operator=(const ThriftProcessor&) = delete;

  // Called once for each RPC from a channel object.  After performing
  // some checks and setup operations, this schedules the function
  // handler on a worker thread.  "headers" and "payload" are passed
  // to the handler as parameters.  For RPCs with streaming requests,
  // "payload" contains the non-stream parameters of the function.
  // "channel" is used to call back with the response for single
  // (non-streaming) responses, and to manage stream objects for RPCs
  // with streaming.
  virtual void onThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::shared_ptr<ThriftChannelIf> channel,
      std::unique_ptr<Cpp2ConnContext> connContext = nullptr) noexcept;

  // Called from the server initialization code if there's an update
  // to the thread manager used to manage the server
  void setThreadManager(apache::thrift::concurrency::ThreadManager* tm) {
    tm_ = tm;
  }

 private:
  // Object of the generated AsyncProcessor subclass.
  std::unique_ptr<AsyncProcessor> cpp2Processor_;
  // To access server specific fields.
  const apache::thrift::server::ServerConfigs& serverConfigs_;
  // Thread manager that is used to run thrift handlers.
  // Owned by the server initialization code.
  apache::thrift::concurrency::ThreadManager* tm_;
};

} // namespace thrift
} // namespace apache
