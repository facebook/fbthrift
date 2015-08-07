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

#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <folly/io/Cursor.h>
#include <folly/String.h>

#include <utility>
#include <exception>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using folly::make_unique;
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
    : HeaderServerChannel(
        std::shared_ptr<Cpp2Channel>(
            Cpp2Channel::newChannel(transport,
                make_unique<ServerFramingHandler>(*this))))
{}

HeaderServerChannel::HeaderServerChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel)
    : callback_(nullptr)
    , arrivalSeqId_(1)
    , lastWrittenSeqId_(0)
    , sampleRate_(0)
    , timeoutSASL_(5000)
    , saslServerCallback_(*this)
    , cpp2Channel_(cpp2Channel)
    , timer_(new apache::thrift::async::HHWheelTimer(getEventBase())) {}

void HeaderServerChannel::destroy() {
  DestructorGuard dg(this);

  saslServerCallback_.cancelTimeout();
  if (saslServer_) {
    saslServer_->detachEventBase();
  }

  if (callback_) {
    auto error = folly::make_exception_wrapper<TTransportException>(
        "Channel destroyed");
    callback_->channelClosed(std::move(error));
  }

  cpp2Channel_->closeNow();

  TDelayedDestruction::destroy();
}

// Header framing
unique_ptr<IOBuf>
HeaderServerChannel::ServerFramingHandler::addFrame(unique_ptr<IOBuf> buf,
                                                    THeader* header) {
  channel_.updateClientType(header->getClientType());

  // Note: This THeader function may throw.  However, we don't want to catch
  // it here, because this would send an empty message out on the wire.
  // Instead we have to catch it at sendMessage
  return header->addHeader(
    std::move(buf),
    channel_.getPersistentWriteHeaders(),
    false /* Data already transformed in AsyncProcessor.h */);
}

std::tuple<unique_ptr<IOBuf>, size_t, unique_ptr<THeader>>
HeaderServerChannel::ServerFramingHandler::removeFrame(IOBufQueue* q) {
  std::unique_ptr<THeader> header(new THeader);
  // removeHeader will set seqid in header.
  // For older clients with seqid in the protocol, header
  // will dig in to the protocol to get the seqid correctly.
  if (!q || !q->front() || q->front()->empty()) {
    return make_tuple(std::unique_ptr<IOBuf>(), 0, nullptr);
  }

  std::unique_ptr<folly::IOBuf> buf;
  size_t remaining = 0;
  try {
    buf = header->removeHeader(q, remaining,
                               channel_.getPersistentReadHeaders());
  } catch (const std::exception& e) {
    LOG(ERROR) << "Received invalid request from client: "
               << folly::exceptionStr(e) << " "
               << getTransportDebugString(channel_.getTransport());
    throw;
  }
  if (!buf) {
    return make_tuple(std::unique_ptr<IOBuf>(), remaining, nullptr);
  }

  CLIENT_TYPE ct = header->getClientType();
  if (!channel_.isSupportedClient(ct) &&
      ct != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    LOG(ERROR) << "Server rejecting unsupported client type " << ct;
    channel_.checkSupportedClient(ct);
  }

  // In order to allow negotiation to happen when the client requests
  // sasl but it's not supported, we don't throw an exception in the
  // sasl case.  Instead, we let the message bubble up, and check if
  // the client is supported in handleSecurityMessage called from the
  // messageReceived callback.

  header->setMinCompressBytes(channel_.getMinCompressBytes());
  return make_tuple(std::move(buf), 0, std::move(header));
}

std::string
HeaderServerChannel::getTransportDebugString(TAsyncTransport* transport) {
  if (!transport) {
    return std::string();
  }

  auto ret = folly::to<std::string>("(transport ",
                                    folly::demangle(typeid(*transport)));

  try {
    folly::SocketAddress addr;
    transport->getPeerAddress(&addr);
    folly::toAppend(", address ", addr.getAddressStr(),
                    ", port ", addr.getPort(),
                    &ret);
  } catch (const std::exception& e) {
  }

  ret += ')';
  return std::move(ret);
}

