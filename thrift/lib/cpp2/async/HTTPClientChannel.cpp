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
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
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
using apache::thrift::async::TEventBase;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::transport::THeader;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;

namespace apache {
namespace thrift {

template class ChannelCallbacks::TwowayCallback<HTTPClientChannel>;

HTTPClientChannel::HTTPClientChannel(
    const std::shared_ptr<TAsyncTransport>& transport,
    const std::string& host,
    const std::string& url)
    : HTTPClientChannel(
          std::shared_ptr<Cpp2Channel>(Cpp2Channel::newChannel(
              transport, make_unique<ClientFramingHandler>(*this))),
          host,
          url) {}

HTTPClientChannel::HTTPClientChannel(
    const std::shared_ptr<TAsyncTransport>& transport,
    const std::string& host,
    const std::string& url,
    std::unique_ptr<proxygen::HTTPCodec> codec)
    : HTTPClientChannel(
          std::shared_ptr<Cpp2Channel>(Cpp2Channel::newChannel(
              transport, make_unique<ClientFramingHandler>(*this))),
          host,
          url,
          std::move(codec)) {}

HTTPClientChannel::HTTPClientChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel,
    const std::string& host,
    const std::string& url)
    : HTTPClientChannel(cpp2Channel,
                        host,
                        url,
                        folly::make_unique<proxygen::HTTP1xCodec>(
                            proxygen::TransportDirection::UPSTREAM)) {}

HTTPClientChannel::HTTPClientChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel,
    const std::string& host,
    const std::string& url,
    std::unique_ptr<proxygen::HTTPCodec> codec)
    : httpCallback_(folly::make_unique<HTTPCodecCallback>(this)),
      httpCodec_(std::move(codec)),
      httpHost_(host),
      httpUrl_(url),
      sendSeqId_(0),
      closeCallback_(nullptr),
      timeout_(0),
      keepRegisteredForClose_(true),
      cpp2Channel_(cpp2Channel),
      timer_(new folly::HHWheelTimer(getEventBase())),
      protocolId_(apache::thrift::protocol::T_BINARY_PROTOCOL) {
  httpCodec_->setCallback(httpCallback_.get());
}

void HTTPClientChannel::setTimeout(uint32_t ms) {
  getTransport()->setSendTimeout(ms);
  timeout_ = ms;
}

void HTTPClientChannel::closeNow() { cpp2Channel_->closeNow(); }

void HTTPClientChannel::destroy() {
  closeNow();
  async::TDelayedDestruction::destroy();
}

void HTTPClientChannel::attachEventBase(TEventBase* eventBase) {
  cpp2Channel_->attachEventBase(eventBase);
  timer_->attachEventBase(eventBase);
}

void HTTPClientChannel::detachEventBase() {
  cpp2Channel_->detachEventBase();
  timer_->detachEventBase();
}

bool HTTPClientChannel::isDetachable() {
  return getTransport()->isDetachable() && timer_->isDetachable();
}

// Client Interface
uint32_t HTTPClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  cb->context_ = RequestContext::saveContext();

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  // Both cb and buf are allowed to be null.
  uint32_t oldSeqId = sendSeqId_;
  sendSeqId_ = ResponseChannel::ONEWAY_REQUEST_ID;

  if (cb) {
    sendMessage(new OnewayCallback(std::move(cb), std::move(ctx), false),
                std::move(buf),
                header.get());
  } else {
    sendMessage(nullptr, std::move(buf), header.get());
  }
  sendSeqId_ = oldSeqId;
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

void HTTPClientChannel::setCloseCallback(CloseCallback* cb) {
  closeCallback_ = cb;
  setBaseReceivedCallback();
}

void HTTPClientChannel::setRequestHeaderOptions(THeader* header) {
  if (httpCodec_->supportsParallelRequests()) {
    header->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  }
  header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
  header->forceClientType(THRIFT_HTTP_CLIENT_TYPE);
}

uint32_t HTTPClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  // cb is not allowed to be null.
  DCHECK(cb);

  cb->context_ = RequestContext::saveContext();

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

  auto twcb =
      new TwowayCallback<HTTPClientChannel>(this,
                                            sendSeqId_,
                                            protocolId_,
                                            std::move(cb),
                                            std::move(ctx),
                                            timer_.get(),
                                            timeout,
                                            rpcOptions.getChunkTimeout());

  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);

  recvCallbackOrder_.push_back(sendSeqId_);
  recvCallbacks_[sendSeqId_] = twcb;
  setBaseReceivedCallback();

  sendMessage(twcb, std::move(buf), header.get());
  return sendSeqId_;
}

