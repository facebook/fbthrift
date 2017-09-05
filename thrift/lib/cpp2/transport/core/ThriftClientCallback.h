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

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <map>
#include <memory>
#include <string>

namespace apache {
namespace thrift {

/**
 * Callback object for a single response RPC.
 */
class ThriftClientCallback {
 public:
  ThriftClientCallback(
      folly::EventBase* evb,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      bool isSecurityActive,
      uint16_t protoId);

  virtual ~ThriftClientCallback() = default;

  ThriftClientCallback(const ThriftClientCallback&) = delete;
  ThriftClientCallback& operator=(const ThriftClientCallback&) = delete;

  // Called from the channel at the end of a single response RPC.
  // This is not called for streaming response RPCs.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  void onThriftResponse(
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload) noexcept;

  // Can be called from the channel to cancel a RPC (instead of
  // calling "onThriftResponse()").  Once this is called, the channel
  // does not have to perform any additional calls such as
  // "onThriftResponse()", or closing of streams.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  void cancel(folly::exception_wrapper ex) noexcept;

  // Returns the event base on which calls to "onThriftResponse()"
  // and "cancel()" must be scheduled.
  folly::EventBase* getEventBase() const;

 private:
  folly::EventBase* evb_;
  std::unique_ptr<RequestCallback> cb_;
  std::unique_ptr<ContextStack> ctx_;
  bool isSecurityActive_;
  uint16_t protoId_;
};

} // namespace thrift
} // namespace apache
