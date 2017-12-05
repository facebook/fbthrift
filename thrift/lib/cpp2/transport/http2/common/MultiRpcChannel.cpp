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
#include <thrift/lib/cpp2/transport/http2/common/MultiRpcChannel.h>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBaseManager.h>
#include <glog/logging.h>
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
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelFactory.h>
#include <array>
#include <chrono>

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
using proxygen::IOBufPrinter;
using proxygen::ProxygenError;
using proxygen::ResponseHandler;
using std::map;
using std::string;

MultiRpcChannel::MultiRpcChannel(
    ResponseHandler* toHttp2,
    ThriftProcessor* processor)
    : H2Channel(toHttp2), processor_(processor) {
  evb_ = EventBaseManager::get()->getExistingEventBase();
  HTTPMessage msg;
  msg.setStatusCode(200);
  responseHandler_->sendHeaders(msg);
}

MultiRpcChannel::MultiRpcChannel(H2ClientConnection* toHttp2)
    : H2Channel(toHttp2) {
  evb_ = toHttp2->getEventBase();
  callbacks_.reserve(kMaxRpcs);
}

MultiRpcChannel::~MultiRpcChannel() {
  if (rpcsCompleted_ != rpcsInitiated_) {
    LOG(ERROR) << "Some RPCs have not completed";
  } else if (!isClosed_) {
    VLOG(2) << "Channel is not closed";
  }
}

void MultiRpcChannel::initialize(std::chrono::milliseconds timeout) {
  httpTransaction_ = h2ClientConnection_->newTransaction(this);
  httpTransaction_->setIdleTimeout(timeout);
  HTTPMessage msg;
  msg.setMethod(HTTPMethod::POST);
  msg.setURL("/");
  msg.getHeaders().set(
      HTTPHeaderCode::HTTP_HEADER_USER_AGENT, "C++/THttpClient");
  maybeAddChannelVersionHeader(msg, "3");
  httpTransaction_->sendHeaders(msg);
}

void MultiRpcChannel::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  DCHECK(metadata);
  DCHECK(metadata->__isset.seqId);
  DCHECK(evb_->isInEventBaseThread());
  VLOG(2) << "sendThriftResponse:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (responseHandler_) {
    auto metadataBuf = std::make_unique<IOBufQueue>();
    CompactProtocolWriter writer;
    writer.setOutput(metadataBuf.get());
    uint32_t size = metadata->write(&writer);
    size += static_cast<uint32_t>(payload->computeChainDataLength());
    responseHandler_->sendBody(
        combine(size, metadataBuf->move(), std::move(payload)));
  }
  ++rpcsCompleted_;
  if (isClosed_ && rpcsCompleted_ == rpcsInitiated_) {
    VLOG(2) << "closing outgoing stream";
    responseHandler_->sendEOM();
  }
}

void MultiRpcChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(
      metadata->kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
      metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE);
  DCHECK(payload);
  DCHECK(callback);
  VLOG(2) << "sendThriftRequest:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  if (!EnvelopeUtil::stripEnvelope(metadata.get(), payload)) {
    LOG(ERROR) << "Unexpected problem stripping envelope";
    auto evb = callback->getEventBase();
    evb->runInEventBaseThread([cb = std::move(callback)]() mutable {
      cb->onError(folly::exception_wrapper(
          TTransportException("Unexpected problem stripping envelope")));
    });
    return;
  }
  DCHECK(metadata->__isset.protocol);
  DCHECK(metadata->__isset.name);
  DCHECK(callbacks_.size() < kMaxRpcs);
  // The sequence id is the next available index in callbacks_.
  metadata->seqId = callbacks_.size();
  metadata->__isset.seqId = true;
  auto metadataBuf = std::make_unique<IOBufQueue>();
  CompactProtocolWriter writer;
  writer.setOutput(metadataBuf.get());
  uint32_t size = metadata->write(&writer);
  size += static_cast<uint32_t>(payload->computeChainDataLength());
  httpTransaction_->sendBody(
      combine(size, metadataBuf->move(), std::move(payload)));
  // For oneway calls, we move "callback" to "callbackEvb" since we do
  // not require the callback any more.  For twoway calls, we move
  // "callback" to "callbacks_" and use a raw pointer to call
  // "onThriftRequestSent()".  This is safe because the callback will
  // be eventually moved to this same thread to call either
  // "onThriftResponse()" or "onThriftError()".
  auto callbackEvb = callback->getEventBase();
  if (metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    callbackEvb->runInEventBaseThread(
        [cb = std::move(callback)]() mutable { cb->onThriftRequestSent(); });
    callbacks_.push_back(std::unique_ptr<ThriftClientCallback>());
  } else {
    callbackEvb->runInEventBaseThread(
        [cb = callback.get()]() mutable { cb->onThriftRequestSent(); });
    callbacks_.push_back(std::move(callback));
  }
  ++rpcsInitiated_;
}

