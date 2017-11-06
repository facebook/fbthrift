/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/HTTPClientChannel.h>

#include <utility>

#include <folly/Format.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/http/codec/HTTP1xCodec.h>
#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <proxygen/lib/utils/Base64.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <wangle/ssl/SSLContextConfig.h>

using folly::IOBuf;
using folly::IOBufQueue;
using folly::RequestContext;
using std::make_unique;
using std::pair;
using std::unique_ptr;
using namespace apache::thrift::transport;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::transport::THeader;
using folly::EventBase;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;
using apache::thrift::transport::TTransportException;
using proxygen::WheelTimerInstance;

namespace apache {
namespace thrift {

const std::chrono::milliseconds HTTPClientChannel::kDefaultTransactionTimeout =
    std::chrono::milliseconds(500);

HTTPClientChannel::Ptr HTTPClientChannel::newHTTP1xChannel(
    async::TAsyncTransport::UniquePtr transport,
    const std::string& httpHost,
    const std::string& httpUrl) {
  HTTPClientChannel::Ptr channel(new HTTPClientChannel(
      std::move(transport),
      std::make_unique<proxygen::HTTP1xCodec>(
          proxygen::TransportDirection::UPSTREAM)));
  channel->setHTTPHost(httpHost);
  channel->setHTTPUrl(httpUrl);
  return channel;
}

HTTPClientChannel::Ptr HTTPClientChannel::newHTTP2Channel(
    async::TAsyncTransport::UniquePtr transport) {
  return HTTPClientChannel::Ptr(new HTTPClientChannel(
      std::move(transport),
      std::make_unique<proxygen::HTTP2Codec>(
          proxygen::TransportDirection::UPSTREAM)));
}

HTTPClientChannel::HTTPClientChannel(
    async::TAsyncTransport::UniquePtr transport,
    std::unique_ptr<proxygen::HTTPCodec> codec)
    : evb_(transport->getEventBase()) {
  auto localAddress = transport->getLocalAddress();
  auto peerAddress = transport->getPeerAddress();

  httpSession_ = new proxygen::HTTPUpstreamSession(
      WheelTimerInstance(timeout_, evb_),
      std::move(transport),
      localAddress,
      peerAddress,
      std::move(codec),
      wangle::TransportInfo(),
      this);
}

HTTPClientChannel::~HTTPClientChannel() {
  closeNow();
}

void HTTPClientChannel::setMaxPendingRequests(uint32_t num) {
  if (httpSession_) {
    httpSession_->setMaxConcurrentOutgoingStreams(num);
  }
}

// apache::thrift::ClientChannel methods

bool HTTPClientChannel::good() {
  auto transport = httpSession_ ? httpSession_->getTransport() : nullptr;
  return transport && transport->good();
}

ClientChannel::SaturationStatus HTTPClientChannel::getSaturationStatus() {
  if (httpSession_) {
    return SaturationStatus(
        httpSession_->getNumOutgoingStreams(),
        httpSession_->getMaxConcurrentOutgoingStreams());
  } else {
    return SaturationStatus();
  }
}

void HTTPClientChannel::closeNow() {
  if (httpSession_) {
    httpSession_->dropConnection();
    httpSession_ = nullptr;
  }
}

void HTTPClientChannel::attachEventBase(EventBase* eventBase) {
  assert(eventBase->isInEventBaseThread());
  if (httpSession_) {
    httpSession_->attachThreadLocals(
        eventBase,
        nullptr,
        WheelTimerInstance(timeout_, eventBase),
        nullptr,
        [](proxygen::HTTPCodecFilter*) {},
        nullptr,
        nullptr);
  }
  evb_ = eventBase;
}

void HTTPClientChannel::detachEventBase() {
  assert(evb_->isInEventBaseThread());
  assert(isDetachable());
  if (httpSession_) {
    httpSession_->detachThreadLocals();
  }
  evb_ = nullptr;
}

bool HTTPClientChannel::isDetachable() {
  return !httpSession_ || httpSession_->isDetachable();
}

// end apache::thrift::ClientChannel methods

// folly::DelayedDestruction methods

void HTTPClientChannel::destroy() {
  closeNow();
  folly::DelayedDestruction::destroy();
}

// end folly::DelayedDestruction methods

// apache::thrift::RequestChannel methods

uint32_t HTTPClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  sendRequest_(
      rpcOptions,
      true,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t HTTPClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  return sendRequest_(
      rpcOptions,
      false,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
}

uint32_t HTTPClientChannel::sendRequest_(
    RpcOptions& rpcOptions,
    bool oneway,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  DestructorGuard dg(this);

  cb->context_ = RequestContext::saveContext();

  std::chrono::milliseconds timeout(timeout_);
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    timeout = rpcOptions.getTimeout();
  }

  // Do not try to keep the raw pointer of this out of this function,
  // it is a self-destruct-object, it can dangle at some time later,
  // instead, only react to its callback
  auto httpCallback = new HTTPTransactionCallback(
      oneway, std::move(cb), std::move(ctx), isSecurityActive(), protocolId_);

  if (!httpSession_) {
    TTransportException ex(
        TTransportException::NOT_OPEN, "HTTPSession is not open");
    httpCallback->messageSendError(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)));
    delete httpCallback;
    return -1;
  }

