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
#include <stdint.h>
#include <thrift/lib/cpp2/transport/core/FunctionInfo.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <map>
#include <memory>
#include <string>

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
  ThriftProcessor(
      std::unique_ptr<AsyncProcessor> cpp2Processor,
      apache::thrift::concurrency::ThreadManager* tm);
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
      std::unique_ptr<FunctionInfo> functionInfo,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload,
      std::shared_ptr<ThriftChannelIf> channel) noexcept;

  // Called from the channel to cancel a previous call to
  // "onThriftRequest()".  The processor does not have to call back
  // once this method has been called, and the call back will be
  // ignored if it does.
  virtual void cancel(uint32_t seqId, ThriftChannelIf* channel) noexcept;

 private:
  // Object of the generated AsyncProcessor subclass.
  std::unique_ptr<AsyncProcessor> cpp2Processor_;
  // Thread manager that is used to run thrift handlers.
  // Owned by the server initialization code.
  apache::thrift::concurrency::ThreadManager* tm_;

  /**
   * Manages per-RPC state.  There is one of these objects for each
   * RPC.
   */
  class ThriftRequest : public ResponseChannel::Request {
   public:
    ThriftRequest(
        std::shared_ptr<ThriftChannelIf> channel,
        std::unique_ptr<transport::THeader> header,
        std::unique_ptr<Cpp2RequestContext> context,
        std::unique_ptr<Cpp2ConnContext> connContext,
        uint32_t seqId)
        : channel_(channel),
          header_(std::move(header)),
          context_(std::move(context)),
          connContext_(std::move(connContext)),
          seqId_(seqId),
          active_(true) {}

    bool isActive() override {
      return active_;
    }

    void cancel() override {
      active_ = false;
    }

    // TODO: Implement isOneway
    bool isOneway() override {
      return false;
    }

    void sendReply(
        std::unique_ptr<folly::IOBuf>&& buf,
        apache::thrift::MessageChannel::SendCallback* /* cb */ =
            nullptr) override {
      auto headers = std::make_unique<std::map<std::string, std::string>>();
      channel_->sendThriftResponse(seqId_, std::move(headers), std::move(buf));
    }

    // TODO: Implement sendErrorWrapped
    void sendErrorWrapped(
        folly::exception_wrapper /* ex */,
        std::string /* exCode */,
        apache::thrift::MessageChannel::SendCallback* /* cb */ =
            nullptr) override {}

   private:
    std::shared_ptr<ThriftChannelIf> channel_;
    std::unique_ptr<transport::THeader> header_;
    std::unique_ptr<Cpp2RequestContext> context_;
    std::unique_ptr<Cpp2ConnContext> connContext_;
    uint32_t seqId_;
    std::atomic<bool> active_;
  };
};

} // namespace thrift
} // namespace apache
