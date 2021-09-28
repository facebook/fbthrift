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

#include <folly/futures/Future.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/omnithrift/client/DeclaredException.h>

namespace apache {
namespace thrift {
namespace omniclient {

/**
 * Wraps a response from a thrift request.
 * - If a requests results in a normal response, `buf` will contain a
 *   `folly::IOBuf` with the serialized response.
 * - Otherwise, `buf` will wrap an `DeclaredException` or
 *   `TApplicationException` encoding the exception thrown by the callee.
 * - `headers` contains the thrift headers included with the response.
 */
struct OmniClientWrappedResponse {
  folly::Expected<folly::IOBuf, folly::exception_wrapper> buf;
  apache::thrift::transport::THeader::StringToStringMap headers;
};

class OmniClientBase : public apache::thrift::TClientBase {
 public:
  explicit OmniClientBase(
      std::shared_ptr<apache::thrift::RequestChannel> channel)
      : channel_(std::move(channel)) {}

  virtual ~OmniClientBase() = default;

  virtual folly::SemiFuture<OmniClientWrappedResponse> semifuture_sendWrapped(
      const std::string& functionName,
      const std::string& encodedArgs,
      const std::unordered_map<std::string, std::string>& headers = {}) = 0;

  folly::SemiFuture<folly::IOBuf> semifuture_send(
      const std::string& functionName,
      const std::string& encodedArgs,
      const std::unordered_map<std::string, std::string>& headers = {});

  std::shared_ptr<apache::thrift::RequestChannel> getChannel() const {
    return channel_;
  }

  std::shared_ptr<apache::thrift::RequestChannel>&& moveChannel() {
    return std::move(channel_);
  }

  std::shared_ptr<apache::thrift::HeaderChannel> getHeaderChannel() const {
    return std::dynamic_pointer_cast<apache::thrift::HeaderChannel>(channel_);
  }

 protected:
  std::shared_ptr<apache::thrift::RequestChannel> channel_;
};

} // namespace omniclient
} // namespace thrift
} // namespace apache
