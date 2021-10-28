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

#include <folly/Expected.h>
#include <folly/futures/Future.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/RequestCallback.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace thrift {
namespace py3lite {
namespace client {

using RequestChannel_ptr = std::unique_ptr<
    apache::thrift::RequestChannel,
    folly::DelayedDestruction::Destructor>;

struct OmniClientResponseWithHeaders {
  folly::Expected<std::unique_ptr<folly::IOBuf>, folly::exception_wrapper> buf;
  apache::thrift::transport::THeader::StringToStringMap headers;
};

/**
 * The Omniclient class can send requests to any Thrift service given
 * a RequestChannel.
 */
class OmniClient : public apache::thrift::TClientBase {
 public:
  explicit OmniClient(RequestChannel_ptr channel);
  ~OmniClient();

  OmniClientResponseWithHeaders sync_send(
      const std::string& serviceName,
      const std::string& functionName,
      std::unique_ptr<folly::IOBuf> args,
      const std::unordered_map<std::string, std::string>& headers = {});

  OmniClientResponseWithHeaders sync_send(
      const std::string& serviceName,
      const std::string& functionName,
      const std::string& args,
      const std::unordered_map<std::string, std::string>& headers = {});

  void oneway_send(
      const std::string& serviceName,
      const std::string& functionName,
      std::unique_ptr<folly::IOBuf> args,
      const std::unordered_map<std::string, std::string>& headers = {});

  void oneway_send(
      const std::string& serviceName,
      const std::string& functionName,
      const std::string& args,
      const std::unordered_map<std::string, std::string>& headers = {});

  /**
   * The semifuture_send function takes in a function name and its arguments
   * encoded in channel protocol, and sends back the raw response with headers,
   * wrapped in a folly::SemiFuture.
   */
  folly::SemiFuture<OmniClientResponseWithHeaders> semifuture_send(
      const std::string& serviceName,
      const std::string& functionName,
      std::unique_ptr<folly::IOBuf> args,
      const std::unordered_map<std::string, std::string>& headers = {});

  folly::SemiFuture<OmniClientResponseWithHeaders> semifuture_send(
      const std::string& serviceName,
      const std::string& functionName,
      const std::string& args,
      const std::unordered_map<std::string, std::string>& headers = {});

  uint16_t getChannelProtocolId();

 private:
  /**
   * Sends the request to the Thrift service through the RequestChannel.
   */
  void sendImpl(
      apache::thrift::RpcOptions rpcOptions,
      const std::string& functionName,
      std::unique_ptr<folly::IOBuf> args,
      const char* serviceNameForContextStack,
      const char* functionNameForContextStack,
      std::unique_ptr<apache::thrift::RequestCallback> callback);

  std::shared_ptr<apache::thrift::RequestChannel> channel_;
};

} // namespace client
} // namespace py3lite
} // namespace thrift