  auto txn = httpSession_->newTransaction(httpCallback);

  if (!txn) {
    TTransportException ex(
        TTransportException::NOT_OPEN,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    httpCallback->messageSendError(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)));
    delete httpCallback;
    return -1;
  }

  if (timeout.count()) {
    txn->setIdleTimeout(timeout);
  }
  auto streamId = txn->getID();

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  auto msg = buildHTTPMessage(header.get());

  txn->sendHeaders(msg);
  txn->sendBody(std::move(buf));
  txn->sendEOM();

  return (uint32_t)streamId;
}

// end apache::thrift::RequestChannel methods

// HTTPSession::InfoCallback methods

void HTTPClientChannel::onDestroy(const proxygen::HTTPSessionBase&) {
  if (closeCallback_) {
    closeCallback_->channelClosed();
  }
  httpSession_ = nullptr;
  closeCallback_ = nullptr;
}

// end HTTPSession::InfoCallback methods

void HTTPClientChannel::setRequestHeaderOptions(THeader* header) {
  header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
  header->forceClientType(THRIFT_HTTP_CLIENT_TYPE);
}

void HTTPClientChannel::setHeaders(
    proxygen::HTTPHeaders& dstHeaders,
    const transport::THeader::StringToStringMap& srcHeaders) {
  for (const auto& header : srcHeaders) {
    if (header.first.find(":") != std::string::npos) {
      auto name = proxygen::Base64::urlEncode(folly::StringPiece(header.first));
      auto value =
          proxygen::Base64::urlEncode(folly::StringPiece(header.second));
      dstHeaders.rawSet(
          folly::to<std::string>("encode_", name),
          folly::to<std::string>(name, "_", value));
    } else {
      dstHeaders.rawSet(header.first, header.second);
    }
  }
}

proxygen::HTTPMessage HTTPClientChannel::buildHTTPMessage(THeader* header) {
  proxygen::HTTPMessage msg;
  msg.setMethod(proxygen::HTTPMethod::POST);
  msg.setURL(httpUrl_);
  msg.setHTTPVersion(1, 1);
  msg.setIsChunked(false);
  auto& headers = msg.getHeaders();

  {
    auto pwh = getPersistentWriteHeaders();
    setHeaders(headers, pwh);
    // We do not clear the persistent write headers, since http does not
    // distinguish persistent/per request headers
    // pwh.clear();
  }

  {
    auto wh = header->releaseWriteHeaders();
    setHeaders(headers, wh);
  }

  {
    auto eh = header->getExtraWriteHeaders();
    if (eh) {
      setHeaders(headers, *eh);
    }
  }

  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_HOST, httpHost_);
  headers.set(
      proxygen::HTTPHeaderCode::HTTP_HEADER_USER_AGENT, "C++/THttpClient");
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

  return msg;
}

void HTTPClientChannel::setFlowControl(
    size_t initialReceiveWindow,
    size_t receiveStreamWindowSize,
    size_t receiveSessionWindowSize) {
  if (httpSession_) {
    httpSession_->setFlowControl(
        initialReceiveWindow,
        receiveStreamWindowSize,
        receiveSessionWindowSize);
  }
}

// HTTPTransactionCallback methods

HTTPClientChannel::HTTPTransactionCallback::HTTPTransactionCallback(
    bool oneway,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    bool isSecurityActive,
    uint16_t protoId)
    : oneway_(oneway),
      cb_(std::move(cb)),
      ctx_(std::move(ctx)),
      isSecurityActive_(isSecurityActive),
      protoId_(protoId),
      txn_(nullptr) {}

