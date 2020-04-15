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

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache {
namespace thrift {

template <typename F>
static void maybeRunInEb(folly::EventBase* eb, F func) {
  if (!eb || eb->isInEventBaseThread()) {
    func();
  } else {
    eb->runInEventBaseThread(std::forward<F>(func));
  }
}

template <>
void RequestChannel::sendRequestAsync<RpcKind::SINGLE_REQUEST_NO_RESPONSE>(
    const apache::thrift::RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr callback) {
  maybeRunInEb(
      getEventBase(),
      [this,
       rpcOptions,
       methodNameStr = std::string(methodName),
       request = std::move(request),
       header = std::move(header),
       callback = std::move(callback)]() mutable {
        sendRequestNoResponse(
            rpcOptions,
            methodNameStr.c_str(),
            std::move(request),
            std::move(header),
            std::move(callback));
      });
}
template <>
void RequestChannel::sendRequestAsync<RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(
    const apache::thrift::RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr callback) {
  maybeRunInEb(
      getEventBase(),
      [this,
       rpcOptions,
       methodNameStr = std::string(methodName),
       request = std::move(request),
       header = std::move(header),
       callback = std::move(callback)]() mutable {
        sendRequestResponse(
            rpcOptions,
            methodNameStr.c_str(),
            std::move(request),
            std::move(header),
            std::move(callback));
      });
}
template <>
void RequestChannel::sendRequestAsync<
    RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE>(
    const apache::thrift::RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    StreamClientCallback* callback) {
  maybeRunInEb(
      getEventBase(),
      [this,
       rpcOptions,
       methodNameStr = std::string(methodName),
       request = std::move(request),
       header = std::move(header),
       callback = std::move(callback)]() mutable {
        sendRequestStream(
            rpcOptions,
            methodNameStr.c_str(),
            std::move(request),
            std::move(header),
            callback);
      });
}
template <>
void RequestChannel::sendRequestAsync<RpcKind::SINK>(
    const apache::thrift::RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    SinkClientCallback* callback) {
  maybeRunInEb(
      getEventBase(),
      [this,
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
            callback);
      });
}

void RequestChannel::sendRequestResponse(
    const RpcOptions& rpcOptions,
    LegacySerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr clientCallback) {
  if (auto envelopeAndRequest =
          EnvelopeUtil::stripRequestEnvelope(std::move(request.buffer))) {
    return sendRequestResponse(
        rpcOptions,
        envelopeAndRequest->first.methodName,
        SerializedRequest(std::move(envelopeAndRequest->second)),
        std::move(header),
        std::move(clientCallback));
  }
  clientCallback.release()->onResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::CORRUPTED_DATA,
          "Unexpected problem stripping envelope"));
}

void RequestChannel::sendRequestNoResponse(
    const RpcOptions& rpcOptions,
    LegacySerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestClientCallback::Ptr clientCallback) {
  if (auto envelopeAndRequest =
          EnvelopeUtil::stripRequestEnvelope(std::move(request.buffer))) {
    return sendRequestNoResponse(
        rpcOptions,
        envelopeAndRequest->first.methodName,
        SerializedRequest(std::move(envelopeAndRequest->second)),
        std::move(header),
        std::move(clientCallback));
  }
  clientCallback.release()->onResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::CORRUPTED_DATA,
          "Unexpected problem stripping envelope"));
}

void RequestChannel::sendRequestStream(
    const RpcOptions&,
    folly::StringPiece,
    SerializedRequest&&,
    std::shared_ptr<transport::THeader>,
    StreamClientCallback* clientCallback) {
  clientCallback->onFirstResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          "This channel doesn't support stream RPC"));
}

void RequestChannel::sendRequestSink(
    const RpcOptions&,
    folly::StringPiece,
    SerializedRequest&&,
    std::shared_ptr<transport::THeader>,
    SinkClientCallback* clientCallback) {
  clientCallback->onFirstResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          "This channel doesn't support sink RPC"));
}

} // namespace thrift
} // namespace apache
