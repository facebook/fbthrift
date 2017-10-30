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

#include <proxygen/httpserver/RequestHandler.h>

#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPConstants.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/ProxygenErrorEnum.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/common/H2Channel.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * The HTTPServer object inside H2ThriftServer creates a new object of
 * this class type for every new incoming stream.  This is then
 * responsible to process the stream contents.  The bulk of the work
 * is passed on to ChannelIf objects which can support both server
 * and client code (this class is only on the server side).
 */
class ThriftRequestHandler : public proxygen::RequestHandler {
 public:
  explicit ThriftRequestHandler(
      ThriftProcessor* processor,
      uint32_t channelVersion);

  ~ThriftRequestHandler() override;

  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers)
      noexcept override;

  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

  void onEOM() noexcept override;

  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;

  void requestComplete() noexcept override;

  void onError(proxygen::ProxygenError error) noexcept override;

 private:
  // There is a single ThriftProcessor object which is used for all requests.
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_;
  // The channel version for this request.
  uint32_t channelVersion_;
  // The channel used with this request handler.  The request handler
  // creates the channel object.
  std::shared_ptr<H2Channel> channel_;
};

} // namespace thrift
} // namespace apache
