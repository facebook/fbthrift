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

#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <folly/fibers/Baton.h>
#include <folly/fibers/Fiber.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/client/ClientCallbackFlowable.h>

namespace apache {
namespace thrift {

void RequestChannel::sendRequestAsync(
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr callback,
    RpcKind kind,
    bool useClientStreamBridge) {
  auto eb = getEventBase();
  if (!eb || eb->isInEventBaseThread()) {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        sendRequestNoResponse(
            rpcOptions, std::move(buf), std::move(header), std::move(callback));
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        sendRequestResponse(
            rpcOptions, std::move(buf), std::move(header), std::move(callback));
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        if (useClientStreamBridge) {
          auto streamCallback = createStreamClientCallback(
              std::move(callback), rpcOptions.getChunkBufferSize());
          sendRequestStream(
              rpcOptions, std::move(buf), std::move(header), streamCallback);
        } else {
          rpcOptions.setChunkBufferSize(0);
          sendRequestStream(
              rpcOptions,
              std::move(buf),
              std::move(header),
              std::move(callback));
        }
        break;
      default:
        folly::assume_unreachable();
        break;
    }
  } else {
    eb->runInEventBaseThread([this,
                              rpcOptions,
                              buf = std::move(buf),
                              header = std::move(header),
                              callback = std::move(callback),
                              kind,
                              useClientStreamBridge]() mutable {
      switch (kind) {
        case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
          sendRequestNoResponse(
              rpcOptions,
              std::move(buf),
              std::move(header),
              std::move(callback));
          break;
        case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          sendRequestResponse(
              rpcOptions,
              std::move(buf),
              std::move(header),
              std::move(callback));
          break;
        case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
          if (useClientStreamBridge) {
            auto streamCallback = createStreamClientCallback(
                std::move(callback), rpcOptions.getChunkBufferSize());
            sendRequestStream(
                rpcOptions, std::move(buf), std::move(header), streamCallback);
          } else {
            rpcOptions.setChunkBufferSize(0);
            sendRequestStream(
                rpcOptions,
                std::move(buf),
                std::move(header),
                std::move(callback));
          }
          break;
        default:
          folly::assume_unreachable();
          break;
      }
    });
  }
}

void RequestChannel::sendRequestAsync(
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    SinkClientCallback* callback) {
  auto eb = getEventBase();
  if (!eb || eb->inRunningEventBaseThread()) {
    sendRequestSink(
        rpcOptions, std::move(buf), std::move(header), std::move(callback));
  } else {
    eb->runInEventBaseThread([this,
                              rpcOptions,
                              buf = std::move(buf),
                              header = std::move(header),
                              callback = std::move(callback)]() mutable {
      sendRequestSink(
          rpcOptions, std::move(buf), std::move(header), std::move(callback));
    });
  }
}

void RequestChannel::sendRequestStream(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  auto chunkTimeout = rpcOptions.getChunkTimeout();
  auto callback = std::make_unique<ThriftClientCallback>(
      nullptr, false, std::move(cb), std::chrono::milliseconds::zero());
  auto clientCallBackFlowable = std::make_shared<ClientCallbackFlowable>(
      std::move(callback), chunkTimeout);
  clientCallBackFlowable->init();
  StreamClientCallback* clientCallback = clientCallBackFlowable.get();
  sendRequestStream(
      rpcOptions, std::move(buf), std::move(header), clientCallback);
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

void RequestChannel::sendRequestSink(
    RpcOptions&,
    std::unique_ptr<folly::IOBuf>,
    std::shared_ptr<transport::THeader>,
    SinkClientCallback* clientCallback) {
  clientCallback->onFirstResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          "Current channel doesn't support sink RPC"));
}

} // namespace thrift
} // namespace apache
