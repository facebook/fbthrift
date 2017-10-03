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
#include <chrono>

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
using proxygen::ProxygenError;
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
    VLOG(2) << "Channel received nothing from Proxygen and Thrift";
  } else if (receivedH2Stream_) {
    VLOG(2) << "Channel received message from Proxygen, but not Thrift";
  } else {
    VLOG(2) << "Channel received message from Thrift, but not Proxygen";
  }
}

bool SingleRpcChannel::supportsHeaders() const noexcept {
  return true;
}

void SingleRpcChannel::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(metadata);
  DCHECK(evb_->isInEventBaseThread());
  VLOG(2) << "sendThriftResponse:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (responseHandler_) {
    HTTPMessage msg;
    msg.setStatusCode(200);
    if (metadata->__isset.otherMetadata) {
      encodeHeaders(std::move(metadata->otherMetadata), msg);
    }
    responseHandler_->sendHeaders(msg);
    responseHandler_->sendBody(std::move(payload));
    responseHandler_->sendEOM();
  }
  receivedThriftRPC_ = true;
}

void SingleRpcChannel::cancel(int32_t /*seqId*/) noexcept {
  LOG(ERROR) << "cancel() not yet implemented";
}

void SingleRpcChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(metadata);
  DCHECK(payload);
  VLOG(2) << "sendThriftRequest:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (metadata->kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
      // TODO: probably remove following check.  This is going to be a legacy
      // channel and will not support streaming.  DCHECK somewhere that it
      // is oneway or twoway.
      metadata->kind == RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE) {
    DCHECK(callback);
    callback_ = std::move(callback);
  }
  try {
    httpTransaction_ = h2ClientConnection_->newTransaction(this);
  } catch (TTransportException& te) {
    if (callback_) {
      auto evb = callback_->getEventBase();
      evb->runInEventBaseThread([evbCallback = std::move(callback_),
                                 evbTe = std::move(te)]() mutable {
        evbCallback->onError(folly::make_exception_wrapper<TTransportException>(
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
  if (metadata->__isset.clientTimeoutMs) {
    DCHECK(metadata->clientTimeoutMs > 0);
    httpTransaction_->setIdleTimeout(
        std::chrono::milliseconds(metadata->clientTimeoutMs));
    metadata->otherMetadata[transport::THeader::CLIENT_TIMEOUT_HEADER] =
        folly::to<std::string>(metadata->clientTimeoutMs);
  }
  if (metadata->__isset.queueTimeoutMs) {
    DCHECK(metadata->queueTimeoutMs > 0);
    metadata->otherMetadata[transport::THeader::QUEUE_TIMEOUT_HEADER] =
        folly::to<std::string>(metadata->queueTimeoutMs);
  }
  if (metadata->__isset.priority) {
    metadata->otherMetadata[transport::THeader::PRIORITY_HEADER] =
        folly::to<std::string>(metadata->priority);
  }
  // TODO: We've left out a few header settings when copying from
  // HTTPClientChannel.  Verify that this is OK.
  encodeHeaders(std::move(metadata->otherMetadata), msg);
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

void SingleRpcChannel::setInput(int32_t, SubscriberRef) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

ThriftChannelIf::SubscriberRef SingleRpcChannel::getOutput(int32_t) noexcept {
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

void SingleRpcChannel::onH2StreamEnd() noexcept {
  VLOG(2) << "onH2StreamEnd";
  receivedH2Stream_ = true;
  if (!contents_) {
    contents_ = IOBuf::createCombined(0);
  }
  if (processor_) {
    // Server side
    bool oneway = isOneWay();
    auto metadata = std::make_unique<RequestRpcMetadata>();
    if (oneway) {
      metadata->kind = RpcKind::SINGLE_REQUEST_NO_RESPONSE;
    } else {
      metadata->kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
    }
    metadata->__isset.kind = true;
    auto iter = headers_->find(transport::THeader::CLIENT_TIMEOUT_HEADER);
    if (iter != headers_->end()) {
      try {
        metadata->clientTimeoutMs = folly::to<int64_t>(iter->second);
        metadata->__isset.clientTimeoutMs = true;
      } catch (const std::range_error&) {
        LOG(INFO) << "Bad client timeout " << iter->second;
      }
      headers_->erase(iter);
    }
    iter = headers_->find(transport::THeader::QUEUE_TIMEOUT_HEADER);
    if (iter != headers_->end()) {
      try {
        metadata->queueTimeoutMs = folly::to<int64_t>(iter->second);
        metadata->__isset.queueTimeoutMs = true;
      } catch (const std::range_error&) {
        LOG(INFO) << "Bad client timeout " << iter->second;
      }
      headers_->erase(iter);
    }
    iter = headers_->find(transport::THeader::PRIORITY_HEADER);
    if (iter != headers_->end()) {
      try {
        auto pr = static_cast<RpcPriority>(folly::to<int32_t>(iter->second));
        if (pr < RpcPriority::N_PRIORITIES) {
          metadata->priority = pr;
          metadata->__isset.priority = true;
        } else {
          LOG(INFO) << "Too large value for method priority " << iter->second;
        }
      } catch (const std::range_error&) {
        LOG(INFO) << "Bad method priority " << iter->second;
      }
      headers_->erase(iter);
    }
    if (!headers_->empty()) {
      metadata->otherMetadata = std::move(*headers_);
      metadata->__isset.otherMetadata = true;
    }
    processor_->onThriftRequest(
        std::move(metadata), std::move(contents_), shared_from_this());
    if (oneway) {
      // Send a dummy response since we need to do this with HTTP2.
      auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
      auto payload = IOBuf::createCombined(0);
      sendThriftResponse(std::move(responseMetadata), std::move(payload));
      receivedThriftRPC_ = true;
    }
  } else {
    // Client side
    DCHECK(httpTransaction_);
    if (callback_) {
      auto metadata = std::make_unique<ResponseRpcMetadata>();
      if (!headers_->empty()) {
        metadata->otherMetadata = std::move(*headers_);
        metadata->__isset.otherMetadata = true;
      }
      auto evb = callback_->getEventBase();
      evb->runInEventBaseThread([
        evbCallback = std::move(callback_),
        evbMetadata = std::move(metadata),
        evbContents = std::move(contents_)
      ]() mutable {
        evbCallback->onThriftResponse(
            std::move(evbMetadata), std::move(evbContents));
      });
    }
  }
}

void SingleRpcChannel::onH2StreamClosed(ProxygenError error) noexcept {
  VLOG(2) << "onH2StreamClosed";
  if (callback_) {
    std::unique_ptr<TTransportException> ex;
    if (error == ProxygenError::kErrorTimeout) {
      ex =
          std::make_unique<TTransportException>(TTransportException::TIMED_OUT);
    } else {
      // Some unknown error.
      VLOG(2) << "Network error before call completion";
      ex = std::make_unique<TTransportException>(
          TTransportException::NETWORK_ERROR);
    }
    // We assume that the connection is still valid.  If not, we will
    // get an error the next time we try to create a new transaction
    // and can deal with it then.
    // TODO: We could also try to understand the kind of error be looking
    // at "error" in more detail.
    ex->setOptions(TTransportException::CHANNEL_IS_VALID);
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([evbCallback = std::move(callback_),
                               evbEx = std::move(ex)]() mutable {
      evbCallback->onError(folly::make_exception_wrapper<TTransportException>(
          std::move(*evbEx)));
    });
  }
  H2ChannelIf::onH2StreamClosed(error);
}

bool SingleRpcChannel::isOneWay() noexcept {
  // TODO: currently hardwired to false.
  return false;
}

} // namespace thrift
} // namespace apache
