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

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * StreamElementEncoder<T> — the per-element codec for one producing stream:
 * turns an application value (or a producer failure) into the wire chunk the
 * producing-end sub-pipeline emits.
 *
 * Function pointers, no vtable — consistent with fast_thrift request/response
 * encoding, where `FastHandlerCallback` holds `resultFn_` / `exceptionFn_` as
 * plain function pointers specialized per (presult, protocol). Thrift codegen
 * supplies the protocol-specialized instantiation; the framework selects it per
 * request and binds it into a [[StreamFactory]], which yields the
 * `ProducerPipeline::ConfigFunc` the stream mux runs.
 *
 * `encodeValue` currently assumes serialization succeeds; a `folly::Try`-style
 * result that turns an encode failure into a `stream::Error` can be added when
 * a codec that can fail comes online.
 *
 * Both pointers are required. They default to null so an unset codec fails
 * deterministically (a null call) rather than dereferencing an indeterminate
 * value; `StreamFactory::bind` DCHECKs them.
 */
template <typename T>
struct StreamElementEncoder {
  // Encode one application element into an outbound stream data chunk.
  Payload (*encodeValue)(T&&) noexcept {nullptr};
  // Encode a producer failure into a terminal stream error.
  Error (*encodeError)(folly::exception_wrapper&&) noexcept {nullptr};
};

} // namespace apache::thrift::fast_thrift::thrift::stream
