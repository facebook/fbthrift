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

#include <thrift/lib/cpp2/transport/rsocket/client/StreamThriftClient.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

#include <yarpl/single/Singles.h>

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncTransport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using apache::thrift::transport::THeader;
using apache::thrift::transport::TTransportException;
using folly::EventBase;
using folly::IOBuf;
using folly::RequestContext;

StreamThriftClient::StreamThriftClient(
    std::shared_ptr<ClientConnectionIf> connection,
    folly::EventBase* callbackEvb)
    : ThriftClient(std::move(connection), callbackEvb) {}

uint32_t StreamThriftClient::sendRequestSync(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  auto connectionEvb = connection_->getEventBase();
  DCHECK(!connectionEvb->inRunningEventBaseThread());
  auto& cbr = *cb;
  // Only the functions which are stream enabled will create an instance
  // of this callback
  if (typeid(StreamRequestCallback) != typeid(cbr)) {
    return ThriftClient::sendRequestSync(
        rpcOptions,
        std::move(cb),
        std::move(ctx),
        std::move(buf),
        std::move(header));
  }

  std::unique_ptr<StreamRequestCallback> scb(
      static_cast<StreamRequestCallback*>(cb.release()));

  auto replyFuture = scb->getReplyFuture();

  auto result = sendStreamRequestHelper(
      rpcOptions,
      std::move(scb),
      std::move(ctx),
      std::move(buf),
      std::move(header),
      connectionEvb);

  replyFuture.wait();
  return result;
}

uint32_t StreamThriftClient::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  auto& cbr = *cb;
  if (typeid(StreamRequestCallback) == typeid(cbr)) {
    std::unique_ptr<StreamRequestCallback> scb(
        static_cast<StreamRequestCallback*>(cb.release()));

    return sendStreamRequestHelper(
        rpcOptions,
        std::move(scb),
        std::move(ctx),
        std::move(buf),
        std::move(header),
        connection_->getEventBase());
  }
  return ThriftClient::sendRequest(
      rpcOptions,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
}

uint32_t StreamThriftClient::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  auto& cbr = *cb;
  if (typeid(StreamRequestCallback) == typeid(cbr)) {
    std::unique_ptr<StreamRequestCallback> scb(
        static_cast<StreamRequestCallback*>(cb.release()));

    return sendStreamRequestHelper(
        rpcOptions,
        std::move(scb),
        std::move(ctx),
        std::move(buf),
        std::move(header),
        connection_->getEventBase());
  }
  return ThriftClient::sendOnewayRequest(
      rpcOptions,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
}

uint32_t StreamThriftClient::sendStreamRequestHelper(
    RpcOptions& rpcOptions,
    std::unique_ptr<StreamRequestCallback> cb,
    std::unique_ptr<ContextStack>,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header,
    EventBase*) {
  DestructorGuard dg(this);
  cb->context_ = RequestContext::saveContext();
  auto metadata = createRequestRpcMetadata(
      rpcOptions,
      cb->kind_,
      static_cast<apache::thrift::ProtocolId>(protocolId_),
      header.get());

  auto conn = connection_;
  connection_->getEventBase()->runInEventBaseThread(
      [conn = std::move(conn),
       metadata = std::move(metadata),
       buf = std::move(buf),
       cb = std::move(cb)]() mutable {
        std::shared_ptr<RSClientThriftChannel> channel{nullptr};
        try {
          channel = std::dynamic_pointer_cast<RSClientThriftChannel>(
              conn->getChannel(metadata.get()));
          if (!channel) {
            throw TTransportException("invalid channel type");
          }
        } catch (const TTransportException& te) {
          // Give the error as the stream!
          cb->getOutput()->onSubscribe(yarpl::flowable::Subscription::empty());
          cb->getOutput()->onError(te);
          return;
        }

        channel->sendStreamThriftRequest(
            std::move(metadata), std::move(buf), std::move(cb));
      });
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

} // namespace thrift
} // namespace apache
