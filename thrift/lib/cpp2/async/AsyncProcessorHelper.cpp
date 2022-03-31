/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/async/AsyncProcessorHelper.h>

#include <fmt/core.h>

#include <thrift/lib/cpp/TApplicationException.h>

namespace apache::thrift {

/* static */ void AsyncProcessorHelper::sendUnknownMethodError(
    ResponseChannelRequest::UniquePtr request, std::string_view methodName) {
  auto message = fmt::format("Method name {} not found", methodName);
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::UNKNOWN_METHOD, std::move(message)),
      kMethodUnknownErrorCode);
}

/* static */ void AsyncProcessorHelper::executeRequest(
    ServerRequest&& serverRequest) {
  // Since this request was queued, reset the processBegin
  // time to the actual start time, and not the queue time.
  auto ctx = serverRequest.requestContext();
  if (ctx->getTimestamps().getSamplingStatus().isEnabled()) {
    ctx->getTimestamps().processBegin = std::chrono::steady_clock::now();
  }

  auto ap = detail::ServerRequestHelper::asyncProcessor(serverRequest);
  AsyncProcessorFactory::MethodMetadata const& metadata =
      *serverRequest.methodMetadata();
  folly::RequestContextScopeGuard rctx(serverRequest.follyRequestContext());
  if (auto requestInfo = serverRequest.requestInfo()) {
    // Currently we will only execute requests with a valid requestInfo. We will
    // have to modify/relax this restriction to handle wildcard services.
    try {
      std::unique_ptr<ContextStack> ctxStack(ap->getContextStack(
          ap->getServiceName(),
          requestInfo->functionName_deprecated,
          serverRequest.requestContext()));
      if (ctxStack) {
        ctxStack->preRead();
        SerializedMessage smsg;
        smsg.protocolType =
            detail::ServerRequestHelper::protocol(serverRequest);
        // This provides the compressed data which is not the previous behavior.
        // TODO: T112295873
        smsg.buffer =
            detail::ServerRequestHelper::compressedRequest(serverRequest)
                .compressedBuffer();
        smsg.methodName = requestInfo->functionName_deprecated;
        ctxStack->onReadData(smsg);
        // Provide the length of the compressed data
        // TODO: T112295873
        ctxStack->postRead(
            nullptr,
            detail::ServerRequestHelper::compressedRequest(serverRequest)
                .compressedBuffer()
                ->computeChainDataLength());
        detail::ServerRequestHelper::setContextStack(
            serverRequest, std::move(ctxStack));
      }
      ap->executeRequest(std::move(serverRequest), metadata);
    } catch (std::exception& ex) {
      // Temporary code - just ensure that a failure produces an error.
      // TODO: T113039894
      folly::exception_wrapper ew(std::current_exception(), ex);
      auto eb = detail::ServerRequestHelper::eventBase(serverRequest);
      auto req = detail::ServerRequestHelper::request(std::move(serverRequest));
      eb->runInEventBaseThread([request = std::move(req)]() {
        request->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(
                TApplicationException::INTERNAL_ERROR,
                "AsyncProcessorHelper::executeRequest - resource pools mode"),
            kUnknownErrorCode);
      });
      return;
    }
  } else {
    auto eb = detail::ServerRequestHelper::eventBase(serverRequest);
    eb->runInEventBaseThread(
        [serverRequest = std::move(serverRequest)]() mutable {
          auto methodName = serverRequest.requestContext()->getMethodName();
          sendUnknownMethodError(
              detail::ServerRequestHelper::request(std::move(serverRequest)),
              methodName);
        });
  }
}

/* static */ SelectPoolResult AsyncProcessorHelper::selectResourcePool(
    const ServerRequest& request,
    const AsyncProcessorFactory::MethodMetadata&) {
  if (auto requestInfo = request.requestInfo()) {
    if (requestInfo->isSync) {
      return std::ref(ResourcePoolHandle::defaultSync());
    } else {
      return std::ref(ResourcePoolHandle::defaultAsync());
    }
  }
  return std::ref(ResourcePoolHandle::defaultAsync());
}

} // namespace apache::thrift
