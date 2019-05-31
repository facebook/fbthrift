/*
 * Copyright 2014-present Facebook, Inc.
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
#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <folly/fibers/Baton.h>
#include <folly/fibers/Fiber.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/client/ClientCallbackFlowable.h>

namespace apache {
namespace thrift {

void RequestChannel::sendRequestAsync(
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RpcKind kind) {
  auto eb = getEventBase();
  if (!eb || eb->isInEventBaseThread()) {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        // Calling asyncComplete before sending because
        // sendOnewayRequest moves from ctx and clears it.
        ctx->asyncComplete();
        sendOnewayRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        sendRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        sendStreamRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(buf),
            std::move(header));
        break;
      default:
        folly::assume_unreachable();
        break;
    }

  } else {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          // Calling asyncComplete before sending because
          // sendOnewayRequest moves from ctx and clears it.
          ctx->asyncComplete();
          sendOnewayRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          sendRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        eb->runInEventBaseThread([this,
                                  rpcOptions,
                                  callback = std::move(callback),
                                  ctx = std::move(ctx),
                                  buf = std::move(buf),
                                  header = std::move(header)]() mutable {
          sendStreamRequest(
              rpcOptions,
              std::move(callback),
              std::move(ctx),
              std::move(buf),
              std::move(header));
        });
        break;
      default:
        folly::assume_unreachable();
        break;
    }
  }
}

uint32_t RequestChannel::sendStreamRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  DestructorGuard dg(this);
  cb->context_ = folly::RequestContext::saveContext();
  auto chunkTimeout = rpcOptions.getChunkTimeout();
  auto callback = std::make_unique<ThriftClientCallback>(
      nullptr,
      std::move(cb),
      std::move(ctx),
      getProtocolId(),
      std::chrono::milliseconds::zero());
  auto clientCallBackFlowable = std::make_shared<ClientCallbackFlowable>(
      std::move(callback), chunkTimeout);
  clientCallBackFlowable->init();
  StreamClientCallback* clientCallback = clientCallBackFlowable.get();
  sendRequestStream(
      rpcOptions, std::move(buf), std::move(header), clientCallback);
  return 0;
}

void RequestChannel::sendRequestStream(
    RpcOptions&,
    std::unique_ptr<folly::IOBuf>,
    std::shared_ptr<transport::THeader>,
    StreamClientCallback* clientCallback) {
  clientCallback->onFirstResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          "Current channel doesn't support stream RPC"));
}

} // namespace thrift
} // namespace apache
