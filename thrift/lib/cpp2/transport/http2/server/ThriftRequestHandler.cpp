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

#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandler.h>

#include <thrift/lib/cpp2/transport/http2/common/H2ChannelFactory.h>

namespace apache {
namespace thrift {

using folly::IOBuf;
using proxygen::HTTPMessage;
using proxygen::ProxygenError;
using proxygen::RequestHandler;
using proxygen::UpgradeProtocol;

ThriftRequestHandler::ThriftRequestHandler(ThriftProcessor* processor)
    : processor_(processor) {}

ThriftRequestHandler::~ThriftRequestHandler() {}

void ThriftRequestHandler::onRequest(
    std::unique_ptr<HTTPMessage> headers) noexcept {
  auto val = headers->getHeaders().rawGet(kChannelVersionKey);
  int version = 1;
  try {
    version = folly::to<int>(val);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Channel version not set properly in header: " << val;
  }
  channel_ = H2ChannelFactory::createChannel(version, downstream_, processor_);
  channel_->onH2StreamBegin(std::move(headers));
}

void ThriftRequestHandler::onBody(std::unique_ptr<IOBuf> body) noexcept {
  channel_->onH2BodyFrame(std::move(body));
}

void ThriftRequestHandler::onEOM() noexcept {
  channel_->onH2StreamEnd();
}

void ThriftRequestHandler::onUpgrade(UpgradeProtocol /*prot*/) noexcept {}

void ThriftRequestHandler::requestComplete() noexcept {
  channel_->onH2StreamClosed(ProxygenError::kErrorNone);
  delete this;
}

void ThriftRequestHandler::onError(ProxygenError error) noexcept {
  if (channel_) {
    channel_->onH2StreamClosed(error);
  }
  delete this;
}

} // namespace thrift
} // namespace apache
