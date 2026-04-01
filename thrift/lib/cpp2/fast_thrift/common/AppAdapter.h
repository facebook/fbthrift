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

#include <folly/ExceptionWrapper.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/EndpointAdapter.h>

namespace apache::thrift::fast_thrift::channel_pipeline {

/**
 * InboundAppHandler — alias for the generic EndpointHandler concept.
 *
 * In a transport pipeline the application layer is the head endpoint:
 * it consumes decoded messages and receives exceptions.
 */
template <typename A>
concept InboundAppHandler = EndpointHandler<A>;

/**
 * OutboundAppHandler concept - sends messages from application to pipeline.
 *
 * The application calls this to send a message through the pipeline
 * toward the transport.
 *
 * Note: This is an interface contract, not an owned object.
 * It does NOT require DelayedDestructionBase.
 */
template <typename O>
concept OutboundAppHandler = requires(O o, TypeErasedBox&& msg) {
  typename O::ResponseHandler;

  {
    o.write(std::declval<typename O::ResponseHandler>(), std::move(msg))
  } noexcept -> std::same_as<void>;
};

} // namespace apache::thrift::fast_thrift::channel_pipeline
