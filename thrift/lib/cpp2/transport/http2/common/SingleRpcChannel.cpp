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
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/utils/Base64.h>
#include <proxygen/lib/utils/Logging.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>

namespace apache {
namespace thrift {

using apache::thrift::transport::TTransportException;
using std::map;
using std::string;
using folly::EventBaseManager;
using folly::IOBuf;
using proxygen::HTTPHeaderCode;
using proxygen::HTTPMessage;
using proxygen::HTTPMethod;
using proxygen::HTTPTransaction;
using proxygen::IOBufPrinter;
using proxygen::ResponseHandler;

SingleRpcChannel::SingleRpcChannel(
    ResponseHandler* toHttp2,
    ThriftProcessor* processor)
    : H2ChannelIf(toHttp2), processor_(processor) {
  evb_ = EventBaseManager::get()->getExistingEventBase();
}

SingleRpcChannel::SingleRpcChannel(
    H2ClientConnection* toHttp2,
    const string& httpHost,
    const string& httpUrl)
    : H2ChannelIf(toHttp2), httpHost_(httpHost), httpUrl_(httpUrl) {
  evb_ = toHttp2->getEventBase();
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

bool SingleRpcChannel::supportsHeaders() const noexcept {
  return true;
}

void SingleRpcChannel::sendThriftResponse(
    uint32_t seqId,
    std::unique_ptr<map<string, string>> headers,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(seqId == 0);
  DCHECK(evb_->isInEventBaseThread());
  VLOG(2) << "sendThriftResponse:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (responseHandler_) {
    HTTPMessage msg;
    msg.setStatusCode(200);
    encodeHeaders(std::move(*headers), msg);
    responseHandler_->sendHeaders(msg);
    responseHandler_->sendBody(std::move(payload));
    responseHandler_->sendEOM();
  }
  receivedThriftRPC_ = true;
}

void SingleRpcChannel::cancel(uint32_t /*seqId*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

void SingleRpcChannel::sendThriftRequest(
    std::unique_ptr<FunctionInfo> functionInfo,
    std::unique_ptr<map<string, string>> headers,
    std::unique_ptr<IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(functionInfo);
  DCHECK(payload);
  VLOG(2) << "sendThriftRequest:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (functionInfo->kind == SINGLE_REQUEST_SINGLE_RESPONSE ||
      functionInfo->kind == STREAMING_REQUEST_SINGLE_RESPONSE) {
    DCHECK(callback);
    callback_ = std::move(callback);
  }
  try {
    httpTransaction_ = h2ClientConnection_->newTransaction(this);
  } catch (TTransportException& te) {
    if (callback_) {
      auto evb = callback_->getEventBase();
      evb->runInEventBaseThread([
        evbCallback = std::move(callback_),
        evbTe = std::move(te)
      ]() mutable {
        evbCallback->cancel(folly::make_exception_wrapper<TTransportException>(
            std::move(evbTe)));
      });
    }
    return;
  }
  HTTPMessage msg;
  msg.setMethod(HTTPMethod::POST);
  msg.setURL(httpUrl_);
  if (!httpHost_.empty()) {
    auto& msgHeaders = msg.getHeaders();
    msgHeaders.set(HTTPHeaderCode::HTTP_HEADER_HOST, httpHost_);
  }
  // TODO: We've left out a few header settings when copying from
  // HTTPClientChannel.  Verify that this is OK.
  encodeHeaders(std::move(*headers), msg);
  httpTransaction_->sendHeaders(msg);
  httpTransaction_->sendBody(std::move(payload));
  httpTransaction_->sendEOM();
  receivedThriftRPC_ = true;
}

void SingleRpcChannel::cancel(ThriftClientCallback* /*callback*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

folly::EventBase* SingleRpcChannel::getEventBase() noexcept {
  return evb_;
}

void SingleRpcChannel::setInput(uint32_t, SubscriberRef) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

ThriftChannelIf::SubscriberRef SingleRpcChannel::getOutput(uint32_t) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

void SingleRpcChannel::onH2StreamBegin(
    std::unique_ptr<HTTPMessage> headers) noexcept {
  VLOG(2) << "onH2StreamBegin";
  headers_ = std::make_unique<map<string, string>>();
  decodeHeaders(*headers, *headers_);
}

void SingleRpcChannel::onH2BodyFrame(std::unique_ptr<IOBuf> contents) noexcept {
  VLOG(2) << "onH2BodyFrame: " << std::endl
          << IOBufPrinter::printHexFolly(contents.get(), true);
  if (contents_) {
    contents_->prependChain(std::move(contents));
  } else {
    contents_ = std::move(contents);
  }
}

bool SingleRpcChannel::isOneWay() noexcept {
  // TODO: currently hardwired to false.
  return false;
}

void SingleRpcChannel::onH2StreamEnd() noexcept {
  VLOG(2) << "onH2StreamEnd";
  receivedH2Stream_ = true;
  if (!contents_) {
    contents_ = IOBuf::createCombined(0);
  }
  if (processor_) {
    // Server side
    bool oneway = isOneWay();
    auto finfo = std::make_unique<FunctionInfo>();
    if (oneway) {
      finfo->kind = SINGLE_REQUEST_NO_RESPONSE;
    } else {
      finfo->kind = SINGLE_REQUEST_SINGLE_RESPONSE;
    }
    finfo->seqId = 0;
    // "name" and "protocol" fields of "finfo" are not used right now.
    processor_->onThriftRequest(
        std::move(finfo),
        std::move(headers_),
        std::move(contents_),
        shared_from_this());
    if (oneway) {
      // Send a dummy response since we need to do this with HTTP2.
      auto headers = std::make_unique<map<string, string>>();
      auto payload = IOBuf::createCombined(0);
      sendThriftResponse(0, std::move(headers), std::move(payload));
      receivedThriftRPC_ = true;
    }
  } else {
    // Client side
    DCHECK(httpTransaction_);
    if (callback_) {
      auto evb = callback_->getEventBase();
      evb->runInEventBaseThread([
        evbCallback = std::move(callback_),
        evbHeaders = std::move(headers_),
        evbContents = std::move(contents_)
      ]() mutable {
        evbCallback->onThriftResponse(
            std::move(evbHeaders), std::move(evbContents));
      });
    }
  }
}

void SingleRpcChannel::onH2StreamClosed() noexcept {
  VLOG(2) << "onH2StreamClosed";
  if (callback_) {
    // Stream died before callback happened.
    LOG(ERROR) << "Network error before call completion";
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([evbCallback = std::move(callback_)]() mutable {
      TTransportException ex(
          TTransportException::NETWORK_ERROR,
          "Network error before call completion");
      evbCallback->cancel(
          folly::make_exception_wrapper<TTransportException>(std::move(ex)));
    });
  }
  H2ChannelIf::onH2StreamClosed();
}

} // namespace thrift
} // namespace apache
