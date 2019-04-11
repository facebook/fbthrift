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

#include <chrono>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBaseManager.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/utils/Base64.h>
#include <proxygen/lib/utils/Logging.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>

namespace apache {
namespace thrift {

using apache::thrift::protocol::TProtocolException;
using apache::thrift::transport::TTransportException;
using folly::EventBase;
using folly::EventBaseManager;
using folly::IOBuf;
using proxygen::HTTPHeaderCode;
using proxygen::HTTPMessage;
using proxygen::HTTPMethod;
using proxygen::HTTPTransaction;
using proxygen::IOBufPrinter;
using proxygen::ProxygenError;
using proxygen::ResponseHandler;
using std::map;
using std::string;

static constexpr folly::StringPiece RPC_KIND = "rpckind";

SingleRpcChannel::SingleRpcChannel(
    ResponseHandler* toHttp2,
    ThriftProcessor* processor)
    : H2Channel(toHttp2), processor_(processor) {
  evb_ = EventBaseManager::get()->getExistingEventBase();
}

SingleRpcChannel::SingleRpcChannel(H2ClientConnection* toHttp2)
    : H2Channel(toHttp2) {
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

void SingleRpcChannel::sendThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  VLOG(4) << "sendThriftResponse:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (responseHandler_) {
    HTTPMessage msg;
    msg.setStatusCode(200);
    if (auto otherMetadata = metadata.otherMetadata_ref()) {
      encodeHeaders(std::move(*otherMetadata), msg);
    }
    responseHandler_->sendHeaders(msg);
    responseHandler_->sendBody(std::move(payload));
    responseHandler_->sendEOM();
  }
  receivedThriftRPC_ = true;
}

void SingleRpcChannel::sendThriftRequest(
    RequestRpcMetadata&& metadata,
    std::unique_ptr<IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(metadata.kind_ref());
  DCHECK(
      metadata.kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
      metadata.kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE);
  DCHECK(payload);
  DCHECK(callback);
  VLOG(4) << "sendThriftRequest:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  auto callbackEvb = callback->getEventBase();
  try {
    httpTransaction_ = h2ClientConnection_->newTransaction(this);
  } catch (TTransportException& te) {
    callbackEvb->runInEventBaseThread([evbCallback = std::move(callback),
                                       evbTe = std::move(te)]() mutable {
      evbCallback->onError(
          folly::make_exception_wrapper<TTransportException>(std::move(evbTe)));
    });
    return;
  }
  HTTPMessage msg;
  msg.setMethod(HTTPMethod::POST);
  msg.setURL(metadata.url);
  auto& msgHeaders = msg.getHeaders();
  msgHeaders.set(HTTPHeaderCode::HTTP_HEADER_HOST, metadata.host);
  msgHeaders.set(HTTPHeaderCode::HTTP_HEADER_USER_AGENT, "C++/THttpClient");
  msgHeaders.set(
      HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, "application/x-thrift");
  if (auto clientTimeoutMs = metadata.clientTimeoutMs_ref()) {
    DCHECK(*clientTimeoutMs > 0);
    httpTransaction_->setIdleTimeout(
        std::chrono::milliseconds(*clientTimeoutMs));
  }

  if (auto clientTimeoutMs = metadata.clientTimeoutMs_ref()) {
    metadata.otherMetadata[transport::THeader::CLIENT_TIMEOUT_HEADER] =
        folly::to<string>(*clientTimeoutMs);
  }
  if (auto queueTimeoutMs = metadata.queueTimeoutMs_ref()) {
    DCHECK(*queueTimeoutMs > 0);
    metadata.otherMetadata[transport::THeader::QUEUE_TIMEOUT_HEADER] =
        folly::to<string>(*queueTimeoutMs);
  }
  if (auto priority = metadata.priority_ref()) {
    metadata.otherMetadata[transport::THeader::PRIORITY_HEADER] =
        folly::to<string>(*priority);
  }
  if (auto kind = metadata.kind_ref()) {
    metadata.otherMetadata[RPC_KIND.str()] = folly::to<string>(*kind);
  }
  encodeHeaders(std::move(metadata.otherMetadata), msg);
  httpTransaction_->sendHeaders(msg);

  httpTransaction_->sendBody(std::move(payload));
  httpTransaction_->sendEOM();
  // For oneway calls, we move "callback" to "callbackEvb" since we do
  // not require the callback any more.  For twoway calls, we move
  // "callback" to "callback_" and use a raw pointer to call
  // "onThriftRequestSent()".  This is safe because "callback_" will
  // be eventually moved to this same thread to call either
  // "onThriftResponse()" or "onThriftError()".
  if (metadata.kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    callbackEvb->runInEventBaseThread(
        [cb = std::move(callback)]() mutable { cb->onThriftRequestSent(); });
  } else {
    callback_ = std::move(callback);
    callbackEvb->runInEventBaseThread(
        [cb = callback_.get()]() mutable { cb->onThriftRequestSent(); });
  }
  receivedThriftRPC_ = true;
}

EventBase* SingleRpcChannel::getEventBase() noexcept {
  return evb_;
}

void SingleRpcChannel::onH2StreamBegin(
    std::unique_ptr<HTTPMessage> headers) noexcept {
  VLOG(4) << "onH2StreamBegin";
  VLOG_IF(4, headers->isResponse())
      << "onH2StreamBegin: " << headers->getStatusCode() << " "
      << headers->getStatusMessage();
  headers_ = std::move(headers);
}

void SingleRpcChannel::onH2BodyFrame(std::unique_ptr<IOBuf> contents) noexcept {
  VLOG(4) << "onH2BodyFrame: " << std::endl
          << IOBufPrinter::printHexFolly(contents.get(), true);
  if (contents_) {
    contents_->prependChain(std::move(contents));
  } else {
    contents_ = std::move(contents);
  }
}

void SingleRpcChannel::onH2StreamEnd() noexcept {
  VLOG(4) << "onH2StreamEnd";
  receivedH2Stream_ = true;
  if (processor_) {
    // Server side
    onThriftRequest();
  } else {
    // Client side
    onThriftResponse();
  }
}

void SingleRpcChannel::onH2StreamClosed(ProxygenError error) noexcept {
  VLOG(4) << "onH2StreamClosed";
  if (callback_) {
    std::unique_ptr<TTransportException> ex;
    if (error == ProxygenError::kErrorTimeout) {
      ex =
          std::make_unique<TTransportException>(TTransportException::TIMED_OUT);
    } else {
      // Some unknown error.
      ex = std::make_unique<TTransportException>(
          TTransportException::NETWORK_ERROR,
          folly::to<std::string>(
              "ProxygenError ", proxygen::getErrorString(error)));
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
  H2Channel::onH2StreamClosed(error);
}

void SingleRpcChannel::onThriftRequest() noexcept {
  if (!contents_) {
    sendThriftErrorResponse("Proxygen stream has no body");
    return;
  }
  RequestRpcMetadata metadata;
  if (!EnvelopeUtil::stripEnvelope(&metadata, contents_)) {
    sendThriftErrorResponse("Invalid envelope: see logs for error");
    return;
  }
  // Default Single Request Single Response
  metadata.kind_ref() = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  extractHeaderInfo(&metadata);

  DCHECK(metadata.protocol_ref());
  DCHECK(metadata.name_ref());
  DCHECK(metadata.kind_ref());
  if (*metadata.kind_ref() == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    // Send a dummy response for the oneway call since we need to do
    // this with HTTP2.
    ResponseRpcMetadata responseMetadata;
    auto payload = IOBuf::createCombined(0);
    sendThriftResponse(std::move(responseMetadata), std::move(payload));
    receivedThriftRPC_ = true;
  }
  metadata.seqId_ref() = 0;
  auto connContext =
      std::make_unique<Cpp2ConnContext>(&headers_->getClientAddress());
  processor_->onThriftRequest(
      std::move(metadata),
      std::move(contents_),
      shared_from_this(),
      std::move(connContext));
}

void SingleRpcChannel::onThriftResponse() noexcept {
  DCHECK(httpTransaction_);
  if (!callback_) {
    return;
  }

  auto statusCode = headers_->getStatusCode();
  if (statusCode != 100 && statusCode != 200) {
    auto evb = callback_->getEventBase();
    auto exWrapper = folly::make_exception_wrapper<TTransportException>(
        TTransportException::UNKNOWN,
        folly::to<std::string>(
            "Bad status: ", statusCode, " ", headers_->getStatusMessage()));
    evb->runInEventBaseThread([evbCallback = std::move(callback_),
                               exw = std::move(exWrapper)]() mutable {
      evbCallback->onError(std::move(exw));
    });
    return;
  }

  // TODO: contents_ should never be empty. For some reason, once the number
  // of queries starts going past 1.5MQPS, this contents_ will not get set.
  // For now, just drop if contents_ is null. However, this should be
  // investigated further to figure out why contents_ is not being set.
  if (!contents_) {
    VLOG(2) << "Contents has not been set.";
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([evbCallback = std::move(callback_)]() mutable {
      evbCallback->onError(folly::make_exception_wrapper<TTransportException>(
          TTransportException::END_OF_FILE, "No content"));
    });
    return;
  }
  auto evb = callback_->getEventBase();
  ResponseRpcMetadata metadata;
  map<string, string> headers;
  decodeHeaders(*headers_, headers);
  if (!headers.empty()) {
    metadata.otherMetadata_ref() = std::move(headers);
  }

  // We don't need to set any of the other fields in metadata currently.
  evb->runInEventBaseThread([evbCallback = std::move(callback_),
                             evbMetadata = std::move(metadata),
                             evbContents = std::move(contents_)]() mutable {
    evbCallback->onThriftResponse(
        std::move(evbMetadata), std::move(evbContents));
  });
}

void SingleRpcChannel::extractHeaderInfo(
    RequestRpcMetadata* metadata) noexcept {
  map<string, string> headers;
  decodeHeaders(*headers_, headers);
  auto iter = headers.find(transport::THeader::CLIENT_TIMEOUT_HEADER);
  if (iter != headers.end()) {
    try {
      metadata->clientTimeoutMs_ref() = folly::to<int64_t>(iter->second);
    } catch (const std::range_error&) {
      LOG(INFO) << "Bad client timeout " << iter->second;
    }
    headers.erase(iter);
  }
  iter = headers.find(transport::THeader::QUEUE_TIMEOUT_HEADER);
  if (iter != headers.end()) {
    try {
      metadata->queueTimeoutMs_ref() = folly::to<int64_t>(iter->second);
    } catch (const std::range_error&) {
      LOG(INFO) << "Bad client timeout " << iter->second;
    }
    headers.erase(iter);
  }
  iter = headers.find(transport::THeader::PRIORITY_HEADER);
  if (iter != headers.end()) {
    try {
      auto pr = static_cast<RpcPriority>(folly::to<int32_t>(iter->second));
      if (pr < RpcPriority::N_PRIORITIES) {
        metadata->priority_ref() = pr;
      } else {
        LOG(INFO) << "Too large value for method priority " << iter->second;
      }
    } catch (const std::range_error&) {
      LOG(INFO) << "Bad method priority " << iter->second;
    }
    headers.erase(iter);
  }
  iter = headers.find(RPC_KIND.str());
  if (iter != headers.end()) {
    try {
      metadata->kind_ref() =
          static_cast<RpcKind>(folly::to<int32_t>(iter->second));
    } catch (const std::range_error&) {
      LOG(INFO) << "Bad Request Kind " << iter->second;
    }
    headers.erase(iter);
  }
  if (!headers.empty()) {
    metadata->otherMetadata_ref() = std::move(headers);
  }
}

void SingleRpcChannel::sendThriftErrorResponse(
    const string& message,
    ProtocolId protoId,
    const string& name) noexcept {
  ResponseRpcMetadata responseMetadata;
  responseMetadata.protocol_ref() = protoId;
  // Not setting the "ex" header since these errors do not fit into any
  // of the existing error categories.
  TApplicationException tae(message);
  std::unique_ptr<IOBuf> payload;
  auto proto = static_cast<int16_t>(protoId);
  try {
    payload = serializeError(proto, tae, name, 0);
  } catch (const TProtocolException& pe) {
    // Should never happen.  Log an error and return an empty
    // payload.
    LOG(ERROR) << "serializeError failed. type=" << pe.getType()
               << " what()=" << pe.what();
    payload = IOBuf::createCombined(0);
  }
  sendThriftResponse(std::move(responseMetadata), std::move(payload));
  receivedThriftRPC_ = true;
}

} // namespace thrift
} // namespace apache
