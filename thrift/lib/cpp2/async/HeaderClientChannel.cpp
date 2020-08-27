/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <utility>

#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>

using folly::IOBuf;
using folly::IOBufQueue;
using std::make_unique;
using std::pair;
using std::unique_ptr;
using namespace apache::thrift::transport;
using folly::EventBase;
using folly::RequestContext;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;

namespace apache {
namespace thrift {

template class ChannelCallbacks::TwowayCallback<HeaderClientChannel>;

HeaderClientChannel::HeaderClientChannel(
    const std::shared_ptr<folly::AsyncTransport>& transport)
    : HeaderClientChannel(std::shared_ptr<Cpp2Channel>(Cpp2Channel::newChannel(
          transport,
          make_unique<ClientFramingHandler>(*this)))) {}

HeaderClientChannel::HeaderClientChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel)
    : sendSeqId_(0),
      closeCallback_(nullptr),
      timeout_(0),
      keepRegisteredForClose_(true),
      cpp2Channel_(cpp2Channel),
      protocolId_(apache::thrift::protocol::T_COMPACT_PROTOCOL) {}

void HeaderClientChannel::setTimeout(uint32_t ms) {
  getTransport()->setSendTimeout(ms);
  timeout_ = ms;
}

void HeaderClientChannel::closeNow() {
  cpp2Channel_->closeNow();
}

void HeaderClientChannel::destroy() {
  closeNow();
  folly::DelayedDestruction::destroy();
}

void HeaderClientChannel::useAsHttpClient(
    const std::string& host,
    const std::string& uri) {
  setClientType(THRIFT_HTTP_CLIENT_TYPE);
  httpClientParser_ = std::make_shared<util::THttpClientParser>(host, uri);
}

bool HeaderClientChannel::good() {
  auto transport = getTransport();
  return transport && transport->good();
}

void HeaderClientChannel::attachEventBase(EventBase* eventBase) {
  cpp2Channel_->attachEventBase(eventBase);
}

void HeaderClientChannel::detachEventBase() {
  cpp2Channel_->detachEventBase();
}

bool HeaderClientChannel::isDetachable() {
  return getTransport()->isDetachable() && recvCallbacks_.empty();
}

bool HeaderClientChannel::clientSupportHeader() {
  return getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
      getClientType() == THRIFT_HTTP_CLIENT_TYPE;
}

// Client Interface
void HeaderClientChannel::sendRequestNoResponse(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& serializedRequest,
    std::shared_ptr<THeader> header,
    RequestClientCallback::Ptr cb) {
  auto buf =
      LegacySerializedRequest(
          header->getProtocolId(), methodName, std::move(serializedRequest))
          .buffer;

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  // Both cb and buf are allowed to be null.
  uint32_t oldSeqId = sendSeqId_;
  sendSeqId_ = ResponseChannel::ONEWAY_REQUEST_ID;

  if (cb) {
    sendMessage(
        new OnewayCallback(std::move(cb)), std::move(buf), header.get());
  } else {
    sendMessage(nullptr, std::move(buf), header.get());
  }
  sendSeqId_ = oldSeqId;
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
      getClientType() == THRIFT_HTTP_CLIENT_TYPE) {
    return protocolId_;
  } else if (getClientType() == THRIFT_FRAMED_COMPACT) {
    return T_COMPACT_PROTOCOL;
  } else {
    return T_BINARY_PROTOCOL; // Assume other transports use TBinary
  }
}

void HeaderClientChannel::sendRequestResponse(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& serializedRequest,
    std::shared_ptr<THeader> header,
    RequestClientCallback::Ptr cb) {
  auto buf =
      LegacySerializedRequest(
          header->getProtocolId(), methodName, std::move(serializedRequest))
          .buffer;

  // cb is not allowed to be null.
  DCHECK(cb);

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

  auto twcb = new TwowayCallback<HeaderClientChannel>(
      this, sendSeqId_, std::move(cb), &getEventBase()->timer(), timeout);

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  if (getClientType() != THRIFT_HEADER_CLIENT_TYPE) {
    recvCallbackOrder_.push_back(sendSeqId_);
  }
  recvCallbacks_[sendSeqId_] = twcb;
  try {
    setBaseReceivedCallback(); // Cpp2Channel->setReceiveCallback can throw
  } catch (const TTransportException& ex) {
    twcb->messageSendError(
        folly::exception_wrapper(std::current_exception(), ex));
    return;
  }

  sendMessage(twcb, std::move(buf), header.get());
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderClientChannel::ClientFramingHandler::addFrame(
    unique_ptr<IOBuf> buf,
    THeader* header) {
  channel_.updateClientType(header->getClientType());
  header->setSequenceNumber(channel_.sendSeqId_);
  return header->addHeader(
      std::move(buf), channel_.getPersistentWriteHeaders());
}

std::tuple<std::unique_ptr<IOBuf>, size_t, std::unique_ptr<THeader>>
HeaderClientChannel::ClientFramingHandler::removeFrame(IOBufQueue* q) {
  std::unique_ptr<THeader> header(new THeader(THeader::ALLOW_BIG_FRAMES));
  if (!q || !q->front() || q->front()->empty()) {
    return make_tuple(std::unique_ptr<IOBuf>(), 0, nullptr);
  }

  size_t remaining = 0;
  std::unique_ptr<folly::IOBuf> buf =
      header->removeHeader(q, remaining, channel_.getPersistentReadHeaders());
  if (!buf) {
    return make_tuple(std::unique_ptr<folly::IOBuf>(), remaining, nullptr);
  }
  channel_.checkSupportedClient(header->getClientType());
  return make_tuple(std::move(buf), 0, std::move(header));
}

// Interface from MessageChannel::RecvCallback
void HeaderClientChannel::messageReceived(
    unique_ptr<IOBuf>&& buf,
    unique_ptr<THeader>&& header) {
  DestructorGuard dg(this);

  if (!buf) {
    return;
  }

  uint32_t recvSeqId;

  if (header->getClientType() != THRIFT_HEADER_CLIENT_TYPE) {
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

  recvCallbacks_.erase(recvSeqId);
  // we are the last callback?
  setBaseReceivedCallback();
  f->replyReceived(std::move(buf), std::move(header));
}

void HeaderClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
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

  setBaseReceivedCallback(); // was this the last callback?
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
  if (recvCallbacks_.size() != 0 ||
      (closeCallback_ && keepRegisteredForClose_)) {
    cpp2Channel_->setReceiveCallback(this);
  } else {
    cpp2Channel_->setReceiveCallback(nullptr);
  }
}

} // namespace thrift
} // namespace apache
