/*
 * Copyright 2015 Facebook, Inc.
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
#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <proxygen/lib/http/codec/HTTP1xCodec.h>
#include <proxygen/lib/http/codec/TransportDirection.h>
#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <folly/io/Cursor.h>

#include <utility>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using folly::RequestContext;
using folly::make_unique;
using namespace apache::thrift::transport;
using folly::EventBase;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::transport::THeader;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;
using apache::thrift::transport::TTransportException;

namespace apache {
namespace thrift {

HTTPClientChannel::Ptr HTTPClientChannel::newHTTP1xChannel(
    apache::thrift::async::TAsyncTransport::UniquePtr transport,
    const std::string& host,
    const std::string& url) {
  auto channel = newChannel(std::move(transport),
                            host,
                            url,
                            folly::make_unique<proxygen::HTTP1xCodec>(
                                proxygen::TransportDirection::UPSTREAM));
  channel->setProtocolId(apache::thrift::protocol::T_BINARY_PROTOCOL);

  return channel;
}

HTTPClientChannel::Ptr HTTPClientChannel::newHTTP2Channel(
    apache::thrift::async::TAsyncTransport::UniquePtr transport,
    const std::string& host,
    const std::string& url) {
  return newChannel(std::move(transport),
                    host,
                    url,
                    folly::make_unique<proxygen::HTTP2Codec>(
                        proxygen::TransportDirection::UPSTREAM));
}

HTTPClientChannel::HTTPClientChannel(TAsyncTransport::UniquePtr transport,
                                     const std::string& host,
                                     const std::string& url,
                                     std::unique_ptr<proxygen::HTTPCodec> codec)
    : httpHost_(host),
      httpUrl_(url),
      closeCallback_(nullptr),
      timeout_(0),
      keepRegisteredForClose_(true),
      evb_(transport->getEventBase()),
      timer_(folly::HHWheelTimer::newTimer(
          evb_,
          std::chrono::milliseconds(folly::HHWheelTimer::DEFAULT_TICK_INTERVAL),
          folly::AsyncTimeout::InternalEnum::NORMAL,
          std::chrono::seconds(60))),
      protocolId_(apache::thrift::protocol::T_COMPACT_PROTOCOL) {
  folly::SocketAddress localAddr;
  folly::SocketAddress peerAddr;
  transport->getLocalAddress(&localAddr);
  transport->getPeerAddress(&peerAddr);

  httpSession_ = new proxygen::HTTPUpstreamSession(timer_.get(),
                                                   std::move(transport),
                                                   localAddr,
                                                   peerAddr,
                                                   std::move(codec),
                                                   wangle::TransportInfo(),
                                                   this);
  this->setFlowControl(4194304, 4194304, 4194304);
}

void HTTPClientChannel::setTimeout(uint32_t ms) { timeout_ = ms; }

void HTTPClientChannel::closeNow() {
  if (httpSession_) {
    httpSession_->setInfoCallback(nullptr);
    httpSession_->shutdownTransport();
    httpSession_ = nullptr;
    timer_.reset();
  }
}

void HTTPClientChannel::destroy() {
  closeNow();
  folly::DelayedDestruction::destroy();
}

void HTTPClientChannel::attachEventBase(EventBase* eventBase) {
  timer_->attachEventBase(eventBase);
  if (httpSession_) {
    auto trans = httpSession_->getTransport();
    if (trans) {
      trans->attachEventBase(eventBase);
    }
  }
  evb_ = eventBase;
}

void HTTPClientChannel::detachEventBase() {
  timer_->detachEventBase();
  if (httpSession_) {
    auto trans = httpSession_->getTransport();
    if (trans) {
      trans->detachEventBase();
    }
  }
  evb_ = nullptr;
}

bool HTTPClientChannel::isDetachable() {
  return timer_->isDetachable() &&
         (!httpSession_ || httpSession_->getTransport()->isDetachable());
}

void HTTPClientChannel::setCloseCallback(CloseCallback* cb) {
  closeCallback_ = cb;
}

void HTTPClientChannel::setRequestHeaderOptions(THeader* header) {
  header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
  header->forceClientType(THRIFT_HTTP_CLIENT_TYPE);
}

// Client Interface
uint32_t HTTPClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  DestructorGuard dg(this);
  cb->context_ = RequestContext::saveContext();

  HTTPTransactionOnewayCallback* owcb = nullptr;

  if (cb) {
    owcb = new HTTPTransactionOnewayCallback(
        std::move(cb), std::move(ctx), isSecurityActive());
  }

  if (!httpSession_) {
    if (owcb) {
      TTransportException ex(TTransportException::NOT_OPEN,
                             "HTTPSession is not open");
      owcb->messageSendError(
          folly::make_exception_wrapper<TTransportException>(std::move(ex)));

      delete owcb;
    }

    return -1;
  }

  auto txn = httpSession_->newTransaction(owcb);

  if (!txn) {
    if (owcb) {
      TTransportException ex(TTransportException::NOT_OPEN,
                             "Too many active requests on connection");
      owcb->messageSendError(
          folly::make_exception_wrapper<TTransportException>(std::move(ex)));

      delete owcb;
    }

    return -1;
  }

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  auto msg = buildHTTPMessage(header.get());

  txn->sendHeaders(msg);
  txn->sendBody(std::move(buf));
  txn->sendEOM();

  if (owcb) {
    owcb->sendQueued();
  }

  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t HTTPClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  // cb is not allowed to be null.
  DCHECK(cb);

  DestructorGuard dg(this);

  cb->context_ = RequestContext::saveContext();

  std::chrono::milliseconds timeout(timeout_);
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    timeout = rpcOptions.getTimeout();
  }

  auto twcb =
      new HTTPTransactionTwowayCallback(std::move(cb),
                                        std::move(ctx),
                                        isSecurityActive(),
                                        protocolId_,
                                        timer_.get(),
                                        std::chrono::milliseconds(timeout_));

  if (!httpSession_) {
    TTransportException ex(TTransportException::NOT_OPEN,
                           "HTTPSession is not open");
    twcb->messageSendError(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)));

    delete twcb;

    return -1;
  }

  auto txn = httpSession_->newTransaction(twcb);

  if (!txn) {
    TTransportException ex(TTransportException::NOT_OPEN,
                            "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    twcb->messageSendError(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)));

    delete twcb;

    return -1;
  }

  auto streamId = txn->getID();

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  auto msg = buildHTTPMessage(header.get());

  txn->sendHeaders(msg);
  txn->sendBody(std::move(buf));
  txn->sendEOM();

  twcb->sendQueued();

  return (uint32_t)streamId;
}

proxygen::HTTPMessage HTTPClientChannel::buildHTTPMessage(THeader* header) {
  proxygen::HTTPMessage msg;
  msg.setMethod(proxygen::HTTPMethod::POST);
  msg.setURL(httpUrl_);
  msg.setHTTPVersion(1, 1);
  msg.setIsChunked(false);
  auto& headers = msg.getHeaders();

  auto pwh = getPersistentWriteHeaders();

  for (auto it = pwh.begin(); it != pwh.end(); ++it) {
    headers.rawSet(it->first, it->second);
  }

  // We do not clear the persistent write headers, since http does not
  // distinguish persistent/per request headers
  // pwh.clear();

  auto wh = header->releaseWriteHeaders();

  for (auto it = wh.begin(); it != wh.end(); ++it) {
    headers.rawSet(it->first, it->second);
  }

  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_HOST, httpHost_);
  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
              "application/x-thrift");

  switch (protocolId_) {
    case protocol::T_BINARY_PROTOCOL:
      headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
                  "binary");
      break;
    case protocol::T_COMPACT_PROTOCOL:
      headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
                  "compact");
      break;
    case protocol::T_JSON_PROTOCOL:
      headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
                  "json");
      break;
    case protocol::T_SIMPLE_JSON_PROTOCOL:
      headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
                  "simplejson");
      break;
    default:
      // Do nothing
      break;
  }

  return msg;
}

void HTTPClientChannel::setFlowControl(size_t initialReceiveWindow,
                                       size_t receiveStreamWindowSize,
                                       size_t receiveSessionWindowSize) {
  if (httpSession_) {
    httpSession_->setFlowControl(initialReceiveWindow,
                                 receiveStreamWindowSize,
                                 receiveSessionWindowSize);
  }
}
}
} // apache::thrift