HTTPClientChannel::HTTPTransactionCallback::~HTTPTransactionCallback() {
  if (txn_) {
    txn_->setHandler(nullptr);
    txn_->setTransportCallback(nullptr);
  }
}

// MessageChannel::SendCallback methods

void HTTPClientChannel::HTTPTransactionCallback::messageSent() {
  if (cb_) {
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestSent();
  }
}

void HTTPClientChannel::HTTPTransactionCallback::messageSendError(
    folly::exception_wrapper&& ex) {
  requestError(std::move(ex));
}

void HTTPClientChannel::HTTPTransactionCallback::requestError(
    folly::exception_wrapper ex) {
  if (cb_) {
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestError(
        ClientReceiveState(std::move(ex), std::move(ctx_), isSecurityActive_));
    cb_ = nullptr;
  }
}

// end MessageChannel::SendCallback methods

// proxygen::HTTPTransactionHandler methods

void HTTPClientChannel::HTTPTransactionCallback::setTransaction(
    proxygen::HTTPTransaction* txn) noexcept {
  txn_ = txn;
  // If Transaction is created through HTTPSession::newTransaction,
  // handler is already set, thus no need for txn_->setHandler(this);
  txn_->setTransportCallback(this);
}

void HTTPClientChannel::HTTPTransactionCallback::detachTransaction() noexcept {
  VLOG(5) << "HTTPTransaction on memory " << this << " is detached.";
  delete this;
}

void HTTPClientChannel::HTTPTransactionCallback::onHeadersComplete(
    std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
  msg_ = std::move(msg);
}

void HTTPClientChannel::HTTPTransactionCallback::onBody(
    std::unique_ptr<folly::IOBuf> body) noexcept {
  if (!body_) {
    body_ = std::make_unique<folly::IOBufQueue>();
  }
  body_->append(std::move(body));
}

void HTTPClientChannel::HTTPTransactionCallback::onTrailers(
    std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {
  trailers_ = std::move(trailers);
}

void HTTPClientChannel::HTTPTransactionCallback::onEOM() noexcept {
  if (!oneway_ && cb_) {
    if (!body_) {
      requestError(folly::make_exception_wrapper<
                   transport::TTransportException>(folly::sformat(
          "Empty HTTP response, {}",
          (msg_ ? folly::to<std::string>(
                      msg_->getStatusCode(), ", ", msg_->getStatusMessage())
                : "Empty Header"))));
      return;
    }

    auto header = std::make_unique<transport::THeader>();
    header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
    apache::thrift::transport::THeader::StringToStringMap readHeaders;
    msg_->getHeaders().forEach(
        [&readHeaders](const std::string& key, const std::string& val) {
          readHeaders[key] = val;
        });
    header->setReadHeaders(std::move(readHeaders));
    folly::RequestContextScopeGuard rctx(cb_->context_);
    auto body = body_->move();
    body_.reset();
    cb_->replyReceived(ClientReceiveState(
        protoId_,
        std::move(body),
        std::move(header),
        std::move(ctx_),
        isSecurityActive_,
        true));
    cb_ = nullptr;
  }
}

void HTTPClientChannel::HTTPTransactionCallback::onError(
    const proxygen::HTTPException& error) noexcept {
  if (!oneway_) {
    if (error.getProxygenError() == proxygen::ProxygenError::kErrorTimeout) {
      TTransportException ex(TTransportException::TIMED_OUT, "Timed Out");
      ex.setOptions(TTransportException::CHANNEL_IS_VALID);
      requestError(
          folly::make_exception_wrapper<TTransportException>(std::move(ex)));
    } else {
      requestError(
          folly::make_exception_wrapper<transport::TTransportException>(
              error.what()));
    }
  }
}

// end proxygen::HTTPTransactionHandler methods

// proxygen::HTTPTransaction::TransportCallback methods

void HTTPClientChannel::HTTPTransactionCallback::lastByteFlushed() noexcept {
  if (cb_) {
    folly::RequestContextScopeGuard rctx(cb_->context_);
    cb_->requestSent();
  }
}

// end proxygen::HTTPTransaction::TransportCallback methods

} // namespace thrift
} // namespace apache
