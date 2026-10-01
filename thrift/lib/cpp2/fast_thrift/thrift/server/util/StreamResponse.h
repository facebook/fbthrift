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
#include <folly/io/async/DelayedDestruction.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/ThriftServerAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/StreamResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponseMetadata.h>
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
 * The initial response currently carries only success metadata (no initial
 * value); `ResponseAndServerStream`-style initial responses and the precise
 * chunk flags are a follow-up once the client stream path lands.
 */
template <typename T>
inline ThriftServerResponseMessage makeStreamOpenMessage(
    uint32_t streamId,
    stream::StreamFactory<T>&& factory,
    stream::StreamElementEncoder<T> encoder) {
  auto metadata = std::make_unique<apache::thrift::ResponseRpcMetadata>();
  fillSuccessResponseMetadata(*metadata);
  return ThriftServerResponseMessage{
      .payload = ThriftServerStreamOpenPayload{
          .initialResponse =
              ThriftStreamInitialResponsePayload{
                  .metadata = std::move(metadata),
                  .streamId = streamId,
                  .complete = false,
                  .next = true},
          .configFunc = std::make_unique<stream::ProducerPipeline::ConfigFunc>(
              std::move(factory).bind(std::move(encoder)))}};
}

namespace detail {

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
      std::move(factory),
      stream::StreamElementEncoder<T>{EncodeValue, EncodeError});
  message.requestContext = std::move(requestContext);
  a->writeResponse(std::move(message), std::move(adapterGuard));
}

} // namespace detail

} // namespace apache::thrift::fast_thrift::thrift