std::unique_ptr<IOBuf> MultiRpcChannel::combine(
    uint32_t size,
    std::unique_ptr<IOBuf>&& metadata,
    std::unique_ptr<IOBuf>&& payload) noexcept {
  std::array<uint8_t, 4> sizeBuf;
  sizeBuf[0] = static_cast<uint8_t>(size >> 24);
  sizeBuf[1] = static_cast<uint8_t>((size >> 16) & 0xFF);
  sizeBuf[2] = static_cast<uint8_t>((size >> 8) & 0xFF);
  sizeBuf[3] = static_cast<uint8_t>(size & 0xFF);
  auto buf = IOBuf::copyBuffer(sizeBuf, 4);
  buf->prependChain(std::move(metadata));
  buf->prependChain(std::move(payload));
  return buf;
}

EventBase* MultiRpcChannel::getEventBase() noexcept {
  return evb_;
}

void MultiRpcChannel::setInput(int32_t, SubscriberRef) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

ThriftChannelIf::SubscriberRef MultiRpcChannel::getOutput(int32_t) noexcept {
  LOG(FATAL) << "Streaming not supported.";
}

bool MultiRpcChannel::canDoRpcs() noexcept {
  return httpTransaction_ && callbacks_.size() < kMaxRpcs;
}

void MultiRpcChannel::closeClientSide() noexcept {
  VLOG(2) << "closing outgoing stream on client";
  if (httpTransaction_) {
    httpTransaction_->sendEOM();
  }
  isClosed_ = true;
}

void MultiRpcChannel::onH2StreamBegin(std::unique_ptr<HTTPMessage>) noexcept {
  VLOG(2) << "onH2StreamBegin";
}

void MultiRpcChannel::onH2BodyFrame(std::unique_ptr<IOBuf> contents) noexcept {
  VLOG(2) << "onH2BodyFrame: " << std::endl
          << IOBufPrinter::printHexFolly(contents.get(), true);
  std::unique_ptr<IOBuf> clone;
  IOBuf* bufptr = contents.get();
  do {
    if (sizeBytesRemaining_ == 0) {
      clone = bufptr->cloneOne();
    }
    const uint8_t* data = bufptr->data();
    uint64_t length = bufptr->length();
    while (length > 0) {
      if (sizeBytesRemaining_ > 0) {
        payloadBytesRemaining_ = (payloadBytesRemaining_ << 8) | *data;
        ++data;
        --length;
        if (--sizeBytesRemaining_ == 0) {
          if (length != 0) {
            clone = bufptr->cloneOne();
            clone->trimStart(bufptr->length() - length);
          }
        }
      } else if (payloadBytesRemaining_ > length) {
        payloadBytesRemaining_ -= length;
        // All of bufptr consumed so inner loop can be exited
        break;
      } else {
        // End of rpc payload is within bufptr
        data += payloadBytesRemaining_;
        length -= payloadBytesRemaining_;
        clone->trimEnd(length);
        addToPayload(std::move(clone));
        VLOG(2) << "Received complete RPC payload: " << std::endl
                << IOBufPrinter::printHexFolly(payload_.get(), true);
        if (processor_) {
          // Server side
          onThriftRequest();
          ++rpcsInitiated_;
        } else {
          // Client side
          onThriftResponse();
          ++rpcsCompleted_;
        }
        // Initialize for next payload
        sizeBytesRemaining_ = 4;
        payloadBytesRemaining_ = 0;
      }
    }
    if (clone) {
      addToPayload(std::move(clone));
    }
    bufptr = bufptr->next();
  } while (bufptr != contents.get());
}

void MultiRpcChannel::addToPayload(std::unique_ptr<IOBuf> buf) noexcept {
  if (payload_) {
    payload_->prependChain(std::move(buf));
  } else {
    payload_ = std::move(buf);
  }
}

void MultiRpcChannel::onH2StreamEnd() noexcept {
  VLOG(2) << "onH2StreamEnd";
  if (sizeBytesRemaining_ != 4) {
    LOG(ERROR) << "Received incomplete stream from peer";
  }
  if (processor_) {
    // Server side
    if (rpcsCompleted_ == rpcsInitiated_) {
      // All rpc have been completed.
      responseHandler_->sendEOM();
    }
    isClosed_ = true;
  } else {
    // Client side
    if (rpcsCompleted_ != rpcsInitiated_) {
      LOG(ERROR) << "Did not receive responses for all RPCs";
      performErrorCallbacks(ProxygenError::kErrorUnknown);
    }
    httpTransaction_ = nullptr;
  }
}

