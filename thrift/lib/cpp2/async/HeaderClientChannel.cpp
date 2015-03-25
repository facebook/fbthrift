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
using folly::make_unique;
using namespace apache::thrift::transport;
using apache::thrift::async::TEventBase;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::async::RequestContext;

namespace apache { namespace thrift {

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
    , timer_(new apache::thrift::async::HHWheelTimer(getEventBase())) {
  header_.reset(new THeader);
  header_->setSupportedClients(nullptr);
  header_->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
}

void HeaderClientChannel::setTimeout(uint32_t ms) {
  getTransport()->setSendTimeout(ms);
  timeout_ = ms;
}

void HeaderClientChannel::setSaslTimeout(uint32_t ms) {
  timeoutSASL_ = ms;
}

void HeaderClientChannel::closeNow() {
  saslClientCallback_.cancelTimeout();
  if (saslClient_) {
    saslClient_->detachEventBase();
  }

  cpp2Channel_->closeNow();
}

void HeaderClientChannel::destroy() {
  closeNow();
  TDelayedDestruction::destroy();
}

void HeaderClientChannel::attachEventBase(
    TEventBase* eventBase) {
  cpp2Channel_->attachEventBase(eventBase);
  timer_->attachEventBase(eventBase);
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
  timer_->detachEventBase();
}

bool HeaderClientChannel::isDetachable() {
  return getTransport()->isDetachable() && timer_->isDetachable();
}

void HeaderClientChannel::startSecurity() {
  if (getProtectionState() != ProtectionState::UNKNOWN) {
    return;
  }

  // It might be possible to short-circuit this to happen earlier,
  // but since it's internal state, it's not clear there's any
  // value.
  if (header_->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
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
  userProtocolId_ = header_->getProtocolId();

  // Let's get this party started.
  setProtectionState(ProtectionState::INPROGRESS);
  setBaseReceivedCallback();
  saslClient_->setProtocolId(T_COMPACT_PROTOCOL);
  saslClient_->start(&saslClientCallback_);
}

unique_ptr<IOBuf> HeaderClientChannel::handleSecurityMessage(
    unique_ptr<IOBuf>&& buf) {
  if (header_->getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (getProtectionState() == ProtectionState::INPROGRESS) {
      saslClient_->consumeFromServer(&saslClientCallback_, std::move(buf));
      return nullptr;
    }
    // else, fall through to application message processing
  } else if (getProtectionState() == ProtectionState::INPROGRESS ||
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
    channel_.timer_->scheduleTimeout(
      this, std::chrono::milliseconds(channel_.timeoutSASL_));
  }
}

void HeaderClientChannel::SaslClientCallback::saslSendServer(
    std::unique_ptr<folly::IOBuf>&& message) {
  if (channel_.timeoutSASL_ > 0) {
    channel_.timer_->scheduleTimeout(this,
        std::chrono::milliseconds(channel_.timeoutSASL_));
  }
  channel_.handshakeMessagesSent_++;

  channel_.header_->setProtocolId(T_COMPACT_PROTOCOL);
  channel_.sendMessage(nullptr, std::move(message));
  channel_.header_->setProtocolId(channel_.userProtocolId_);
}

void HeaderClientChannel::SaslClientCallback::saslError(
    folly::exception_wrapper&& ex) {
  DestructorGuard g(&channel_);

  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
  channel_.saslClient_->detachEventBase();

  auto logger = channel_.saslClient_->getSaslLogger();

  bool ex_eof = false;
  ex.with_exception<TTransportException>([&](TTransportException& tex) {
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
    // Overall latency incurred for doing security ends here.
    logger->logEnd("security_latency");
    logger->log("sasl_error", ex.what().toStdString());
  }

  auto ew = folly::try_and_catch<std::exception>([&]() {
    // Fall back to insecure.  This will throw an exception if the
    // insecure client type is not supported.
    channel_.header_->setClientType(THRIFT_HEADER_CLIENT_TYPE);
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
  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
  channel_.saslClient_->detachEventBase();

  VLOG(5) << "SASL client negotiation complete: "
          << channel_.saslClient_->getClientIdentity() << " => "
          << channel_.saslClient_->getServerIdentity();
  auto logger = channel_.saslClient_->getSaslLogger();
  if (logger) {
    // Overall latency incurred for doing security ends here.
    logger->logEnd("security_latency");
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
  }

  CHECK(false);
}

void HeaderClientChannel::setSecurityComplete(ProtectionState state) {
  assert(state == ProtectionState::NONE ||
         state == ProtectionState::VALID);

  setProtectionState(state);
  setBaseReceivedCallback();

  // restore protocol to the one the user selected
  header_->setProtocolId(userProtocolId_);

  // Replay any pending requests
  for (auto&& funcarg : afterSecurity_) {
    header_->setHeaders(std::move(std::get<5>(funcarg)));
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
uint32_t HeaderClientChannel::sendOnewayRequest(
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
                      std::move(buf),
                      header_->releaseWriteHeaders()));
    return ResponseChannel::ONEWAY_REQUEST_ID;
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
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

void HeaderClientChannel::setCloseCallback(CloseCallback* cb) {
  closeCallback_ = cb;
  setBaseReceivedCallback();
}

uint32_t HeaderClientChannel::sendRequest(
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
                      std::move(buf),
                      header_->releaseWriteHeaders()));

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

  auto twcb = new TwowayCallback(this,
                                 sendSeqId_,
                                 header_->getProtocolId(),
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

  sendMessage(twcb, std::move(buf));
  return sendSeqId_;
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderClientChannel::ClientFramingHandler::addFrame(unique_ptr<IOBuf> buf) {
  THeader* header = channel_.getHeader();
  header->setSequenceNumber(channel_.sendSeqId_);
  return header->addHeader(std::move(buf),
      channel_.getPersistentWriteHeaders());
}

std::pair<std::unique_ptr<IOBuf>, size_t>
HeaderClientChannel::ClientFramingHandler::removeFrame(IOBufQueue* q) {
  THeader* header = channel_.getHeader();
  if (!q || !q->front() || q->front()->empty()) {
    return make_pair(std::unique_ptr<IOBuf>(), 0);
  }

  size_t remaining = 0;
  std::unique_ptr<folly::IOBuf> buf = header->removeHeader(q, remaining,
      channel_.getPersistentReadHeaders());
  if (!buf) {
    return make_pair(std::unique_ptr<folly::IOBuf>(), remaining);
  }
  header->checkSupportedClient();
  return make_pair(std::move(buf), 0);
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

  f->replyReceived(std::move(buf));
}

void HeaderClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
  setProtectionState(ProtectionState::INVALID);
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
      recvCallbacks_.size() != 0 ||
      (closeCallback_ && keepRegisteredForClose_)) {
    cpp2Channel_->setReceiveCallback(this);
  } else {
    cpp2Channel_->setReceiveCallback(nullptr);
  }
}

}} // apache::thrift
