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

#include <folly/ExceptionWrapper.h>
#include <thrift/lib/cpp/ContextStack.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp2/async/RequestCallback.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/omnithrift/client/OmniClientBase.h>

namespace apache {
namespace thrift {
namespace omniclient {

/**
 * The Omniclient class can send requests to any Thrift service given:
 *
 * (1) A RequestChannel; or
 * (2) The service's hostname, port, and service name.
 *
 * The semifuture_send function takes in a function name and its arguments
 * encoded in ProtocolIn, and sends back the response encoded in ProtocolOut,
 * wrapped in a folly::SemiFuture.
 */
template <class ProtocolIn, class ProtocolOut>
class OmniClient : public OmniClientBase {
 public:
  explicit OmniClient(
      std::shared_ptr<apache::thrift::RequestChannel> channel,
      std::string serviceName)
      : OmniClientBase(std::move(channel)),
        serviceName_(std::move(serviceName)) {}

  virtual ~OmniClient() override = default;

  /**
   * Returns a SemiFuture with the request response encoded in ProtocolOut.
   * The arguments must be encoded according to ProtocolIn.
   */
  folly::SemiFuture<OmniClientWrappedResponse> semifuture_sendWrapped(
      const std::string& functionName,
      const std::string& encodedArgs,
      const std::unordered_map<std::string, std::string>& headers = {})
      override;

 private:
  /**
   * Sends the request to the Thrift service through the RequestChannel.
   */
  void sendImpl(
      apache::thrift::RpcOptions rpcOptions,
      const std::string& args,
      const std::string& functionName,
      const char* serviceNameForContextStack,
      const char* functionNameForContextStack,
      std::unique_ptr<apache::thrift::RequestCallback> callback);

  std::string serviceName_;
};

} // namespace omniclient
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/omnithrift/client/OmniClient-inl.h>
