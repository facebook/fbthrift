/*
 * Copyright 2014-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/GssSaslClient.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <folly/io/Cursor.h>

#include <utility>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using std::make_unique;
using namespace apache::thrift::transport;
using folly::EventBase;
using apache::thrift::async::TAsyncTransport;
using folly::RequestContext;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;

namespace apache { namespace thrift {

template class ChannelCallbacks::TwowayCallback<HeaderClientChannel>;

HeaderClientChannel::HeaderClientChannel(
  const std::shared_ptr<TAsyncTransport>& transport)
    : HeaderClientChannel(
        std::shared_ptr<Cpp2Channel>(
            Cpp2Channel::newChannel(transport,
                make_unique<ClientFramingHandler>(*this))))
{}

HeaderClientChannel::HeaderClientChannel(
  const std::shared_ptr<Cpp2Channel>& cpp2Channel)
    : sendSeqId_(0)
    , sendSecurityPendingSeqId_(0)
    , closeCallback_(nullptr)
    , timeout_(0)
    , timeoutSASL_(500)
    , handshakeMessagesSent_(0)
    , keepRegisteredForClose_(true)
    , saslClientCallback_(*this)
    , cpp2Channel_(cpp2Channel)
    , protocolId_(apache::thrift::protocol::T_COMPACT_PROTOCOL) {}

void HeaderClientChannel::setTimeout(uint32_t ms) {
  getTransport()->setSendTimeout(ms);
  timeout_ = ms;
}

void HeaderClientChannel::setSaslTimeout(uint32_t ms) {
  timeoutSASL_ = ms;
}

void HeaderClientChannel::closeNow() {
  cpp2Channel_->closeNow();
}

void HeaderClientChannel::destroy() {
  closeNow();
  saslClientCallback_.cancelTimeout();
  if (saslClient_) {
    saslClient_->detachEventBase();
  }
  folly::DelayedDestruction::destroy();
}

void HeaderClientChannel::useAsHttpClient(const std::string& host,
                                          const std::string& uri) {
  setClientType(THRIFT_HTTP_CLIENT_TYPE);
  httpClientParser_ = std::make_shared<util::THttpClientParser>(host, uri);
}

bool HeaderClientChannel::good() {
  auto transport = getTransport();
  return transport && transport->good();
}

void HeaderClientChannel::attachEventBase(
    EventBase* eventBase) {
  cpp2Channel_->attachEventBase(eventBase);
  if (saslClient_ && getProtectionState() == ProtectionState::UNKNOWN) {
    // Note that we only want to attach the event base here if
    // the handshake never started. If it started then reattaching the
    // event base would cause a lot of problems.
    // It could cause the SASL threads to invoke callbacks on the
    // channel that are unexpected.
    saslClient_->attachEventBase(eventBase);
  }
}

void HeaderClientChannel::detachEventBase() {
  saslClientCallback_.cancelTimeout();
  if (saslClient_) {
    saslClient_->detachEventBase();
  }

  cpp2Channel_->detachEventBase();
}

bool HeaderClientChannel::isDetachable() {
  return getTransport()->isDetachable() && recvCallbacks_.empty();
}

void HeaderClientChannel::startSecurity() {
  if (getProtectionState() != ProtectionState::UNKNOWN) {
    return;
  }

  // It might be possible to short-circuit this to happen earlier,
  // but since it's internal state, it's not clear there's any
  // value.
  if (getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    setProtectionState(ProtectionState::NONE);
    return;
  }

  if (!saslClient_) {
    throw TTransportException("Security requested, but SASL client not set");
  }

  // Save the protocol (binary/compact) for later and set it to compact for
  // compatibility with old servers that only accept compact security messages.
  // We'll restore it in setSecurityComplete().
  // TODO(alandau): Remove this code once all servers are upgraded to be able
  // to handle both compact and binary.
  userProtocolId_ = getProtocolId();

  // Let's get this party started.
  setProtectionState(ProtectionState::INPROGRESS);
  setBaseReceivedCallback();
  saslClient_->setProtocolId(T_COMPACT_PROTOCOL);
  saslClient_->start(&saslClientCallback_);
}

unique_ptr<IOBuf> HeaderClientChannel::handleSecurityMessage(
    unique_ptr<IOBuf>&& buf, unique_ptr<THeader>&& header) {
  if (header->getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (getProtectionState() == ProtectionState::INPROGRESS ||
        getProtectionState() == ProtectionState::WAITING) {
      setProtectionState(ProtectionState::INPROGRESS);
      saslClientCallback_.setHeader(std::move(header));
      saslClient_->consumeFromServer(&saslClientCallback_, std::move(buf));
      return nullptr;
    }
    // else, fall through to application message processing
  } else if (getProtectionState() == ProtectionState::INPROGRESS ||
             getProtectionState() == ProtectionState::WAITING ||
             getProtectionState() == ProtectionState::VALID) {
    setProtectionState(ProtectionState::INVALID);
    // If the security negotiation has completed successfully, or is
    // in progress, we expect SASL to continue to be used.  If
    // something else happens, it's either an attack or something is
    // very broken.  Fail hard.
    LOG(WARNING) << "non-SASL message received on SASL channel";
    saslClientCallback_.saslError(
        folly::make_exception_wrapper<TTransportException>(
          "non-SASL message received on SASL channel"));
    return nullptr;
  }

  return std::move(buf);
}

void HeaderClientChannel::SaslClientCallback::saslStarted() {
  if (channel_.timeoutSASL_ > 0) {
    channel_.getEventBase()->timer().scheduleTimeout(
      this, std::chrono::milliseconds(channel_.timeoutSASL_));
  }
}

void HeaderClientChannel::SaslClientCallback::saslSendServer(
    std::unique_ptr<folly::IOBuf>&& message) {
  if (channel_.timeoutSASL_ > 0) {
    channel_.getEventBase()->timer().scheduleTimeout(this,
        std::chrono::milliseconds(channel_.timeoutSASL_));
  }
  if (sendServerHook_) {
    sendServerHook_();
  }
  channel_.handshakeMessagesSent_++;

  header_->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  header_->setProtocolId(T_COMPACT_PROTOCOL);
  header_->setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
  channel_.setProtectionState(ProtectionState::WAITING);
  channel_.sendMessage(nullptr, std::move(message), header_.get());
  channel_.setProtocolId(channel_.userProtocolId_);
}

void HeaderClientChannel::SaslClientCallback::saslError(
    folly::exception_wrapper&& ex) {
  DestructorGuard g(&channel_);

  folly::HHWheelTimer::Callback::cancelTimeout();
  channel_.saslClient_->detachEventBase();

  auto logger = channel_.saslClient_->getSaslLogger();

  bool ex_eof = false;
  ex.with_exception([&](TTransportException& tex) {
      if (tex.getType() == TTransportException::END_OF_FILE) {
        ex_eof = true;
      }
    });
  if (ex_eof) {
    ex = folly::make_exception_wrapper<TTransportException>(
      folly::to<std::string>(
          ex.what(),
          " during SASL handshake (likely keytab entry error on server)"));
  }

  // Record error string
  std::string errorMessage =
    "MsgNum: " + std::to_string(channel_.handshakeMessagesSent_);
  channel_.saslClient_->setErrorString(
    folly::to<std::string>(errorMessage, " ", ex.what()));

  if (logger) {
    logger->log("sasl_error", ex.what().toStdString());
  }

  auto ew = folly::try_and_catch<std::exception>([&]() {
    // Fall back to insecure.  This will throw an exception if the
    // insecure client type is not supported.
    channel_.setClientType(THRIFT_HEADER_CLIENT_TYPE);
  });
  if (ew) {
    LOG(ERROR) << "SASL required by client but failed or rejected by server: "
               << ex.what();
    if (logger) {
      logger->log("sasl_failed_hard");
    }
    channel_.messageReceiveErrorWrapped(std::move(ex));
    channel_.cpp2Channel_->closeNow();
    return;
  }

  VLOG(5) << "SASL client falling back to insecure: " << ex.what();
  if (logger) {
    logger->log("sasl_fell_back_to_insecure");
  }
  // We need to tell saslClient that the security channel is no longer
  // available, so that it does not attempt to send messages to the server.
  channel_.setSecurityComplete(ProtectionState::NONE);
}

void HeaderClientChannel::SaslClientCallback::saslComplete() {
  folly::HHWheelTimer::Callback::cancelTimeout();
  channel_.saslClient_->detachEventBase();

  VLOG(5) << "SASL client negotiation complete: "
          << channel_.saslClient_->getClientIdentity() << " => "
          << channel_.saslClient_->getServerIdentity();
  auto logger = channel_.saslClient_->getSaslLogger();
  if (logger) {
    logger->log(
      "sasl_complete",
      {
        channel_.saslClient_->getClientIdentity(),
        channel_.saslClient_->getServerIdentity()
      }
    );
  }

  channel_.setSecurityComplete(ProtectionState::VALID);
}

bool HeaderClientChannel::isSecurityPending() {
  startSecurity();

  switch (getProtectionState()) {
    case ProtectionState::UNKNOWN: {
      return true;
    }
    case ProtectionState::NONE: {
      return false;
    }
    case ProtectionState::INPROGRESS: {
      return true;
    }
    case ProtectionState::VALID: {
      return false;
    }
    case ProtectionState::INVALID: {
      return false;
    }
    case ProtectionState::WAITING: {
      return true;
    }
  }

  CHECK(false);
}

void HeaderClientChannel::setSecurityComplete(ProtectionState state) {
  assert(state == ProtectionState::NONE ||
         state == ProtectionState::VALID);

  setProtectionState(state);
  setBaseReceivedCallback();

  // restore protocol to the one the user selected
  setProtocolId(userProtocolId_);

  // Replay any pending requests
  for (auto&& funcarg : afterSecurity_) {
    auto& cb = std::get<2>(funcarg);
    folly::RequestContextScopeGuard rctx(cb->context_);

    cb->securityEnd_ = std::chrono::duration_cast<Us>(
        HResClock::now().time_since_epoch()).count();
    (this->*(std::get<0>(funcarg)))(std::get<1>(funcarg),
                                    std::move(std::get<2>(funcarg)),
                                    std::move(std::get<3>(funcarg)),
                                    std::move(std::get<4>(funcarg)),
                                    std::move(std::get<5>(funcarg)));
  }
  afterSecurity_.clear();
}

bool HeaderClientChannel::clientSupportHeader() {
  return getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
         getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE;
}

// Client Interface
uint32_t HeaderClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  cb->context_ = RequestContext::saveContext();

  if (isSecurityPending()) {
    cb->securityStart_ = std::chrono::duration_cast<Us>(
        HResClock::now().time_since_epoch()).count();
    afterSecurity_.push_back(
      std::make_tuple(static_cast<AfterSecurityMethod>(
                        &HeaderClientChannel::sendOnewayRequest),
                      RpcOptions(rpcOptions),
                      std::move(cb),
                      std::move(ctx),
                      std::move(buf),
                      std::move(header)));
    return ResponseChannel::ONEWAY_REQUEST_ID;
  }

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  // Both cb and buf are allowed to be null.
  uint32_t oldSeqId = sendSeqId_;
  sendSeqId_ = ResponseChannel::ONEWAY_REQUEST_ID;

  if (cb) {
    sendMessage(new OnewayCallback(std::move(cb), std::move(ctx),
                                   isSecurityActive()),
                std::move(buf), header.get());
  } else {
    sendMessage(nullptr, std::move(buf), header.get());
  }
  sendSeqId_ = oldSeqId;
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

void HeaderClientChannel::setCloseCallback(CloseCallback* cb) {
  closeCallback_ = cb;
  setBaseReceivedCallback();
}

void HeaderClientChannel::setRequestHeaderOptions(THeader* header) {
  header->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  header->setClientType(getClientType());
  header->forceClientType(getForceClientType());
  header->setTransforms(getWriteTransforms());
  if (getClientType() == THRIFT_HTTP_CLIENT_TYPE) {
    header->setHttpClientParser(httpClientParser_);
  }
}

uint16_t HeaderClientChannel::getProtocolId() {
  if (getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
      getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE ||
      getClientType() == THRIFT_HTTP_CLIENT_TYPE) {
    return protocolId_;
  } else if (getClientType() == THRIFT_FRAMED_COMPACT) {
    return T_COMPACT_PROTOCOL;
  } else {
    return T_BINARY_PROTOCOL; // Assume other transports use TBinary
  }
}

uint32_t HeaderClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  // cb is not allowed to be null.
  DCHECK(cb);

  cb->context_ = RequestContext::saveContext();

  if (isSecurityPending()) {
    cb->securityStart_ = std::chrono::duration_cast<Us>(
        HResClock::now().time_since_epoch()).count();
    afterSecurity_.push_back(
      std::make_tuple(static_cast<AfterSecurityMethod>(
                        &HeaderClientChannel::sendRequest),
                      RpcOptions(rpcOptions),
                      std::move(cb),
                      std::move(ctx),
                      std::move(buf),
                      std::move(header)));

    // Security always happens at the beginning of the channel, with seq id 0.
    // Return sequence id expected to be generated when security is done.
    if (++sendSecurityPendingSeqId_ == ResponseChannel::ONEWAY_REQUEST_ID) {
      ++sendSecurityPendingSeqId_;
    }
    return sendSecurityPendingSeqId_;
  }

  DestructorGuard dg(this);

  // Oneway requests use a special sequence id.
  // Make sure this non-oneway request doesn't use
  // the oneway request ID.
  if (++sendSeqId_ == ResponseChannel::ONEWAY_REQUEST_ID) {
    ++sendSeqId_;
  }

  std::chrono::milliseconds timeout(timeout_);
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    timeout = rpcOptions.getTimeout();
  }

  auto twcb = new TwowayCallback<HeaderClientChannel>(this,
                                 sendSeqId_,
                                 getProtocolId(),
                                 std::move(cb),
                                 std::move(ctx),
                                 &getEventBase()->timer(),
                                 timeout,
                                 rpcOptions.getChunkTimeout());

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  if (getClientType() != THRIFT_HEADER_CLIENT_TYPE &&
      getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    recvCallbackOrder_.push_back(sendSeqId_);
  }
  recvCallbacks_[sendSeqId_] = twcb;
  setBaseReceivedCallback();

  sendMessage(twcb, std::move(buf), header.get());
  return sendSeqId_;
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderClientChannel::ClientFramingHandler::addFrame(unique_ptr<IOBuf> buf,
                                                    THeader* header) {
  channel_.updateClientType(header->getClientType());
  header->setSequenceNumber(channel_.sendSeqId_);
  return header->addHeader(std::move(buf),
                           channel_.getPersistentWriteHeaders());
}

std::tuple<std::unique_ptr<IOBuf>, size_t, std::unique_ptr<THeader>>
HeaderClientChannel::ClientFramingHandler::removeFrame(IOBufQueue* q) {
  std::unique_ptr<THeader> header(new THeader(THeader::ALLOW_BIG_FRAMES));
  if (!q || !q->front() || q->front()->empty()) {
    return make_tuple(std::unique_ptr<IOBuf>(), 0, nullptr);
  }

  size_t remaining = 0;
  std::unique_ptr<folly::IOBuf> buf = header->removeHeader(q, remaining,
      channel_.getPersistentReadHeaders());
  if (!buf) {
    return make_tuple(std::unique_ptr<folly::IOBuf>(), remaining, nullptr);
  }
  channel_.checkSupportedClient(header->getClientType());
  header->setMinCompressBytes(channel_.getMinCompressBytes());
  return make_tuple(std::move(buf), 0, std::move(header));
}

// Interface from MessageChannel::RecvCallback
void HeaderClientChannel::messageReceived(
    unique_ptr<IOBuf>&& buf,
    unique_ptr<THeader>&& header,
    unique_ptr<MessageChannel::RecvCallback::sample>) {
  DestructorGuard dg(this);

  buf = handleSecurityMessage(std::move(buf), std::move(header));

  if (!buf) {
    return;
  }

  uint32_t recvSeqId;

  if (header->getClientType() != THRIFT_HEADER_CLIENT_TYPE &&
      header->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (header->getClientType() == THRIFT_HTTP_CLIENT_TYPE &&
        buf->computeChainDataLength() == 0) {
      // HTTP/1.x Servers must send a response, even for oneway requests.
      // Ignore these responses.
      return;
    }
    // Non-header clients will always be in order.
    // Note that for non-header clients, getSequenceNumber()
    // will return garbage.
    recvSeqId = recvCallbackOrder_.front();
    recvCallbackOrder_.pop_front();
  } else {
    // The header contains the seq-id.  May be out of order.
    recvSeqId = header->getSequenceNumber();
  }

  auto cb = recvCallbacks_.find(recvSeqId);

  // TODO: On some errors, some servers will return 0 for seqid.
  // Could possibly try and deserialize the buf and throw a
  // TApplicationException.
  // BUT, we don't even know for sure what protocol to deserialize with.
  if (cb == recvCallbacks_.end()) {
    VLOG(5) << "Could not find message id in recvCallbacks "
            << "(timed out, possibly server is just now sending response?)";
    return;
  }

  auto f(cb->second);

  auto it = header->getHeaders().find("thrift_stream");
  bool isChunk = (it != header->getHeaders().end() && it->second == "chunk");

  if (isChunk) {
    f->partialReplyReceived(std::move(buf), std::move(header));
  } else {
    // non-stream message or end of stream
    recvCallbacks_.erase(recvSeqId);
    // we are the last callback?
    setBaseReceivedCallback();
    f->replyReceived(std::move(buf), std::move(header));
  }
}

void HeaderClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
  setProtectionState(ProtectionState::INVALID);
  messageReceiveErrorWrapped(folly::make_exception_wrapper<TTransportException>(
      TTransportException::TTransportExceptionType::END_OF_FILE,
      "Channel got EOF. Check for server hitting connection limit, "
      "server connection idle timeout, and server crashes."));
  if (closeCallback_) {
    closeCallback_->channelClosed();
    closeCallback_ = nullptr;
  }
  setBaseReceivedCallback();
}

void HeaderClientChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  DestructorGuard dg(this);

  while (!recvCallbacks_.empty()) {
    auto cb = recvCallbacks_.begin()->second;
    recvCallbacks_.erase(recvCallbacks_.begin());
    DestructorGuard dgcb(cb);
    cb->requestError(ex);
  }

  while (!afterSecurity_.empty()) {
    auto& funcarg = afterSecurity_.front();
    auto cb = std::move(std::get<2>(funcarg));
    auto ctx = std::move(std::get<3>(funcarg));
    afterSecurity_.pop_front();
    if (cb) {
      folly::RequestContextScopeGuard rctx(cb->context_);
      cb->securityEnd_ = std::chrono::duration_cast<Us>(
        HResClock::now().time_since_epoch()).count();
      cb->requestError(
          ClientReceiveState(ex, std::move(ctx), isSecurityActive()));
    }
  }

  setBaseReceivedCallback();
}

void HeaderClientChannel::eraseCallback(
    uint32_t seqId,
    TwowayCallback<HeaderClientChannel>* cb) {
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  CHECK(it != recvCallbacks_.end());
  CHECK(it->second == cb);
  recvCallbacks_.erase(it);

  setBaseReceivedCallback();   // was this the last callback?
}

bool HeaderClientChannel::expireCallback(uint32_t seqId) {
  VLOG(4) << "Expiring callback with sequence id " << seqId;
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  if (it != recvCallbacks_.end()) {
    it->second->expire();
    return true;
  }

  return false;
}

void HeaderClientChannel::setBaseReceivedCallback() {
  if (getProtectionState() == ProtectionState::INPROGRESS ||
      getProtectionState() == ProtectionState::WAITING ||
      recvCallbacks_.size() != 0 ||
      (closeCallback_ && keepRegisteredForClose_)) {
    cpp2Channel_->setReceiveCallback(this);
  } else {
    cpp2Channel_->setReceiveCallback(nullptr);
  }
}

}} // apache::thrift