// Header framing
std::unique_ptr<folly::IOBuf> HTTPClientChannel::ClientFramingHandler::addFrame(
    unique_ptr<IOBuf> buf, apache::thrift::transport::THeader* header) {
  const auto& codec = channel_.httpCodec_;
  proxygen::HTTPCodec::StreamID sid = codec->createStream();

  channel_.streamIDToSeqId_[sid] = channel_.sendSeqId_;

  folly::IOBufQueue writeBuf;

  proxygen::HTTPMessage msg;
  msg.setMethod(proxygen::HTTPMethod::POST);
  msg.setURL(channel_.httpUrl_);
  msg.setHTTPVersion(1, 1);
  msg.setIsChunked(false);
  auto& headers = msg.getHeaders();

  auto pwh = channel_.getPersistentWriteHeaders();

  for (auto it = pwh.begin(); it != pwh.end(); ++it) {
    headers.rawSet(it->first, it->second);
  }

  // We do not clear the persistent write headers, since http does not
  // distinguish persistent/per request headers
  // pwh.clear();

  auto wh = header->getWriteHeaders();

  for (auto it = wh.begin(); it != wh.end(); ++it) {
    headers.rawSet(it->first, it->second);
  }

  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_HOST, channel_.httpHost_);
  headers.set(proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_LENGTH,
              folly::to<std::string>(buf->computeChainDataLength()));

  wh.clear();

  codec->generateHeader(writeBuf, sid, msg);
  codec->generateBody(writeBuf, sid, std::move(buf), boost::none, true);

  return writeBuf.move();
}

std::tuple<std::unique_ptr<IOBuf>, size_t, std::unique_ptr<THeader>>
HTTPClientChannel::ClientFramingHandler::removeFrame(IOBufQueue* q) {
  if (q && q->front() && !q->front()->empty()) {
    const auto& codec = channel_.httpCodec_;

    auto bytes_read = 1;
    while (bytes_read && q->front()) {
      bytes_read = codec->onIngress(*(q->front()));
      q->trimStart(bytes_read);
    }
  }

  if (channel_.completedStreamIDs_.empty()) {
    return make_tuple(std::unique_ptr<IOBuf>(), 1, nullptr);
  } else {
    auto header = folly::make_unique<THeader>();

    auto sid = channel_.completedStreamIDs_.front();
    channel_.completedStreamIDs_.pop_front();

    auto reqid = channel_.streamIDToSeqId_.find(sid);
    auto msg = channel_.streamIDToMsg_.find(sid);
    auto body = channel_.streamIDToBody_.find(sid);

    CHECK(reqid != channel_.streamIDToSeqId_.end());
    CHECK(msg != channel_.streamIDToMsg_.end());
    CHECK(body != channel_.streamIDToBody_.end());

    header->setSequenceNumber(reqid->second);

    THeader::StringToStringMap readHeaders;

    auto hdrs = msg->second->getHeaders();

    hdrs.forEach([&](const std::string& key, const std::string& val) {
      readHeaders[key] = val;
    });

    auto trlrs = msg->second->getTrailers();

    if (trlrs) {
      trlrs->forEach([&](const std::string& key, const std::string& val) {
        readHeaders[key] = val;
      });
    }

    header->setReadHeaders(std::move(readHeaders));

    auto response = make_tuple(body->second->move(), 0, std::move(header));

    channel_.streamIDToSeqId_.erase(reqid);
    channel_.streamIDToMsg_.erase(msg);
    channel_.streamIDToBody_.erase(body);

    return std::move(response);
  }
}

// Interface from MessageChannel::RecvCallback
void HTTPClientChannel::messageReceived(
    unique_ptr<IOBuf>&& buf,
    std::unique_ptr<THeader>&& header,
    unique_ptr<MessageChannel::RecvCallback::sample>) {
  DestructorGuard dg(this);

  uint32_t recvSeqId;

  recvSeqId = recvCallbackOrder_.front();
  recvCallbackOrder_.pop_front();

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

void HTTPClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
  messageReceiveErrorWrapped(
      folly::make_exception_wrapper<TTransportException>("Channel got EOF"));
  if (closeCallback_) {
    closeCallback_->channelClosed();
    closeCallback_ = nullptr;
  }
  setBaseReceivedCallback();
}

void HTTPClientChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  DestructorGuard dg(this);

  // Clear callbacks early.  The last callback can delete the client,
  // which may cause the channel to be destroy()ed, which will call
  // messageChannelEOF(), which will reenter messageReceiveError().

  decltype(recvCallbacks_) callbacks;
  using std::swap;
  swap(recvCallbacks_, callbacks);

  if (!callbacks.empty()) {
    for (auto& cb : callbacks) {
      if (cb.second) {
        cb.second->requestError(ex);
      }
    }
  }

  setBaseReceivedCallback();
}

void HTTPClientChannel::eraseCallback(uint32_t seqId,
                                      TwowayCallback<HTTPClientChannel>* cb) {
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  CHECK(it != recvCallbacks_.end());
  CHECK(it->second == cb);
  recvCallbacks_.erase(it);

  setBaseReceivedCallback(); // was this the last callback?
}

bool HTTPClientChannel::expireCallback(uint32_t seqId) {
  VLOG(4) << "Expiring callback with sequence id " << seqId;
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  if (it != recvCallbacks_.end()) {
    it->second->expire();
    return true;
  }

  return false;
}

void HTTPClientChannel::setBaseReceivedCallback() {
  if (recvCallbacks_.size() != 0 ||
      (closeCallback_ && keepRegisteredForClose_)) {
    cpp2Channel_->setReceiveCallback(this);
  } else {
    cpp2Channel_->setReceiveCallback(nullptr);
  }
}
}
} // apache::thrift
