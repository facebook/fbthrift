/*
 * Copyright 2014 Facebook, Inc.
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

#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/ResponseChannel.h"
#include "thrift/lib/cpp2/async/GssSaslClient.h"
#include "thrift/lib/cpp/EventHandlerBase.h"
#include "thrift/lib/cpp/transport/TTransportException.h"
#include "folly/io/Cursor.h"

#include <utility>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using namespace apache::thrift::transport;
using apache::thrift::async::TEventBase;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::async::RequestContext;

namespace apache { namespace thrift {

HeaderClientChannel::HeaderClientChannel(
  const std::shared_ptr<TAsyncTransport>& transport)
    : Cpp2Channel(transport)
    , sendSeqId_(0)
    , closeCallback_(nullptr)
    , timeout_(0)
    , timeoutSASL_(500)
    , handshakeMessagesSent_(0)
    , keepRegisteredForClose_(true)
    , timer_(new apache::thrift::async::HHWheelTimer(getEventBase()))
    , saslClientCallback_(*this) {
  header_.reset(new THeader);
  header_->setSupportedClients(nullptr);
  header_->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
}

void HeaderClientChannel::setTimeout(uint32_t ms) {
  transport_->setSendTimeout(ms);
  timeout_ = ms;
}

void HeaderClientChannel::setSaslTimeout(uint32_t ms) {
  timeoutSASL_ = ms;
}

void HeaderClientChannel::destroy() {
  saslClientCallback_.cancelTimeout();
  if (saslClient_) {
    saslClient_->markChannelCallbackUnavailable();
  }
  Cpp2Channel::destroy();
}

void HeaderClientChannel::attachEventBase(
    TEventBase* eventBase) {
  Cpp2Channel::attachEventBase(eventBase);
  timer_->attachEventBase(eventBase);
}

void HeaderClientChannel::detachEventBase() {
  saslClientCallback_.cancelTimeout();
  if (saslClient_) {
    saslClient_->markChannelCallbackUnavailable();
  }

  Cpp2Channel::detachEventBase();
  timer_->detachEventBase();
}

void HeaderClientChannel::startSecurity() {
  if (protectionState_ != ProtectionState::UNKNOWN) {
    return;
  }

  // It might be possible to short-circuit this to happen earlier,
  // but since it's internal state, it's not clear there's any
  // value.
  if (header_->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    protectionState_ = ProtectionState::NONE;
    return;
  }

  if (!saslClient_) {
    throw TTransportException("Security requested, but SASL client not set");
  }

  // Let's get this party started.
  protectionState_ = ProtectionState::INPROGRESS;
  setBaseReceivedCallback();
  // Schedule timeout because in saslClient_->start() we may be talking
  // to the KDC.
  if (timeoutSASL_ > 0) {
    timer_->scheduleTimeout(&saslClientCallback_,
                            std::chrono::milliseconds(timeoutSASL_));
  }
  saslClient_->start(&saslClientCallback_);
}

unique_ptr<IOBuf> HeaderClientChannel::handleSecurityMessage(
    unique_ptr<IOBuf>&& buf) {
  if (header_->getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (protectionState_ == ProtectionState::INPROGRESS) {
      saslClient_->consumeFromServer(&saslClientCallback_, std::move(buf));
      return nullptr;
    }
    // else, fall through to application message processing
  } else if (protectionState_ == ProtectionState::INPROGRESS ||
             protectionState_ == ProtectionState::VALID) {
    protectionState_ = ProtectionState::INVALID;
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

void HeaderClientChannel::SaslClientCallback::saslSendServer(
    std::unique_ptr<folly::IOBuf>&& message) {
  if (channel_.timeoutSASL_ > 0) {
    channel_.timer_->scheduleTimeout(this,
        std::chrono::milliseconds(channel_.timeoutSASL_));
  }
  channel_.handshakeMessagesSent_++;
  channel_.sendMessage(nullptr, std::move(message));
}

void HeaderClientChannel::SaslClientCallback::saslError(
    folly::exception_wrapper&& ex) {
  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
  auto logger = channel_.saslClient_->getSaslLogger();

  // Record error string
  std::string errorMessage =
    "MsgNum: " + std::to_string(channel_.handshakeMessagesSent_);
  channel_.saslClient_->setErrorString(errorMessage + " " + ex->what());

  if (logger) {
    logger->log("sasl_error", ex->what());
  }

  auto ew = folly::try_and_catch<std::exception>([&]() {
    // Fall back to insecure.  This will throw an exception if the
    // insecure client type is not supported.
    channel_.header_->setClientType(THRIFT_HEADER_CLIENT_TYPE);
  });
  if (ew) {
    LOG(ERROR) << "SASL required by client but failed or rejected by server";
    if (logger) {
      logger->log("sasl_failed_hard");
    }
    channel_.messageReceiveErrorWrapped(std::move(ex));
    channel_.closeNow();
    return;
  }

  VLOG(5) << "SASL client falling back to insecure: " << ex->what();
  if (logger) {
    logger->log("sasl_fell_back_to_insecure");
  }
  // We need to tell saslClient that the security channel is no longer
  // available, so that it does not attempt to send messages to the server.
  channel_.saslClient_->markChannelCallbackUnavailable();
  channel_.setSecurityComplete(ProtectionState::NONE);
}

void HeaderClientChannel::SaslClientCallback::saslComplete() {
  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
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

  switch (protectionState_) {
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
  }

  CHECK(false);
}

void HeaderClientChannel::setSecurityComplete(ProtectionState state) {
  assert(state == ProtectionState::NONE ||
         state == ProtectionState::VALID);

  protectionState_ = state;
  setBaseReceivedCallback();

  if (state == ProtectionState::VALID) {
    setSaslEndpoint(saslClient_.get());
  }

  // Replay any pending requests
  for (auto&& funcarg : afterSecurity_) {
    (this->*(std::get<0>(funcarg)))(std::get<1>(funcarg),
                                    std::move(std::get<2>(funcarg)),
                                    std::move(std::get<3>(funcarg)),
                                    std::move(std::get<4>(funcarg)));
  }
  afterSecurity_.clear();
}

void HeaderClientChannel::maybeSetPriorityHeader(const RpcOptions& rpcOptions) {
  if (!clientSupportHeader()) {
    return;
  }
  // continue only for header
  if (rpcOptions.getPriority() !=
      apache::thrift::concurrency::N_PRIORITIES) {
    header_->setCallPriority(rpcOptions.getPriority());
  }
}

void HeaderClientChannel::maybeSetTimeoutHeader(const RpcOptions& rpcOptions) {
  if (!clientSupportHeader()) {
    return;
  }
  // continue only for header
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    header_->setClientTimeout(rpcOptions.getTimeout());
  }
}

bool HeaderClientChannel::clientSupportHeader() {
  return header_->getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
         header_->getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE;
}

// Client Interface
void HeaderClientChannel::sendOnewayRequest(
    const RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf) {
  if (isSecurityPending()) {
    afterSecurity_.push_back(
      std::make_tuple(static_cast<AfterSecurityMethod>(
                        &HeaderClientChannel::sendOnewayRequest),
                      RpcOptions(rpcOptions),
                      std::move(cb),
                      std::move(ctx),
                      std::move(buf)));
    return;
  }

  maybeSetPriorityHeader(rpcOptions);
  maybeSetTimeoutHeader(rpcOptions);
  // Both cb and buf are allowed to be null.

  uint32_t oldSeqId = sendSeqId_;
  sendSeqId_ = ResponseChannel::ONEWAY_REQUEST_ID;

  if (cb) {
    sendMessage(new OnewayCallback(std::move(cb), std::move(ctx),
                                   isSecurityActive()),
                std::move(buf));
  } else {
    sendMessage(nullptr, std::move(buf));
  }
  sendSeqId_ = oldSeqId;
}

void HeaderClientChannel::setCloseCallback(CloseCallback* cb) {
  closeCallback_ = cb;
  setBaseReceivedCallback();
}

void HeaderClientChannel::sendRequest(
    const RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf) {
  // cb is not allowed to be null.
  DCHECK(cb);

  cb->context_ = RequestContext::saveContext();

  if (isSecurityPending()) {
    afterSecurity_.push_back(
      std::make_tuple(static_cast<AfterSecurityMethod>(
                        &HeaderClientChannel::sendRequest),
                      RpcOptions(rpcOptions),
                      std::move(cb),
                      std::move(ctx),
                      std::move(buf)));
    return;
  }

  DestructorGuard dg(this);

  // Oneway requests use a special sequence id.
  // Make sure this non-oneway request doesn't use
  // the oneway request ID.
  if (++sendSeqId_ == ResponseChannel::ONEWAY_REQUEST_ID) {
    ++sendSeqId_;
  }

  bool isStreaming = rpcOptions.isStreaming();

  std::chrono::milliseconds timeout(timeout_);
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    timeout = rpcOptions.getTimeout();
  }

  auto twcb = new TwowayCallback(this,
                                 sendSeqId_,
                                 header_->getProtocolId(),
                                 timeout,
                                 std::move(cb),
                                 std::move(ctx));

  if (timeout > std::chrono::milliseconds(0)) {
    timer_->scheduleTimeout(twcb, timeout);
  }
  maybeSetPriorityHeader(rpcOptions);
  maybeSetTimeoutHeader(rpcOptions);

  if (header_->getClientType() != THRIFT_HEADER_CLIENT_TYPE &&
      header_->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    recvCallbackOrder_.push_back(sendSeqId_);
  }
  recvCallbacks_[sendSeqId_] = twcb;
  setBaseReceivedCallback();

  if (!isStreaming) {
    sendMessage(twcb, std::move(buf));
  } else {
    sendStreamingMessage(sendSeqId_,
                         twcb,
                         std::move(buf),
                         HEADER_FLAG_STREAM_BEGIN);
  }
}

void HeaderClientChannel::sendStreamingMessage(
    uint32_t streamSequenceId,
    SendCallback* callback,
    std::unique_ptr<folly::IOBuf>&& data,
    HEADER_FLAGS streamFlag) {

  CHECK((streamFlag == HEADER_FLAG_STREAM_BEGIN) ||
        (streamFlag == HEADER_FLAG_STREAM_PIECE) ||
        (streamFlag == HEADER_FLAG_STREAM_END));

  uint32_t oldSeqId = sendSeqId_;
  sendSeqId_ = streamSequenceId;

  header_->setStreamFlag(streamFlag);
  sendMessage(callback, std::move(data));
  header_->clearStreamFlag();

  sendSeqId_ = oldSeqId;
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderClientChannel::frameMessage(unique_ptr<IOBuf>&& buf) {
  header_->setSequenceNumber(sendSeqId_);
  return header_->addHeader(std::move(buf));
}

std::unique_ptr<folly::IOBuf>
HeaderClientChannel::removeFrame(IOBufQueue* q, size_t& remaining) {
  std::unique_ptr<folly::IOBuf> buf = header_->removeHeader(q, remaining);
  if (!buf) {
    return buf;
  }
  header_->checkSupportedClient();
  return buf;
}

// Interface from MessageChannel::RecvCallback
void HeaderClientChannel::messageReceived(
    unique_ptr<IOBuf>&& buf,
    unique_ptr<MessageChannel::RecvCallback::sample>) {
  DestructorGuard dg(this);

  buf = handleSecurityMessage(std::move(buf));

  if (!buf) {
    return;
  }

  uint32_t recvSeqId;

  if (header_->getClientType() != THRIFT_HEADER_CLIENT_TYPE &&
      header_->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    // Non-header clients will always be in order.
    // Note that for non-header clients, getSequenceNumber()
    // will return garbage.
    recvSeqId = recvCallbackOrder_.front();
    recvCallbackOrder_.pop_front();
  } else {
    // The header contains the seq-id.  May be out of order.
    recvSeqId = header_->getSequenceNumber();
  }

  uint16_t streamFlag = header_->getStreamFlag();
  header_->clearStreamFlag();

  if ((streamFlag == HEADER_FLAG_STREAM_PIECE) ||
      (streamFlag == HEADER_FLAG_STREAM_END)) {

    auto ite = streamCallbacks_.find(recvSeqId);

    // TODO: On some errors, some servers will return 0 for seqid.
    // Could possibly try and deserialize the buf and throw a
    // TApplicationException.
    // BUT, we don't even know for sure what protocol to deserialize with.
    if (ite == streamCallbacks_.end()) {
      VLOG(5) << "Could not find message id in streamCallbacks, "
              << "possibly because we have errored out or timed out.";
      return;
    }

    auto streamCallback = ite->second;
    CHECK_NOTNULL(streamCallback);

    streamCallback->replyReceived(std::move(buf));
    return;
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

  recvCallbacks_.erase(recvSeqId);

  // we are the last callback?
  setBaseReceivedCallback();

  bool serverExpectsStreaming = (streamFlag == HEADER_FLAG_STREAM_BEGIN);
  f->replyReceived(std::move(buf), serverExpectsStreaming);
}

void HeaderClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
  protectionState_ = ProtectionState::INVALID;
  messageReceiveErrorWrapped(folly::make_exception_wrapper<TTransportException>(
      "Channel got EOF"));
  if (closeCallback_) {
    closeCallback_->channelClosed();
    closeCallback_ = nullptr;
  }
  setBaseReceivedCallback();
}

void HeaderClientChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  DestructorGuard dg(this);

  // Clear callbacks early.  The last callback can delete the client,
  // which may cause the channel to be destroy()ed, which will call
  // messageChannelEOF(), which will reenter messageReceiveError().

  decltype(recvCallbacks_) callbacks;
  decltype(afterSecurity_) otherCallbacks;
  using std::swap;
  swap(recvCallbacks_, callbacks);
  swap(afterSecurity_, otherCallbacks);

  if (!callbacks.empty()) {
    for (auto& cb : callbacks) {
      if (cb.second) {
        cb.second->requestError(ex);
      }
    }
  }

  for (auto& funcarg : otherCallbacks) {
    auto& cb = std::get<2>(funcarg);
    auto& ctx = std::get<3>(funcarg);
    if (cb) {
      cb->requestError(
          ClientReceiveState(ex, std::move(ctx), isSecurityActive()));
    }
  }
  setBaseReceivedCallback();
}

void HeaderClientChannel::eraseCallback(uint32_t seqId, TwowayCallback* cb) {
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  CHECK(it != recvCallbacks_.end());
  CHECK(it->second == cb);
  recvCallbacks_.erase(it);

  setBaseReceivedCallback();   // was this the last callback?
}

void HeaderClientChannel::registerStream(uint32_t seqId, StreamCallback* cb) {
  CHECK_NOTNULL(cb);
  auto inserted = streamCallbacks_.insert(std::make_pair(seqId, cb));
  CHECK(inserted.second);

  setBaseReceivedCallback();
}

void HeaderClientChannel::unregisterStream(uint32_t seqId, StreamCallback* cb) {
  CHECK(getEventBase()->isInEventBaseThread());
  auto ite = streamCallbacks_.find(seqId);
  CHECK(ite != streamCallbacks_.end());
  CHECK(ite->second == cb);
  streamCallbacks_.erase(ite);

  setBaseReceivedCallback();
}

void HeaderClientChannel::setBaseReceivedCallback() {
  if (protectionState_ == ProtectionState::INPROGRESS ||
      streamCallbacks_.size() != 0 ||
      recvCallbacks_.size() != 0 ||
      (closeCallback_ && keepRegisteredForClose_)) {
    setReceiveCallback(this);
  } else {
    setReceiveCallback(nullptr);
  }
}

HeaderClientChannel::StreamCallback::StreamCallback(
    HeaderClientChannel* channel,
    uint32_t sequenceId,
    std::chrono::milliseconds timeout,
    std::unique_ptr<StreamManager>&& manager)
  : channel_(channel),
    sequenceId_(sequenceId),
    timeout_(timeout),
    manager_(std::move(manager)),
    hasOutstandingSend_(true) {
  CHECK(manager_);
  manager_->setChannelCallback(this);
  resetTimeout();
}

void HeaderClientChannel::StreamCallback::sendQueued() {
  CHECK(hasOutstandingSend_);
}

void HeaderClientChannel::StreamCallback::messageSent() {
  CHECK(hasOutstandingSend_);
  hasOutstandingSend_ = false;

  if (!manager_->isDone()) {
    manager_->notifySend();
  }

  deleteThisIfNecessary();
}

void HeaderClientChannel::StreamCallback::messageSendError(
    folly::exception_wrapper&& ex) {
  CHECK(hasOutstandingSend_);
  hasOutstandingSend_ = false;

  if (!manager_->isDone()) {
    manager_->notifyError(ex.getExceptionPtr());
  }

  deleteThisIfNecessary();
}


void HeaderClientChannel::StreamCallback::replyReceived(
    std::unique_ptr<folly::IOBuf> buf) {
  if (!manager_->isDone()) {
    manager_->notifyReceive(std::move(buf));
  }

  if (!manager_->isDone()) {
    resetTimeout();
  }

  deleteThisIfNecessary();
}

void HeaderClientChannel::StreamCallback::requestError(std::exception_ptr ex) {
  if (!manager_->isDone()) {
    manager_->notifyError(ex);
  }
  deleteThisIfNecessary();
}

void HeaderClientChannel::StreamCallback::timeoutExpired() noexcept {
  if (!manager_->isDone()) {
    TTransportException ex(TTransportException::TIMED_OUT, "Timed Out");
    manager_->notifyError(std::make_exception_ptr(ex));
  }
  deleteThisIfNecessary();
}

void HeaderClientChannel::StreamCallback::onStreamSend(
    std::unique_ptr<folly::IOBuf>&& buf) {
  CHECK(!hasOutstandingSend_);
  CHECK(!buf->empty());
  hasOutstandingSend_ = true;

  HEADER_FLAGS streamFlag = manager_->isSendingEnd() ?
                            HEADER_FLAG_STREAM_END : HEADER_FLAG_STREAM_PIECE;

  channel_->sendStreamingMessage(sequenceId_, this, std::move(buf), streamFlag);
}

void HeaderClientChannel::StreamCallback::onOutOfLoopStreamError(
    const std::exception_ptr& error) {
  deleteThisIfNecessary();
}

void HeaderClientChannel::StreamCallback::resetTimeout() {
  cancelTimeout();
  if (timeout_ > std::chrono::milliseconds(0)) {
    channel_->timer_->scheduleTimeout(this, timeout_);
  }
}

void HeaderClientChannel::StreamCallback::deleteThisIfNecessary() {
  if (manager_->isDone() && !hasOutstandingSend_) {
    delete this;
  }
}

HeaderClientChannel::StreamCallback::~StreamCallback() {
  CHECK(!hasOutstandingSend_);
  channel_->unregisterStream(sequenceId_, this);
  // any timeout will be automatically cancelled on destruction
}

}} // apache::thrift
