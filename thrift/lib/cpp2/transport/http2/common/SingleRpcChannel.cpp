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
#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>

#include <folly/io/async/EventBaseManager.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

// NOTE: Implementation currently only supports SINGLE_REQUEST_SINGLE_RESPONSE.

namespace apache {
namespace thrift {

using std::map;
using std::string;
using folly::EventBaseManager;
using folly::IOBuf;
using proxygen::HTTPMessage;
using proxygen::HTTPTransaction;
using proxygen::ResponseHandler;

SingleRpcChannel::SingleRpcChannel(
    ThriftProcessor* processor,
    ResponseHandler* toHttp2)
    : H2ChannelIf(processor, toHttp2) {
  evb_ = EventBaseManager::get()->getExistingEventBase();
}

SingleRpcChannel::SingleRpcChannel(HTTPTransaction* toHttp2)
    : H2ChannelIf(toHttp2) {
  evb_ = EventBaseManager::get()->getExistingEventBase();
}

SingleRpcChannel::~SingleRpcChannel() {
  if (receivedH2Stream_ && receivedThriftRPC_) {
    return;
  }
  if (!receivedH2Stream_ && !receivedThriftRPC_) {
    LOG(WARNING) << "Channel received nothing from Proxygen and Thrift";
  } else if (receivedH2Stream_) {
    LOG(ERROR) << "Channel received message from Proxygen, but not Thrift";
  } else {
    LOG(ERROR) << "Channel received message from Thrift, but not Proxygen";
  }
}

void SingleRpcChannel::sendThriftResponse(
    uint32_t seqId,
    std::unique_ptr<std::map<std::string, std::string>> headers,
    std::unique_ptr<folly::IOBuf> payload) noexcept {
  DCHECK(seqId == 0);
  DCHECK(evb_->isInEventBaseThread());
  if (responseHandler_) {
    std::unique_ptr<HTTPMessage> msg = std::make_unique<HTTPMessage>();
    msg->setStatusCode(200);
    auto& msgHeaders = msg->getHeaders();
    for (auto it = headers->begin(); it != headers->end(); ++it) {
      msgHeaders.rawSet(it->first, it->second);
    }
    responseHandler_->sendHeaders(*msg);
    responseHandler_->sendBody(std::move(payload));
    responseHandler_->sendEOM();
  }
  receivedThriftRPC_ = true;
}

void SingleRpcChannel::cancel(uint32_t /*seqId*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

void SingleRpcChannel::sendThriftRequest(
    std::unique_ptr<FunctionInfo> /*functionInfo*/,
    std::unique_ptr<std::map<std::string, std::string>> /*headers*/,
    std::unique_ptr<folly::IOBuf> /*payload*/,
    std::unique_ptr<ThriftClientCallback> /*callback*/) noexcept {
  LOG(FATAL) << "Client side not yet implemented.";

  /* TODO: snippet copied from HTTPClientChannel - reorganize
  proxygen::HTTPMessage msg;
  msg.setMethod(proxygen::HTTPMethod::POST);
  msg.setURL(httpUrl_);
  msg.setHTTPVersion(1, 1);
  msg.setIsChunked(false);
  auto& headers = msg.getHeaders();
  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_HOST, httpHost_);
  headers.set(
      proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
      "application/x-thrift");

  switch (protocolId_) {
    case protocol::T_BINARY_PROTOCOL:
      headers.set(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "binary");
      break;
    case protocol::T_COMPACT_PROTOCOL:
      headers.set(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "compact");
      break;
    case protocol::T_JSON_PROTOCOL:
      headers.set(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "json");
      break;
    case protocol::T_SIMPLE_JSON_PROTOCOL:
      headers.set(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
          "simplejson");
      break;
    default:
      // Do nothing
      break;
  }
  */
}

void SingleRpcChannel::cancel(ThriftClientCallback* /*callback*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

folly::EventBase* SingleRpcChannel::getEventBase() noexcept {
  return evb_;
}

/*
void SingleRpcChannel::setInput(uint32_t, SubscriberRef) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

ThriftChannelIf::SubscriberRef SingleRpcChannel::getOutput(uint32_t) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}
*/

void SingleRpcChannel::onH2StreamBegin(
    std::unique_ptr<HTTPMessage> headers) noexcept {
  headers_ = std::make_unique<map<string, string>>();
  auto copyHeaders = [&](const string& key, const string& val) {
    headers_->insert(make_pair(key, val));
  };
  headers->getHeaders().forEach(copyHeaders);
}

void SingleRpcChannel::onH2BodyFrame(std::unique_ptr<IOBuf> contents) noexcept {
  if (contents_) {
    contents_->prependChain(std::move(contents));
  } else {
    contents_ = std::move(contents);
  }
}

void SingleRpcChannel::onH2StreamEnd() noexcept {
  receivedH2Stream_ = true;
  auto finfo = std::make_unique<FunctionInfo>();
  finfo->kind = SINGLE_REQUEST_SINGLE_RESPONSE;
  finfo->seqId = 0;
  // "name" and "protocol" fields of "finfo" are not used right now.
  processor_->onThriftRequest(
      std::move(finfo),
      std::move(headers_),
      std::move(contents_),
      shared_from_this());
}

} // namespace thrift
} // namespace apache
