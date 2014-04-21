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

#include "thrift/lib/cpp2/async/HeaderServerChannel.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/TApplicationException.h"
#include "thrift/lib/cpp/transport/TTransportException.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"
#include "folly/io/Cursor.h"
#include "folly/String.h"

#include <utility>
#include <exception>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using namespace apache::thrift::transport;
using namespace apache::thrift;
using apache::thrift::async::TEventBase;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::TApplicationException;
using apache::thrift::server::TServerObserver;

namespace apache { namespace thrift {

std::atomic<uint32_t> HeaderServerChannel::sample_(0);

HeaderServerChannel::HeaderServerChannel(
  const std::shared_ptr<TAsyncTransport>& transport)
    : Cpp2Channel(transport)
    , callback_(nullptr)
    , arrivalSeqId_(1)
    , lastWrittenSeqId_(0)
    , sampleRate_(0)
    , timeoutSASL_(5000)
    , timer_(new apache::thrift::async::HHWheelTimer(getEventBase()))
    , saslServerCallback_(*this) {
  header_.reset(new THeader);
  header_->setSupportedClients(nullptr);
  header_->setProtocolId(0);
}

void HeaderServerChannel::destroy() {
  DestructorGuard dg(this);

  saslServerCallback_.cancelTimeout();
  if (saslServer_) {
    saslServer_->markChannelCallbackUnavailable();
  }

  if (callback_) {
    TTransportException error("Channel destroyed");
    callback_->channelClosed(std::make_exception_ptr(error));
  }

  Cpp2Channel::destroy();

  destroyStreams();
}

void HeaderServerChannel::destroyStreams() {
  auto ite = streams_.begin();
  while (ite != streams_.end()) {
    auto stream = ite->second;
    stream->onChannelDestroy();
    delete stream;

    ite = streams_.erase(ite);
  }
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderServerChannel::frameMessage(unique_ptr<IOBuf>&& buf) {
  // Note: This THeader function may throw.  However, we don't want to catch
  // it here, because this would send an empty message out on the wire.
  // Instead we have to catch it at sendMessage
  return header_->addHeader(
    std::move(buf),
    false /* Data already transformed in AsyncProcessor.h */);
}

std::unique_ptr<folly::IOBuf>
HeaderServerChannel::removeFrame(IOBufQueue* q, size_t& remaining) {
  // removeHeader will set seqid in header_.
  // For older clients with seqid in the protocol, header_
  // will dig in to the protocol to get the seqid correctly.
  std::unique_ptr<folly::IOBuf> buf;
  try {
    buf = header_->removeHeader(q, remaining);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Received invalid request from client: "
               << folly::exceptionStr(e) << " "
               << getTransportDebugString(getTransport());
    return buf;
  }
  if (!buf) {
    return buf;
  }
  if (!header_->isSupportedClient() &&
      header_->getClientType() != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    LOG(ERROR) << "Server rejecting unsupported client type "
               << header_->getClientType();
    header_->checkSupportedClient();
  }

  // In order to allow negotiation to happen when the client requests
  // sasl but it's not supported, we don't throw an exception in the
  // sasl case.  Instead, we let the message bubble up, and check if
  // the client is supported in handleSecurityMessage called from the
  // messageReceived callback.

  return buf;
}

std::string
HeaderServerChannel::getTransportDebugString(TAsyncTransport* transport) {
  if (!transport) {
    return std::string();
  }

  auto ret = folly::to<std::string>("(transport ",
                                    folly::demangle(typeid(*transport)));

  try {
    TSocketAddress addr;
    transport->getPeerAddress(&addr);
    folly::toAppend(", address ", addr.getAddressStr(),
                    ", port ", addr.getPort(),
                    &ret);
  } catch (const TTransportException& e) {
  }

  ret += ')';
  return std::move(ret);
}

// Client Interface

HeaderServerChannel::HeaderRequest::HeaderRequest(
      uint32_t seqId,
      HeaderServerChannel* channel,
      unique_ptr<IOBuf>&& buf,
      const std::map<std::string, std::string>& headers,
      const std::vector<uint16_t>& trans,
      bool outOfOrder,
      bool clientExpectsStreams,
      Stream** streamPtrReturn,
      unique_ptr<sample> sample)
  : channel_(channel)
  , seqId_(seqId)
  , headers_(headers)
  , transforms_(trans)
  , outOfOrder_(outOfOrder)
  , clientExpectsStreams_(clientExpectsStreams)
  , streamTimeout_(0)
  , streamPtrReturn_(streamPtrReturn)
  , active_(true) {

  CHECK_NOTNULL(streamPtrReturn_);

  this->buf_ = std::move(buf);
  if (sample) {
    timestamps_.readBegin = sample->readBegin;
    timestamps_.readEnd = sample->readEnd;
  }
}

/**
 * send a reply to the client.
 *
 * Note that to be backwards compatible with thrift1, the generated
 * code calls sendReply(nullptr) for oneway calls where seqid !=
 * ONEWAY_SEQ_ID.  This is so that the sendCatchupRequests code runs
 * correctly for in-order responses to older clients.  sendCatchupRequests
 * does not actually send null buffers, it just ignores them.
 *
 */
void HeaderServerChannel::HeaderRequest::sendReply(
    unique_ptr<IOBuf>&& buf,
    MessageChannel::SendCallback* cb,
    THeader::StringToStringMap&& headers) {
  if (!outOfOrder_) {
    // In order processing, make sure the ordering is correct.
    if (seqId_ != channel_->lastWrittenSeqId_ + 1) {
      // Save it until we can send it in order.
      channel_->inOrderRequests_[seqId_] =
        std::make_tuple(cb, std::move(buf), transforms_, headers);
    } else {
      // Send it now, and send any subsequent requests in order.
      channel_->sendCatchupRequests(
        std::move(buf), cb, transforms_, std::move(headers));
    }
  } else {
    if (!buf) {
      // oneway calls are OK do this, but is a bug for twoway.
      DCHECK(isOneway());
      return;
    }
    try {
      // out of order, send as soon as it is done.
      channel_->header_->setSequenceNumber(seqId_);
      channel_->header_->setTransforms(transforms_);
      channel_->header_->setHeaders(std::move(headers));
      channel_->sendMessage(cb, std::move(buf));
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to send message: " << e.what();
    }
  }
}

/**
 * Send a serialized error back to the client.
 * For a header server, this means serializing the exception, and setting
 * an error flag in the header.
 */
void HeaderServerChannel::HeaderRequest::sendError(
    std::exception_ptr ep,
    std::string exCode,
    MessageChannel::SendCallback* cb) {

  try {
    std::rethrow_exception(ep);
  } catch (const TApplicationException& ex) {
    THeader::StringToStringMap headers;
    headers["ex"] = exCode;
    std::unique_ptr<folly::IOBuf> exbuf(
      serializeError(channel_->header_->getProtocolId(), ex, getBuf()));
    sendReply(std::move(exbuf), cb, std::move(headers));
  } catch (const std::exception& ex) {
    // Other types are unimplemented.
    DCHECK(false);
  }
}

void HeaderServerChannel::HeaderRequest::sendReplyWithStreams(
    std::unique_ptr<folly::IOBuf>&& data,
    std::unique_ptr<StreamManager>&& manager,
    MessageChannel::SendCallback* sendCallback) {

  if (clientExpectsStreams_) {
    CHECK(outOfOrder_);

    Stream* stream = nullptr;
    try {
      stream = new Stream(channel_,
                          seqId_,
                          transforms_,
                          headers_,
                          streamTimeout_,
                          getBuf()->clone(),
                          std::move(manager),
                          sendCallback);

      channel_->header_->setSequenceNumber(seqId_);
      channel_->header_->setTransforms(transforms_);
      channel_->sendStreamingMessage(stream,
                                     std::move(data),
                                     HEADER_FLAG_STREAM_BEGIN);

      // since this function is never called across threads
      // streamPtrReturn_ will always remain valid
      *streamPtrReturn_ = stream;
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to send message: " << e.what();
      delete stream;
    }

  } else {
    manager->cancel();
    sendReply(std::move(data));
  }
}

void HeaderServerChannel::HeaderRequest::setStreamTimeout(
    const std::chrono::milliseconds& timeout) {
  streamTimeout_ = timeout;
}

void HeaderServerChannel::sendCatchupRequests(
    std::unique_ptr<folly::IOBuf> next_req,
    MessageChannel::SendCallback* cb,
    std::vector<uint16_t> transforms,
    THeader::StringToStringMap&& headers) {

  DestructorGuard dg(this);

  while (true) {
    if (next_req) {
      try {
        header_->setSequenceNumber(lastWrittenSeqId_ + 1);
        header_->setTransforms(transforms);
        header_->setHeaders(std::move(headers));
        sendMessage(cb, std::move(next_req));
      } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to send message: " << e.what();
      }
    } else if (nullptr != cb) {
      // There is no message (like a oneway req), but there is a callback
      cb->messageSent();
    }
    lastWrittenSeqId_++;

    // Check for the next req
    auto next = inOrderRequests_.find(
        lastWrittenSeqId_ + 1);
    if (next != inOrderRequests_.end()) {
      next_req = std::move(std::get<1>(next->second));
      cb = std::get<0>(next->second);
      transforms = std::get<2>(next->second);
      headers = std::get<3>(next->second);
      inOrderRequests_.erase(lastWrittenSeqId_ + 1);
    } else {
      break;
    }
  }
}

void HeaderServerChannel::sendStreamingMessage(
    SendCallback* callback,
    std::unique_ptr<folly::IOBuf>&& data,
    HEADER_FLAGS streamFlag) {

  CHECK((streamFlag == HEADER_FLAG_STREAM_BEGIN) ||
        (streamFlag == HEADER_FLAG_STREAM_PIECE) ||
        (streamFlag == HEADER_FLAG_STREAM_END));

  header_->setStreamFlag(streamFlag);
  sendMessage(callback, std::move(data));
  header_->clearStreamFlag();
}

// Interface from MessageChannel::RecvCallback
bool HeaderServerChannel::shouldSample() {
  return (sampleRate_ > 0) &&
    ((sample_++ % sampleRate_) == 0);
}

void HeaderServerChannel::messageReceived(unique_ptr<IOBuf>&& buf,
                                          unique_ptr<sample> sample) {
  DestructorGuard dg(this);

  buf = handleSecurityMessage(std::move(buf));

  if (!buf) {
    return;
  }

  uint32_t recvSeqId = header_->getSequenceNumber();
  bool outOfOrder = (header_->getFlags() & HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  if (!outOfOrder) {
    // Create a new seqid for in-order messages because they might not
    // be sequential.  This seqid is only used internally in HeaderServerChannel
    recvSeqId = arrivalSeqId_++;
  }

  bool clientExpectsStreaming = false;

  // Currently only HeaderClientChannel sets the stream flags.
  // It also sets HEADER_FLAG_SUPPRORT_OUT_OF_ORDER, and currently there
  // is no way for the client to unset this flag in the HeaderClientChannel,
  // so all streaming message will have outOfOrder == true
  if (outOfOrder) {
    uint16_t streamFlag = header_->getStreamFlag();
    header_->clearStreamFlag();

    if ((streamFlag == HEADER_FLAG_STREAM_PIECE) ||
        (streamFlag == HEADER_FLAG_STREAM_END)) {
      // since the client supports out of order, the sequenceId will be
      // unique to the stream
      auto streamIte = streams_.find(recvSeqId);
      if (streamIte != streams_.end()) {
        auto& stream = streamIte->second;
        stream->notifyReceive(std::move(buf));
      } else {
        // we didn't find the stream, possibly because it has errored on our
        // side and data is still being sent to us
        VLOG(5) << "no stream found with id " << recvSeqId;
      }

      return;
    }

    clientExpectsStreaming = (streamFlag == HEADER_FLAG_STREAM_BEGIN);
  }

  if (callback_) {
    Stream* streamPtr = nullptr;

    unique_ptr<Request> request(
        new HeaderRequest(recvSeqId, this, std::move(buf),
                          header_->getHeaders(),
                          header_->getTransforms(),
                          outOfOrder,
                          clientExpectsStreaming,
                          &streamPtr,
                          std::move(sample)));

    if (!outOfOrder) {
      if (inOrderRequests_.size() > MAX_REQUEST_SIZE) {
        // There is probably nothing useful we can do here.
        LOG(WARNING) << "Hit in order request buffer limit";
        TTransportException ex("Hit in order request buffer limit");
        messageReceiveError(make_exception_ptr(ex));
        return;
      }
    }

    try {
      callback_->requestReceived(std::move(request));
    } catch (const std::exception& e) {
      LOG(WARNING) << "Could not parse request: " << e.what();
    }

    if (streamPtr != nullptr) {
      CHECK(clientExpectsStreaming);
      auto inserted = streams_.insert(std::make_pair(recvSeqId, streamPtr));
      CHECK(inserted.second);
    }
  }
}

void HeaderServerChannel::messageChannelEOF() {
  DestructorGuard dg(this);

  TTransportException error("Channel Closed");
  std::exception_ptr errorPtr = std::make_exception_ptr(error);
  for (auto& pair: streams_) {
    pair.second->notifyError(errorPtr);
  }

  if (callback_) {
    callback_->channelClosed(std::make_exception_ptr(error));
  }
}

void HeaderServerChannel::messageReceiveError(std::exception_ptr&& ex) {
  DestructorGuard dg(this);

  VLOG(1) << "Receive error: " << folly::exceptionStr(ex);

  for (auto& pair: streams_) {
    pair.second->notifyError(ex);
  }

  if (callback_) {
    callback_->channelClosed(std::move(ex));
  }
}

void HeaderServerChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  messageReceiveError(ex.getExceptionPtr());
}

unique_ptr<IOBuf> HeaderServerChannel::handleSecurityMessage(
  unique_ptr<IOBuf>&& buf) {
  if (header_->getClientType() == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (!header_->isSupportedClient() || !saslServer_) {
      if (protectionState_ == ProtectionState::UNKNOWN) {
        // The client tried to use SASL, but it's not supported by
        // policy.  Tell the client to fall back.

        // TODO mhorowitz: generate a real message here.
        try {
          sendMessage(nullptr, IOBuf::create(0));
        } catch (const std::exception& e) {
          LOG(ERROR) << "Failed to send message: " << e.what();
        }

        return nullptr;
      } else {
        // The supported client set changed halfway through or
        // something.  Bail out.
        protectionState_ = ProtectionState::INVALID;
        LOG(WARNING) << "Inconsistent SASL support";
        TTransportException ex("Inconsistent SASL support");
        messageReceiveError(make_exception_ptr(ex));
        return nullptr;
      }
    } else if (protectionState_ == ProtectionState::UNKNOWN ||
        protectionState_ == ProtectionState::INPROGRESS) {
      protectionState_ = ProtectionState::INPROGRESS;
      saslServer_->consumeFromClient(&saslServerCallback_, std::move(buf));
      return nullptr;
    }
    // else, fall through to application message processing
  } else if (protectionState_ == ProtectionState::VALID ||
      (protectionState_ == ProtectionState::INPROGRESS &&
          !header_->isSupportedClient())) {
    // Either negotiation has completed or negotiation is incomplete,
    // non-sasl was received, but is not permitted.
    // We should fail hard in this case.
    protectionState_ = ProtectionState::INVALID;
    LOG(WARNING) << "non-SASL message received on SASL channel";
    TTransportException ex("non-SASL message received on SASL channel");
    messageReceiveError(make_exception_ptr(ex));
    return nullptr;
  } else if (protectionState_ == ProtectionState::UNKNOWN) {
    // This is the path non-SASL-aware (or SASL-disabled) clients will
    // take.
    VLOG(5) << "non-SASL client connection received";
    protectionState_ = ProtectionState::NONE;
  } else if (protectionState_ == ProtectionState::INPROGRESS &&
      header_->isSupportedClient()) {
    // If a client  permits a non-secure connection, we allow falling back to
    // one even if a SASL handshake is in progress.
    LOG(INFO) << "Client initiated a fallback during a SASL handshake";
    // Cancel any SASL-related state, and log
    protectionState_ = ProtectionState::NONE;
    saslServerCallback_.cancelTimeout();
    if (saslServer_) {
      // Should be set here, but just in case check that saslServer_
      // exists
      saslServer_->markChannelCallbackUnavailable();
    }
    const auto& observer = std::dynamic_pointer_cast<TServerObserver>(
      getEventBase()->getObserver());
    if (observer) {
      observer->saslFallBack();
    }
  }

  return std::move(buf);
}

void HeaderServerChannel::SaslServerCallback::saslSendClient(
    std::unique_ptr<folly::IOBuf>&& response) {
  if (channel_.timeoutSASL_ > 0) {
    channel_.timer_->scheduleTimeout(this,
        std::chrono::milliseconds(channel_.timeoutSASL_));
  }
  try {
    channel_.sendMessage(nullptr, std::move(response));
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to send message: " << e.what();
  }
}

void HeaderServerChannel::SaslServerCallback::saslError(
    folly::exception_wrapper&& ex) {
  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
  const auto& observer = std::dynamic_pointer_cast<TServerObserver>(
    channel_.getEventBase()->getObserver());

  try {
    // Fall back to insecure.  This will throw an exception if the
    // insecure client type is not supported.
    channel_.header_->setClientType(THRIFT_HEADER_CLIENT_TYPE);
  } catch (const std::exception& e) {
    if (observer) {
      observer->saslError();
    }
    channel_.protectionState_ = ProtectionState::INVALID;
    LOG(ERROR) << "SASL required by server but failed";
    channel_.messageReceiveErrorWrapped(std::move(ex));
    return;
  }

  if (observer) {
    observer->saslFallBack();
  }

  LOG(INFO) << "SASL server falling back to insecure: " << ex->what();

  // Send the client a null message so the client will try again.
  // TODO mhorowitz: generate a real message here.
  channel_.header_->setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
  try {
    channel_.sendMessage(nullptr, IOBuf::create(0));
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to send message: " << e.what();
  }
  channel_.protectionState_ = ProtectionState::NONE;
  // We need to tell saslServer that the security channel is no longer
  // available, so that it does not attempt to send messages to the server.
  // Since the server-side SASL code is virtually non-blocking, it should be
  // rare that this is actually necessary.
  channel_.saslServer_->markChannelCallbackUnavailable();
}

void HeaderServerChannel::SaslServerCallback::saslComplete() {
  const auto& observer = std::dynamic_pointer_cast<TServerObserver>(
    channel_.getEventBase()->getObserver());

  if (observer) {
    observer->saslComplete();
  }

  apache::thrift::async::HHWheelTimer::Callback::cancelTimeout();
  auto& saslServer = channel_.saslServer_;
  VLOG(5) << "SASL server negotiation complete: "
             << saslServer->getServerIdentity() << " <= "
             << saslServer->getClientIdentity();
  channel_.protectionState_ = ProtectionState::VALID;
  channel_.setSaslEndpoint(saslServer.get());
}

void HeaderServerChannel::unregisterStream(uint32_t sequenceId) {
  auto streamIte = streams_.find(sequenceId);
  CHECK(streamIte != streams_.end());
  streams_.erase(streamIte);
}

HeaderServerChannel::Stream::Stream(
      HeaderServerChannel* channel,
      uint32_t sequenceId,
      const std::vector<uint16_t>& trans,
      const std::map<std::string, std::string>& headers,
      std::chrono::milliseconds timeout,
      std::unique_ptr<folly::IOBuf>&& buffer,
      std::unique_ptr<StreamManager>&& manager,
      MessageChannel::SendCallback* sendCallback)
  : channel_(channel),
    sequenceId_(sequenceId),
    transforms_(trans),
    headers_(headers),
    timeout_(timeout),
    buffer_(std::move(buffer)),
    manager_(std::move(manager)),
    sendCallback_(sendCallback),
    hasOutstandingSend_(true) {
  CHECK(manager_);
  manager_->setChannelCallback(this);
  resetTimeout();
}

void HeaderServerChannel::Stream::sendQueued() {
  CHECK(hasOutstandingSend_);
  if (hasSendCallback()) {
    sendCallback_->sendQueued();
  }
}

void HeaderServerChannel::Stream::messageSent() {
  CHECK(hasOutstandingSend_);
  hasOutstandingSend_ = false;

  if (hasSendCallback()) {
    sendCallback_->messageSent();
    sendCallback_ = nullptr;
  }

  if (!manager_->isDone()) {
    manager_->notifySend();
  }

  deleteThisIfNecessary();
}

void HeaderServerChannel::Stream::messageSendError(
    folly::exception_wrapper&& error) {
  CHECK(hasOutstandingSend_);
  hasOutstandingSend_ = false;

  if (hasSendCallback()) {
    auto errorCopy = error;
    sendCallback_->messageSendError(std::move(errorCopy));
    sendCallback_ = nullptr;
  }

  if (!manager_->isDone()) {
    manager_->notifyError(error.getExceptionPtr());
  }

  deleteThisIfNecessary();
}

void HeaderServerChannel::Stream::notifyReceive(unique_ptr<IOBuf>&& buf) {
  if (!manager_->isDone()) {
    manager_->notifyReceive(std::move(buf));
  }
  if (!manager_->isDone()) {
    resetTimeout();
  }
  deleteThisIfNecessary();
}

void HeaderServerChannel::Stream::notifyError(const std::exception_ptr& error) {
  manager_->notifyError(error);
  deleteThisIfNecessary();
}

void HeaderServerChannel::Stream::timeoutExpired() noexcept {
  TTransportException error(TTransportException::TIMED_OUT, "Receive Expired");
  std::exception_ptr errorPtr = std::make_exception_ptr(error);
  manager_->notifyError(errorPtr);

  TApplicationException serializableError(
      TApplicationException::TApplicationExceptionType::TIMEOUT,
      "Task expired");
  channel_->header_->setHeader("ex", "1");
  auto errorBuffer = serializeError(channel_->header_->getProtocolId(),
                                    serializableError,
                                    buffer_.get());
  channel_->sendMessage(nullptr, std::move(errorBuffer));
  deleteThisIfNecessary();
}

void HeaderServerChannel::Stream::onChannelDestroy() {
  // currently we can reach here here in the following cases:
  //    1. client timed out
  //    2. we got an error while waiting for an outstanding send callback
  //       and the channel closes before the send callback fires

  if (!manager_->hasError()) {
    TTransportException error("Channel Destroyed");
    manager_->notifyError(std::make_exception_ptr(error));
  } else if (manager_->hasError()) {
    CHECK(hasOutstandingSend_);
  }

  if (hasSendCallback()) {
    auto error = folly::make_exception_wrapper<TTransportException>(
        "An error occurred before send completed.");
    sendCallback_->messageSendError(std::move(error));
    sendCallback_ = nullptr;
  }

  hasOutstandingSend_ = false;

  // deletion of this stream is handled by the caller of this function
}

bool HeaderServerChannel::Stream::hasSendCallback() {
  return sendCallback_ != nullptr;
}

void HeaderServerChannel::Stream::resetTimeout() {
  cancelTimeout();
  if (timeout_ > std::chrono::milliseconds(0)) {
    channel_->timer_->scheduleTimeout(this, timeout_);
  }
}

void HeaderServerChannel::Stream::onStreamSend(unique_ptr<IOBuf>&& data) {
  CHECK(!hasOutstandingSend_);
  CHECK(!data->empty());

  hasOutstandingSend_ = true;

  sendStreamingMessage(std::move(data), this);
}

void HeaderServerChannel::Stream::sendStreamingMessage(
    unique_ptr<IOBuf>&& data,
    SendCallback * callback) {

  HEADER_FLAGS streamFlag = (manager_->isSendingEnd() || manager_->hasError()) ?
                             HEADER_FLAG_STREAM_END : HEADER_FLAG_STREAM_PIECE;

  try {
    channel_->header_->setSequenceNumber(sequenceId_);
    channel_->header_->setTransforms(transforms_);
    channel_->sendStreamingMessage(callback, std::move(data), streamFlag);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to send message: " << e.what();
  }
}

void HeaderServerChannel::Stream::onOutOfLoopStreamError(
    const std::exception_ptr& error) {
  // Since sync streams are not supported on the server side,
  // it is not possible to have a stream error from outside
  // the event base loop
  CHECK(false);
}

void HeaderServerChannel::Stream::deleteThisIfNecessary() {
  if (manager_->isDone() && !hasOutstandingSend_) {
    channel_->unregisterStream(sequenceId_);
    delete this;
  }
}

HeaderServerChannel::Stream::~Stream() {
  CHECK(!hasOutstandingSend_);
  // any timeout will be automatically cancelled on destruction
}

}} // apache::thrift
