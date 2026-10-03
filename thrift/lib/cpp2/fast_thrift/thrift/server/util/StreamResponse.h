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

#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/ThriftServerAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/StreamResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponseMetadata.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/ProducerPipeline.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/StreamElementEncoder.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/StreamFactory.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::fast_thrift::thrift {

/**
 * Build the app->mux message that opens a producing stream for a stream RPC:
 * binds the framework's element encoder to the handler-returned
 * `StreamFactory<T>` (yielding the `ProducerPipeline::ConfigFunc` the mux runs)
 * and pairs it with the stream's initial response chunk.
 *
 * `initialResponseData` is the serialized first-response presult the stream's
 * first PAYLOAD carries as its data — the client decodes it before switching to
 * stream elements (the `ResponseRpcMetadata` rides the frame metadata). It is a
 * single STOP byte for a bare `stream<T>` and the serialized `R` presult for a
 * `R, stream<T>`; this builder is agnostic to which, so the caller owns the
 * response-vs-void distinction.
 */
template <typename T>
inline ThriftServerResponseMessage makeStreamOpenMessage(
    uint32_t streamId,
    std::unique_ptr<folly::IOBuf> initialResponseData,
    stream::StreamFactory<T>&& factory,
    stream::StreamElementEncoder<T> encoder) {
  auto metadata = std::make_unique<apache::thrift::ResponseRpcMetadata>();
  fillSuccessResponseMetadata(*metadata);
  return ThriftServerResponseMessage{
      .payload = ThriftServerStreamOpenPayload{
          .initialResponse =
              ThriftStreamInitialResponsePayload{
                  .data = std::move(initialResponseData),
                  .metadata = std::move(metadata),
                  .streamId = streamId,
                  .complete = false,
                  .next = true},
          .configFunc = std::make_unique<stream::ProducerPipeline::ConfigFunc>(
              std::move(factory).bind(std::move(encoder)))}};
}

namespace detail {

// A bare `stream<T>` has no initial-response value, but the classic client
// still reads a presult before the first stream element: an empty struct, a
// single STOP byte identical in Binary and Compact. Emitting that (rather than
// null data) keeps the client's presult reader from underflowing.
inline std::unique_ptr<folly::IOBuf> makeEmptyPresultData() {
  static constexpr uint8_t kEmptyPresultStop = 0;
  return folly::IOBuf::copyBuffer(&kEmptyPresultStop, 1);
}

/**
 * Codegen-targeted `FastHandlerCallback<StreamFactory<T>>::ResultFn`: binds the
 * protocol-specialized element encoder (whose value/error thunks codegen
 * supplies as `EncodeValue` / `EncodeError`) to the handler-returned factory,
 * builds the stream-open message, and hands it to the adapter's single
 * `writeResponse` entry point. Mirrors `FastHandlerCallback<T>::writeSuccess`.
 */
template <
    typename T,
    stream::Payload (*EncodeValue)(T&&) noexcept,
    stream::Error (*EncodeError)(folly::exception_wrapper&&) noexcept>
void writeStreamOpen(
    ThriftServerAppAdapter* a,
    uint32_t sid,
    ThriftRequestContextPtr requestContext,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard,
    stream::StreamFactory<T>&& factory) noexcept {
  auto message = makeStreamOpenMessage<T>(
      sid,
      makeEmptyPresultData(),
      std::move(factory),
      stream::StreamElementEncoder<T>{EncodeValue, EncodeError});
  message.requestContext = std::move(requestContext);
  a->writeResponse(std::move(message), std::move(adapterGuard));
}

/**
 * `FastHandlerCallback<ResponseAndStreamFactory<R, T>>::ResultFn` for a
 * `R, stream<T>` RPC: mirrors `writeStreamOpen`, but first serializes the
 * handler-returned initial response `R` into the first PAYLOAD's presult data
 * via the protocol-specialized `EncodeResponse` thunk (codegen reuses the unary
 * presult serializer specialized on the method's `InitialResponsePResultType`),
 * so the classic client reads a real response value before the stream elements.
 */
template <
    typename R,
    typename T,
    std::unique_ptr<folly::IOBuf> (*EncodeResponse)(R&&) noexcept,
    stream::Payload (*EncodeValue)(T&&) noexcept,
    stream::Error (*EncodeError)(folly::exception_wrapper&&) noexcept>
void writeResponseStreamOpen(
    ThriftServerAppAdapter* a,
    uint32_t sid,
    ThriftRequestContextPtr requestContext,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard,
    stream::ResponseAndStreamFactory<R, T>&& value) noexcept {
  auto message = makeStreamOpenMessage<T>(
      sid,
      EncodeResponse(std::move(value.response)),
      std::move(value.factory),
      stream::StreamElementEncoder<T>{EncodeValue, EncodeError});
  message.requestContext = std::move(requestContext);
  a->writeResponse(std::move(message), std::move(adapterGuard));
}

/**
 * `FastHandlerCallback<StreamFactory<T>>::ExceptionFn` for a stream RPC: a
 * failure before the stream opens (arg parse, or the handler throwing before it
 * returns a factory) is reported as a `TApplicationException` on the request.
 * Declared-exception-on-open fidelity is a follow-up; this is the appUnknown
 * fallback, which does not depend on the method presult.
 */
inline void writeStreamException(
    ThriftServerAppAdapter* a,
    uint32_t sid,
    ThriftRequestContextPtr requestContext,
    folly::DelayedDestruction::DestructorGuard&& adapterGuard,
    folly::exception_wrapper ew) noexcept {
  auto message =
      makeUnknownExceptionMessage(sid, ew, apache::thrift::ErrorBlame::SERVER);
  message.requestContext = std::move(requestContext);
  a->writeResponse(std::move(message), std::move(adapterGuard));
}

} // namespace detail

} // namespace apache::thrift::fast_thrift::thrift
