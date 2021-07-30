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

#include <thrift/lib/cpp2/server/ThriftProcessor.h>

#include <string>

#include <folly/Overload.h>

#include <fmt/core.h>
#include <glog/logging.h>

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/AsyncProcessorHelper.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/util/Checksum.h>

namespace apache {
namespace thrift {

using apache::thrift::concurrency::ThreadManager;
using folly::IOBuf;

ThriftProcessor::ThriftProcessor(ThriftServer& server) : server_(server) {}

void ThriftProcessor::onThriftRequest(
    RequestRpcMetadata&& metadata,
    std::unique_ptr<IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel,
    std::unique_ptr<Cpp2ConnContext> connContext) noexcept {
  DCHECK(payload);
  DCHECK(channel);

  auto& processorFactory = server_.getDecoratedProcessorFactory();
  if (processor_ == nullptr) {
    processor_ = processorFactory.getProcessor();
  }

  auto worker = connContext->getWorker();
  worker->getEventBase()->dcheckIsInEventBaseThread();

  bool invalidMetadata =
      !(metadata.protocol_ref() && metadata.name_ref() && metadata.kind_ref());

  bool invalidChecksum = metadata.crc32c_ref() &&
      *metadata.crc32c_ref() != apache::thrift::checksum::crc32c(*payload);

  auto request = std::make_unique<ThriftRequest>(
      server_, channel, std::move(metadata), std::move(connContext));

  auto* evb = channel->getEventBase();
  if (UNLIKELY(invalidMetadata)) {
    LOG(ERROR) << "Invalid metadata object";
    evb->runInEventBaseThread([request = std::move(request)]() {
      request->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::UNSUPPORTED_CLIENT_TYPE,
              "invalid metadata object"),
          "corrupted metadata");
    });
    return;
  }
  if (UNLIKELY(invalidChecksum)) {
    LOG(ERROR) << "Invalid checksum";
    evb->runInEventBaseThread([request = std::move(request)]() {
      request->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::CHECKSUM_MISMATCH, "checksum mismatch"),
          "corrupted request");
    });
    return;
  }

  const auto& serviceMetadata = worker->getMetadataForService(processorFactory);
  using PerServiceMetadata = Cpp2Worker::PerServiceMetadata;
  const PerServiceMetadata::FindMethodResult methodMetadataResult =
      serviceMetadata.findMethod(request->getMethodName());

  auto baseReqCtx =
      serviceMetadata.getBaseContextForRequest(methodMetadataResult);
  auto reqCtx = baseReqCtx ? folly::RequestContext::copyAsChild(*baseReqCtx)
                           : std::make_shared<folly::RequestContext>();
  folly::RequestContextScopeGuard rctx(reqCtx);

  auto protoId = request->getProtoId();
  auto reqContext = request->getRequestContext();

  folly::variant_match(
      methodMetadataResult,
      [&](PerServiceMetadata::MetadataNotImplemented) {
        // The AsyncProcessorFactory does not implement createMethodMetadata
        // so we need to fallback to processSerializedCompressedRequest.
        processor_->processSerializedCompressedRequest(
            std::move(request),
            SerializedCompressedRequest(std::move(payload)),
            protoId,
            reqContext,
            evb,
            server_.getThreadManager().get());
      },
      [&](PerServiceMetadata::MetadataNotFound) {
        std::string_view methodName = request->getMethodName();
        AsyncProcessorHelper::sendUnknownMethodError(
            std::move(request), methodName);
      },
      [&](const PerServiceMetadata::MetadataFound& found) {
        processor_->processSerializedCompressedRequestWithMetadata(
            std::move(request),
            SerializedCompressedRequest(std::move(payload)),
            found.metadata,
            protoId,
            reqContext,
            evb,
            server_.getThreadManager().get());
      });
}
} // namespace thrift
} // namespace apache
