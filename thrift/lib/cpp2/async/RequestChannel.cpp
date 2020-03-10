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
#include <thrift/lib/cpp2/async/StreamCallbacks.h>

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache {
namespace thrift {

namespace {

template <typename ProtocolWriter>
std::unique_ptr<folly::IOBuf> serializeRequest(
    folly::StringPiece methodName,
    std::unique_ptr<folly::IOBuf> buf) {
  ProtocolWriter writer;
  auto messageBeginSizeUpperBound = writer.serializedMessageSize(methodName);
  folly::IOBufQueue queue;

  // If possible, serialize header into the headeroom of buf.
  if (!buf->isChained() && buf->headroom() >= messageBeginSizeUpperBound &&
      !buf->isSharedOne()) {
    // Store previous state of the buffer pointers and rewind it.
    auto startBuffer = buf->buffer();
    auto start = buf->data();
    auto origLen = buf->length();
    buf->trimEnd(origLen);
    buf->retreat(start - startBuffer);

    queue.append(std::move(buf), false);
    writer.setOutput(&queue);
    writer.writeMessageBegin(methodName, T_CALL, 0);

    // Move the new data to come right before the old data and restore the
    // old tail pointer.
    buf = queue.move();
    buf->advance(start - buf->tail());
    buf->append(origLen);

    return buf;
  } else {
    auto messageBeginBuf = folly::IOBuf::create(messageBeginSizeUpperBound);
    queue.append(std::move(messageBeginBuf));
    writer.setOutput(&queue);
    writer.writeMessageBegin(methodName, T_CALL, 0);
    queue.append(std::move(buf));
    return queue.move();
  }

  // We skip writeMessageEnd because for both Binary and Compact it's a noop.
}

std::unique_ptr<folly::IOBuf> serializeRequest(
    folly::StringPiece methodName,
    SerializedRequest&& request,
    uint16_t protocolId) {
  switch (protocolId) {
    case protocol::T_BINARY_PROTOCOL:
      return serializeRequest<BinaryProtocolWriter>(
          methodName, std::move(request.buffer));
    case protocol::T_COMPACT_PROTOCOL:
      return serializeRequest<CompactProtocolWriter>(
          methodName, std::move(request.buffer));
    default:
      LOG(FATAL) << "Unsupported protocolId: " << protocolId;
  };
}
} // namespace

void RequestChannel::sendRequestAsync(
    apache::thrift::RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr callback,
    RpcKind kind) {
  auto eb = getEventBase();
  if (!eb || eb->isInEventBaseThread()) {
    switch (kind) {
      case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        sendRequestNoResponse(
            rpcOptions,
            methodName,
            std::move(request),
            std::move(header),
            std::move(callback));
        break;
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
        sendRequestResponse(
            rpcOptions,
            methodName,
            std::move(request),
            std::move(header),
            std::move(callback));
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
        sendRequestStream(
            rpcOptions,
            methodName,
            std::move(request),
            std::move(header),
            createStreamClientCallback(
                std::move(callback), rpcOptions.getChunkBufferSize()));
        break;
      default:
        folly::assume_unreachable();
        break;
    }
  } else {
    eb->runInEventBaseThread([this,
                              rpcOptions,
                              methodNameStr = std::string(methodName),
                              request = std::move(request),
                              header = std::move(header),
                              callback = std::move(callback),
                              kind]() mutable {
      switch (kind) {
        case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
          sendRequestNoResponse(
              rpcOptions,
              methodNameStr.c_str(),
              std::move(request),
              std::move(header),
              std::move(callback));
          break;
        case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          sendRequestResponse(
              rpcOptions,
              methodNameStr.c_str(),
              std::move(request),
              std::move(header),
              std::move(callback));
          break;
        case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
          sendRequestStream(
              rpcOptions,
              methodNameStr.c_str(),
              std::move(request),
              std::move(header),
              createStreamClientCallback(
                  std::move(callback), rpcOptions.getChunkBufferSize()));
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
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    SinkClientCallback* callback) {
  auto eb = getEventBase();
  if (!eb || eb->inRunningEventBaseThread()) {
    sendRequestSink(
        rpcOptions,
        methodName,
        std::move(request),
        std::move(header),
        std::move(callback));
  } else {
    eb->runInEventBaseThread([this,
                              rpcOptions,
                              methodNameStr = std::string(methodName),
                              request = std::move(request),
                              header = std::move(header),
                              callback = std::move(callback)]() mutable {
      sendRequestSink(
          rpcOptions,
          methodNameStr.c_str(),
          std::move(request),
          std::move(header),
          std::move(callback));
    });
  }
}

void RequestChannel::sendRequestStream(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  StreamClientCallback* clientCallback = createStreamClientCallback(
      std::move(cb), rpcOptions.getChunkBufferSize());
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

void RequestChannel::sendRequestResponse(
    RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr clientCallback) {
  sendRequestResponse(
      rpcOptions,
      serializeRequest(methodName, std::move(request), getProtocolId()),
      std::move(header),
      std::move(clientCallback));
}

void RequestChannel::sendRequestNoResponse(
    RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr clientCallback) {
  sendRequestNoResponse(
      rpcOptions,
      serializeRequest(methodName, std::move(request), getProtocolId()),
      std::move(header),
      std::move(clientCallback));
}

void RequestChannel::sendRequestStream(
    RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr clientCallback) {
  sendRequestStream(
      rpcOptions,
      serializeRequest(methodName, std::move(request), getProtocolId()),
      std::move(header),
      std::move(clientCallback));
}

void RequestChannel::sendRequestStream(
    RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    StreamClientCallback* clientCallback) {
  sendRequestStream(
      rpcOptions,
      serializeRequest(methodName, std::move(request), getProtocolId()),
      std::move(header),
      clientCallback);
}

void RequestChannel::sendRequestSink(
    RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* clientCallback) {
  sendRequestSink(
      rpcOptions,
      serializeRequest(methodName, std::move(request), getProtocolId()),
      std::move(header),
      clientCallback);
}

} // namespace thrift
} // namespace apache