// Client Interface

HeaderServerChannel::HeaderRequest::HeaderRequest(
      HeaderServerChannel* channel,
      unique_ptr<IOBuf>&& buf,
      unique_ptr<THeader>&& header,
      bool outOfOrder,
      unique_ptr<sample> sample)
  : channel_(channel)
  , header_(std::move(header))
  , outOfOrder_(outOfOrder)
  , active_(true) {

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
    MessageChannel::SendCallback* cb) {
  if (!outOfOrder_) {
    // In order processing, make sure the ordering is correct.
    if (InOrderRecvSeqId_ != channel_->lastWrittenSeqId_ + 1) {
      // Save it until we can send it in order.
      channel_->inOrderRequests_[InOrderRecvSeqId_] =
          std::make_tuple(cb, std::move(buf), std::move(header_));
    } else {
      // Send it now, and send any subsequent requests in order.
      channel_->sendCatchupRequests(std::move(buf), cb, std::move(header_));
    }
  } else {
    if (!buf) {
      // oneway calls are OK do this, but is a bug for twoway.
      DCHECK(isOneway());
      return;
    }
    try {
      // out of order, send as soon as it is done.
      channel_->sendMessage(cb, std::move(buf), header_.get());
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
void HeaderServerChannel::HeaderRequest::sendErrorWrapped(
    folly::exception_wrapper ew,
    std::string exCode,
    MessageChannel::SendCallback* cb) {

  // Other types are unimplemented.
  DCHECK(ew.is_compatible_with<TApplicationException>());

  header_->setHeader("ex", exCode);
  ew.with_exception<TApplicationException>([&](TApplicationException& tae) {
      std::unique_ptr<folly::IOBuf> exbuf;
      uint16_t proto = header_->getProtocolId();
      auto transforms = header_->getWriteTransforms();
      try {
        exbuf = serializeError(proto, tae, getBuf());
      } catch (const TProtocolException& pe) {
        if (pe.getType() == TProtocolException::BAD_VERSION) {
          // TODO (haijunz): remove this since now header is per request
          // Bad protocol, maybe because channel_->header_ contains the protocol
          // for a later received message (because header is part of the channel
          // and not of the request). Try with the other protocol.
          uint16_t newproto = (proto == protocol::T_BINARY_PROTOCOL) ?
                  protocol::T_COMPACT_PROTOCOL :
                  protocol::T_BINARY_PROTOCOL;
          try {
            exbuf = serializeError(newproto, tae, getBuf());
          } catch (const TProtocolException& pe2) {
            LOG(ERROR) << "serializeError failed with both binary and compact."
                << " Original proto=" << proto;
            channel_->closeNow();
            return;
          }
        } else {
          LOG(ERROR) << "serializeError failed. type=" << pe.getType()
              << " what()=" << pe.what();
          channel_->closeNow();
          return;
        }
      }
      exbuf = THeader::transform(std::move(exbuf),
                                 transforms,
                                 header_->getMinCompressBytes());
      sendReply(std::move(exbuf), cb);
    });
}

void HeaderServerChannel::sendCatchupRequests(
    std::unique_ptr<folly::IOBuf> next_req,
    MessageChannel::SendCallback* cb,
    std::unique_ptr<THeader> header) {

  DestructorGuard dg(this);

  while (true) {
    if (next_req) {
      try {
        sendMessage(cb, std::move(next_req), header.get());
      } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to send message: " << e.what();
      }
    } else if (nullptr != cb) {
      // There is no message (like a oneway req), but there is a callback
      cb->messageSent();
    }
    lastWrittenSeqId_++;

    // Check for the next req
    auto next = inOrderRequests_.find(lastWrittenSeqId_ + 1);
    if (next != inOrderRequests_.end()) {
      next_req = std::move(std::get<1>(next->second));
      cb = std::get<0>(next->second);
      header = std::move(std::get<2>(next->second));
      inOrderRequests_.erase(lastWrittenSeqId_ + 1);
    } else {
      break;
    }
  }
}

// Interface from MessageChannel::RecvCallback
bool HeaderServerChannel::shouldSample() {
  return (sampleRate_ > 0) &&
    ((sample_++ % sampleRate_) == 0);
}

void HeaderServerChannel::messageReceived(unique_ptr<IOBuf>&& buf,
                                          unique_ptr<THeader>&& header,
                                          unique_ptr<sample> sample) {
  DestructorGuard dg(this);

  buf = handleSecurityMessage(std::move(buf), std::move(header));

  if (!buf) {
    return;
  }

  uint32_t recvSeqId = header->getSequenceNumber();
  bool outOfOrder = (header->getFlags() & HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  if (!outOfOrder) {
    // Create a new seqid for in-order messages because they might not
    // be sequential.  This seqid is only used internally in HeaderServerChannel
    recvSeqId = arrivalSeqId_++;
  }

  if (callback_) {
    unique_ptr<HeaderRequest> request(
        new HeaderRequest(this,
                          std::move(buf),
                          std::move(header),
                          outOfOrder,
                          std::move(sample)));

    if (!outOfOrder) {
      if (inOrderRequests_.size() > MAX_REQUEST_SIZE) {
        // There is probably nothing useful we can do here.
        LOG(WARNING) << "Hit in order request buffer limit";
        auto ex = folly::make_exception_wrapper<TTransportException>(
            "Hit in order request buffer limit");
        messageReceiveErrorWrapped(std::move(ex));
        return;
      }
      request->setInOrderRecvSequenceId(recvSeqId);
    }

    auto ew = folly::try_and_catch<std::exception>([&]() {
        callback_->requestReceived(std::move(request));
      });
    if (ew) {
      LOG(WARNING) << "Could not parse request: " << ew.what();
      messageReceiveErrorWrapped(std::move(ew));
      return;
    }

  }
}

void HeaderServerChannel::messageChannelEOF() {
  DestructorGuard dg(this);

  auto ew = folly::make_exception_wrapper<TTransportException>(
      "Channel Closed");
  if (callback_) {
    callback_->channelClosed(std::move(ew));
  }
}

void HeaderServerChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  DestructorGuard dg(this);

  VLOG(1) << "Receive error: " << ex.what();

  if (callback_) {
    callback_->channelClosed(std::move(ex));
  }
}

unique_ptr<IOBuf> HeaderServerChannel::handleSecurityMessage(
    unique_ptr<IOBuf>&& buf, unique_ptr<THeader>&& header) {
  CLIENT_TYPE ct = header->getClientType();
  if (ct == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (!isSupportedClient(ct) || (!saslServer_ &&
          !cpp2Channel_->getProtectionHandler()->getSaslEndpoint())) {
      if (getProtectionState() == ProtectionState::UNKNOWN) {
        // The client tried to use SASL, but it's not supported by
        // policy.  Tell the client to fall back.

        // TODO mhorowitz: generate a real message here.
        try {
          auto trans = header->getWriteTransforms();
          sendMessage(nullptr,
                      THeader::transform(
                          IOBuf::create(0),
                          trans,
                          getMinCompressBytes()),
                      header.get());
        } catch (const std::exception& e) {
          LOG(ERROR) << "Failed to send message: " << e.what();
        }

        return nullptr;
      } else {
        // The supported client set changed halfway through or
        // something.  Bail out.
        setProtectionState(ProtectionState::INVALID);
        LOG(WARNING) << "Inconsistent SASL support";
        auto ex = folly::make_exception_wrapper<TTransportException>(
            "Inconsistent SASL support");
        messageReceiveErrorWrapped(std::move(ex));
        return nullptr;
      }
    } else if (getProtectionState() == ProtectionState::UNKNOWN ||
        getProtectionState() == ProtectionState::INPROGRESS ||
        getProtectionState() == ProtectionState::WAITING) {
      // Technically we shouldn't get any new messages while in the INPROGRESS
      // state, but we'll allow it to fall through here and let the saslServer_
      // state machine throw an error.
      setProtectionState(ProtectionState::INPROGRESS);
      saslServer_->setProtocolId(header->getProtocolId());
      saslServerCallback_.setHeader(std::move(header));
      saslServer_->consumeFromClient(&saslServerCallback_, std::move(buf));
      return nullptr;
    }
    // else, fall through to application message processing
  } else if ((getProtectionState() == ProtectionState::VALID ||
              getProtectionState() == ProtectionState::INPROGRESS ||
              getProtectionState() == ProtectionState::WAITING) &&
              !isSupportedClient(ct)) {
    // Either negotiation has completed or negotiation is incomplete,
    // non-sasl was received, but is not permitted.
    // We should fail hard in this case.
    setProtectionState(ProtectionState::INVALID);
    LOG(WARNING) << "non-SASL message received on SASL channel";
    auto ex = folly::make_exception_wrapper<TTransportException>(
        "non-SASL message received on SASL channel");
    messageReceiveErrorWrapped(std::move(ex));
    return nullptr;
  } else if (getProtectionState() == ProtectionState::UNKNOWN) {
    // This is the path non-SASL-aware (or SASL-disabled) clients will
    // take.
    VLOG(5) << "non-SASL client connection received";
    setProtectionState(ProtectionState::NONE);
  } else if ((getProtectionState() == ProtectionState::VALID ||
              getProtectionState() == ProtectionState::INPROGRESS ||
              getProtectionState() == ProtectionState::WAITING) &&
              isSupportedClient(ct)) {
    // If a client  permits a non-secure connection, we allow falling back to
    // one even if a SASL handshake is in progress, or SASL handshake has been
    // completed. The reason for latter is that we should allow a fallback if
    // client timed out in the last leg of the handshake.
    VLOG(5) << "Client initiated a fallback during a SASL handshake";
    // Cancel any SASL-related state, and log
    setProtectionState(ProtectionState::NONE);
    saslServerCallback_.cancelTimeout();
    if (saslServer_) {
      // Should be set here, but just in case check that saslServer_
      // exists
      saslServer_->detachEventBase();
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
    auto trans = header_->getWriteTransforms();
    channel_.setProtectionState(ProtectionState::WAITING);
    channel_.sendMessage(nullptr,
                         THeader::transform(
                           std::move(response),
                           trans,
                           channel_.getMinCompressBytes()),
                         header_.get());
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
    channel_.setClientType(THRIFT_HEADER_CLIENT_TYPE);
  } catch (const std::exception& e) {
    if (observer) {
      observer->saslError();
    }
    channel_.setProtectionState(ProtectionState::INVALID);
    LOG(ERROR) << "SASL required by server but failed: " << ex.what();
    channel_.messageReceiveErrorWrapped(std::move(ex));
    return;
  }

  if (observer) {
    observer->saslFallBack();
  }

  LOG(INFO) << "SASL server falling back to insecure: " << ex.what();

  // Send the client a null message so the client will try again.
  // TODO mhorowitz: generate a real message here.
  header_->setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
  try {
    auto trans = header_->getWriteTransforms();
    channel_.sendMessage(nullptr,
                         THeader::transform(
                           IOBuf::create(0),
                           trans,
                           channel_.getMinCompressBytes()),
                         header_.get());
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to send message: " << e.what();
  }
  channel_.setProtectionState(ProtectionState::NONE);
  // We need to tell saslServer that the security channel is no longer
  // available, so that it does not attempt to send messages to the server.
  // Since the server-side SASL code is virtually non-blocking, it should be
  // rare that this is actually necessary.
  channel_.saslServer_->detachEventBase();
}

void HeaderServerChannel::SaslServerCallback::saslComplete() {
  // setProtectionState could eventually destroy the channel
  DestructorGuard dg(&channel_);

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
  channel_.setProtectionState(ProtectionState::VALID);
  channel_.setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
}

}} // apache::thrift