void MultiRpcChannel::onH2StreamClosed(ProxygenError error) noexcept {
  VLOG(2) << "onH2StreamClosed: " << error;
  if (httpTransaction_) {
    // Client side.  Terminate all RPCs.
    if (rpcsCompleted_ != rpcsInitiated_) {
      VLOG(2) << "Did not receive responses for all RPCs";
      performErrorCallbacks(error);
    }
  }
  httpTransaction_ = nullptr;
  H2Channel::onH2StreamClosed(error);
}

void MultiRpcChannel::performErrorCallbacks(ProxygenError error) noexcept {
  for (auto& callback : callbacks_) {
    if (callback) {
      std::unique_ptr<TTransportException> ex;
      if (error == ProxygenError::kErrorTimeout) {
        ex = std::make_unique<TTransportException>(
            TTransportException::TIMED_OUT);
      } else {
        // Some unknown error.
        VLOG(2) << "Network error before call completion";
        ex = std::make_unique<TTransportException>(
            TTransportException::NETWORK_ERROR);
      }
      // We assume that the connection is still valid.  If not, we
      // will get an error the next time we try to create a new
      // transaction and can deal with it then.
      // TODO: We could also try to understand the kind of error be
      // looking at "error" in more detail.
      ex->setOptions(TTransportException::CHANNEL_IS_VALID);
      auto evb = callback->getEventBase();
      evb->runInEventBaseThread([evbCallback = std::move(callback),
                                 evbEx = std::move(ex)]() mutable {
        evbCallback->onError(folly::make_exception_wrapper<TTransportException>(
            std::move(*evbEx)));
      });
      ++rpcsCompleted_;
    }
  }
}

void MultiRpcChannel::onThriftRequest() noexcept {
  if (!payload_) {
    // This should never happen - but we need to handle this case in
    // case of garbage from a peer.
    sendThriftErrorResponse("Proxygen stream has no body");
    return;
  }
  auto metadata = std::make_unique<RequestRpcMetadata>();
  CompactProtocolReader reader;
  reader.setInput(payload_.get());
  auto sz = metadata->read(&reader);
  EnvelopeUtil::removePrefix(payload_, sz);
  DCHECK(metadata->__isset.protocol);
  DCHECK(metadata->__isset.name);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);
  if (metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    // Send a dummy response for the oneway call since we need to do
    // this with HTTP2.
    auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
    responseMetadata->seqId = metadata->seqId;
    responseMetadata->__isset.seqId = true;
    auto payload = IOBuf::createCombined(0);
    sendThriftResponse(std::move(responseMetadata), std::move(payload));
  }
  processor_->onThriftRequest(
      std::move(metadata), std::move(payload_), shared_from_this());
}

void MultiRpcChannel::onThriftResponse() noexcept {
  h2ClientConnection_->setIsStable();
  auto metadata = std::make_unique<ResponseRpcMetadata>();
  CompactProtocolReader reader;
  reader.setInput(payload_.get());
  auto sz = metadata->read(&reader);
  EnvelopeUtil::removePrefix(payload_, sz);
  DCHECK(metadata->__isset.seqId);
  if (metadata->seqId < 0 ||
      metadata->seqId >= static_cast<int32_t>(callbacks_.size())) {
    LOG(ERROR) << "Sequence id " << metadata->seqId << " out of range";
    return;
  }
  auto& callback = callbacks_[metadata->seqId];
  if (!callback) {
    return;
  }
  auto evb = callback->getEventBase();
  evb->runInEventBaseThread([evbCallback = std::move(callback),
                             evbMetadata = std::move(metadata),
                             evbPayload = std::move(payload_)]() mutable {
    evbCallback->onThriftResponse(
        std::move(evbMetadata), std::move(evbPayload));
  });
}

void MultiRpcChannel::sendThriftErrorResponse(
    const string& message,
    ProtocolId protoId) noexcept {
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  // Set an arbitrary seqId.
  responseMetadata->seqId = 0;
  responseMetadata->__isset.seqId = true;
  responseMetadata->protocol = protoId;
  responseMetadata->__isset.protocol = true;
  TApplicationException tae(message);
  std::unique_ptr<IOBuf> payload;
  auto proto = static_cast<int16_t>(protoId);
  try {
    payload = serializeError(proto, tae, "process", 0);
  } catch (const TProtocolException& pe) {
    // Should never happen.  Log an error and return an empty
    // payload.
    LOG(ERROR) << "serializeError failed. type=" << pe.getType()
               << " what()=" << pe.what();
    payload = IOBuf::createCombined(0);
  }
  sendThriftResponse(std::move(responseMetadata), std::move(payload));
}

} // namespace thrift
} // namespace apache
