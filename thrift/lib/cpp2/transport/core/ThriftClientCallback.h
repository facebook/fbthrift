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
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * Callback object for a single response RPC.
 */
class ThriftClientCallback : public folly::HHWheelTimer::Callback {
 public:
  ThriftClientCallback(
      folly::EventBase* evb,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      bool isSecurityActive,
      uint16_t protoId,
      std::chrono::milliseconds timeout);

  virtual ~ThriftClientCallback();

  ThriftClientCallback(const ThriftClientCallback&) = delete;
  ThriftClientCallback& operator=(const ThriftClientCallback&) = delete;

  // Called from the channel once the request has been sent.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  void onThriftRequestSent();

  // Called from the channel at the end of a single response RPC.
  // This is not called for streaming response RPCs.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  void onThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload) noexcept;

  // Called from the channel in case of an error RPC (instead of
  // calling "onThriftResponse()").
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  void onError(folly::exception_wrapper ex) noexcept;

  // Returns the event base on which calls to "onThriftRequestSent()",
  // "onThriftResponse()", and "onError()" must be scheduled.
  folly::EventBase* getEventBase() const;

 protected:
  void timeoutExpired() noexcept override;
  void callbackCanceled() noexcept override;

 public:
  // The default timeout for a Thrift RPC.
  static const std::chrono::milliseconds kDefaultTimeout;

 private:
  folly::EventBase* evb_;
  std::unique_ptr<RequestCallback> cb_;
  std::unique_ptr<ContextStack> ctx_;
  bool isSecurityActive_;
  uint16_t protoId_;

  bool active_;
  std::chrono::milliseconds timeout_;
};

} // namespace thrift
} // namespace apache
