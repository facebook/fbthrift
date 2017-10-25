/*
 * Copyright 2004-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/http2/common/MetadataInBodySingleRpcChannel.h>

#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/utils/Logging.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>

using apache::thrift::transport::TTransportException;
using folly::IOBufQueue;
using proxygen::HTTPHeaderCode;
using proxygen::HTTPMessage;
using proxygen::HTTPMethod;
using proxygen::IOBufPrinter;
using std::string;

DEFINE_bool(
    thrift_cpp2_metadata_in_body,
    false,
    "Serialize metadata in request body");

namespace apache {
namespace thrift {

MetadataInBodySingleRpcChannel::MetadataInBodySingleRpcChannel(
    proxygen::ResponseHandler* toHttp2,
    ThriftProcessor* processor)
    : SingleRpcChannel(toHttp2, processor) {}

MetadataInBodySingleRpcChannel::MetadataInBodySingleRpcChannel(
    H2ClientConnection* toHttp2,
    const string& httpHost,
    const string& httpUrl)
    : SingleRpcChannel(toHttp2, httpHost, httpUrl) {}

void MetadataInBodySingleRpcChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(evb_->isInEventBaseThread());
  DCHECK(metadata);
  DCHECK(metadata->__isset.protocol);
  DCHECK(metadata->__isset.name);
  DCHECK(metadata->__isset.kind);
  DCHECK(
      metadata->kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
      metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE);
  DCHECK(payload);
  DCHECK(callback);
  VLOG(2) << "sendThriftRequest:" << std::endl
          << IOBufPrinter::printHexFolly(payload.get(), true);
  auto callbackEvb = callback->getEventBase();
  try {
    httpTransaction_ = h2ClientConnection_->newTransaction(this);
  } catch (TTransportException& te) {
    callbackEvb->runInEventBaseThread([evbCallback = std::move(callback),
                                       evbTe = std::move(te)]() mutable {
      evbCallback->onError(
          folly::make_exception_wrapper<TTransportException>(std::move(evbTe)));
    });
    return;
  }
  HTTPMessage msg;
  msg.setMethod(HTTPMethod::POST);
  msg.setURL(httpUrl_);
  if (!httpHost_.empty()) {
    auto& msgHeaders = msg.getHeaders();
    msgHeaders.set(HTTPHeaderCode::HTTP_HEADER_HOST, httpHost_);
  }
  if (metadata->__isset.clientTimeoutMs) {
    DCHECK(metadata->clientTimeoutMs > 0);
    httpTransaction_->setIdleTimeout(
        std::chrono::milliseconds(metadata->clientTimeoutMs));
  }
  auto requestMetadata = std::make_unique<IOBufQueue>();
  CompactProtocolWriter writer;
  writer.setOutput(requestMetadata.get());
  metadata->write(&writer);
  httpTransaction_->sendHeaders(msg);
  httpTransaction_->sendBody(requestMetadata->move());

  httpTransaction_->sendBody(std::move(payload));
  httpTransaction_->sendEOM();
  // For oneway calls, we move "callback" to "callbackEvb" since we do
  // not require the callback any more.  For twoway calls, we move
  // "callback" to "callback_" and use a raw pointer to call
  // "onThriftRequestSent()".  This is safe because "callback_" will
  // be eventually moved to this same thread to call either
  // "onThriftResponse()" or "onThriftError()".
  if (metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    callbackEvb->runInEventBaseThread(
        [cb = std::move(callback)]() mutable { cb->onThriftRequestSent(); });
  } else {
    callback_ = std::move(callback);
    callbackEvb->runInEventBaseThread(
        [cb = callback_.get()]() mutable { cb->onThriftRequestSent(); });
  }
  receivedThriftRPC_ = true;
}

void MetadataInBodySingleRpcChannel::onThriftRequest() noexcept {
  if (!contents_) {
    sendThriftErrorResponse("Proxygen stream has no body");
    return;
  }
  auto metadata = std::make_unique<RequestRpcMetadata>();

  CompactProtocolReader reader;
  reader.setInput(contents_.get());
  auto sz = metadata->read(&reader);
  contents_->trimStart(sz);

  if (metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    // Send a dummy response for the oneway call since we need to do
    // this with HTTP2.
    auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
    auto payload = IOBuf::createCombined(0);
    sendThriftResponse(std::move(responseMetadata), std::move(payload));
    receivedThriftRPC_ = true;
  }
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
  processor_->onThriftRequest(
      std::move(metadata), std::move(contents_), shared_from_this());
}

} // namespace thrift
} // namespace apache
