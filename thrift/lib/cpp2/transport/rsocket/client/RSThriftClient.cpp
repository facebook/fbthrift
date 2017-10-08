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

#include <thrift/lib/cpp2/transport/rsocket/client/RSThriftClient.h>

#include <folly/Baton.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncTransport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using apache::thrift::transport::THeader;
using apache::thrift::transport::TTransportException;
using folly::EventBase;
using folly::IOBuf;
using folly::RequestContext;

RSThriftClient::RSThriftClient(
    std::shared_ptr<ClientConnectionIf> connection,
    folly::EventBase* callbackEvb)
    : ThriftClient(std::move(connection), callbackEvb) {}

RSThriftClient::RSThriftClient(std::shared_ptr<ClientConnectionIf> connection)
    : ThriftClient(std::move(connection)) {}

uint32_t RSThriftClient::sendRequestSync(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  EventBase* connectionEvb = connection_->getEventBase();
  DCHECK(!connectionEvb->inRunningEventBaseThread());
  int result;

  auto& _cbr = *cb;
  if (typeid(StreamingRequestCallback) == typeid(_cbr)) {
    // Only the functions which are stream enabled will create an instance
    // of this callback
    std::unique_ptr<StreamingRequestCallback> scb(
        static_cast<StreamingRequestCallback*>(cb.release()));

    auto replyFuture = scb->getReplyFuture();

    result = sendStreamRequestHelper(
        rpcOptions,
        std::move(scb),
        std::move(ctx),
        std::move(buf),
        std::move(header),
        connectionEvb);

    replyFuture.wait();
  } else {
    result = ThriftClient::sendRequestSync(
        rpcOptions,
        std::move(cb),
        std::move(ctx),
        std::move(buf),
        std::move(header));
  }

  return result;
}

uint32_t RSThriftClient::sendStreamRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<StreamingRequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  return sendStreamRequestHelper(
      rpcOptions,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      callbackEvb_);
}

uint32_t RSThriftClient::sendStreamRequestHelper(
    RpcOptions& rpcOptions,
    std::unique_ptr<StreamingRequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header,
    EventBase* callbackEvb) {
  DestructorGuard dg(this);

  // Just connect the channel to the callback, so it will be connected
  // to the executed method
  std::shared_ptr<ThriftChannelIf> channel;
  try {
    channel = connection_->getChannel();
  } catch (TTransportException& te) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(std::move(te)),
        std::move(ctx),
        isSecurityActive()));
    return 0;
  }

  auto result = sendStreamRequestHelper(
      std::move(channel),
      rpcOptions,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      callbackEvb);

  return result;
}

uint32_t RSThriftClient::sendStreamRequestHelper(
    std::shared_ptr<ThriftChannelIf> channel,
    RpcOptions& rpcOptions,
    std::unique_ptr<StreamingRequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header,
    EventBase* callbackEvb) {
  cb->context_ = RequestContext::saveContext();

  auto metadata = std::make_unique<RequestRpcMetadata>();
  metadata->protocol = static_cast<ProtocolId>(protocolId_);
  metadata->__isset.protocol = true;
  metadata->kind = cb->kind_;
  metadata->__isset.kind = true;
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    metadata->clientTimeoutMs = rpcOptions.getTimeout().count();
    metadata->__isset.clientTimeoutMs = true;
  }
  if (rpcOptions.getQueueTimeout() > std::chrono::milliseconds(0)) {
    metadata->queueTimeoutMs = rpcOptions.getTimeout().count();
    metadata->__isset.queueTimeoutMs = true;
  }
  if (rpcOptions.getPriority() < concurrency::N_PRIORITIES) {
    metadata->priority = static_cast<RpcPriority>(rpcOptions.getPriority());
    metadata->__isset.priority = true;
  }
  metadata->otherMetadata = header->releaseWriteHeaders();
  auto* eh = header->getExtraWriteHeaders();
  if (eh) {
    metadata->otherMetadata.insert(eh->begin(), eh->end());
  }
  auto& pwh = getPersistentWriteHeaders();
  metadata->otherMetadata.insert(pwh.begin(), pwh.end());
  if (!metadata->otherMetadata.empty()) {
    metadata->__isset.otherMetadata = true;
  }

  if (cb->kind_ == RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE ||
      cb->kind_ == RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE) {
    channel->setInput(0, cb->getChannelInput());
  }

  bool singleResponse = cb->kind_ == RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE;
  std::unique_ptr<ThriftClientCallback> callback;
  if (singleResponse) {
    callback = std::make_unique<ThriftClientCallback>(
        callbackEvb,
        std::move(cb),
        std::move(ctx),
        isSecurityActive(),
        protocolId_);
  }
  connection_->getEventBase()->runInEventBaseThread(
      [evbChannel = channel,
       evbMetadata = std::move(metadata),
       evbBuf = std::move(buf),
       evbCallback = std::move(callback),
       evbCb = std::move(cb),
       singleResponse]() mutable {
        evbChannel->sendThriftRequest(
            std::move(evbMetadata), std::move(evbBuf), std::move(evbCallback));
        if (!singleResponse) {
          evbCb->setChannelOutput(evbChannel->getOutput(0));
          evbCb->requestSent();
        }
      });
  return 0;
}

} // namespace thrift
} // namespace apache
