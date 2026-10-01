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

#include <utility>

#include <glog/logging.h>
#include <folly/Function.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/ProducerPipeline.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/StreamElementEncoder.h>

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * StreamFactory<T> — the value a server handler returns for a stream RPC, and
 * the application->framework half of the producing side.
 *
 * Roles are named for the server's role: a "stream" RPC makes the server the
 * stream (producer) and the client the sink (consumer). "Factory" types always
 * flow application->framework and carry a deferred pipeline-config awaiting the
 * framework-supplied codec; the non-factory `Stream<T>`
 * (framework->application, client side) is its mirror.
 *
 * The application composes its producer via [[Composer]] in terms of `T`,
 * without naming the wire protocol. The framework holds the
 * protocol-specialized
 * [[StreamElementEncoder]] and calls `bind()` to collapse the factory into a
 * plain `ProducerPipeline::ConfigFunc` — exactly what the stream mux consumes
 * to build the producing-end sub-pipeline.
 */
template <typename T>
class StreamFactory {
 public:
  /**
   * Application-supplied producer composition: adds the application's producer
   * handlers to the builder, using the encoder to turn each `T` (and any
   * producer failure) into a wire chunk.
   */
  using Composer = folly::Function<void(
      ProducerPipeline::Builder&, const StreamElementEncoder<T>&)>;

  explicit StreamFactory(Composer composer) noexcept
      : composer_(std::move(composer)) {}

  /**
   * Bind the framework's element encoder, yielding the
   * `ProducerPipeline::ConfigFunc` the mux runs to compose this stream's
   * producing-end sub-pipeline. Consumes the factory. Both encoder pointers
   * must be non-null.
   */
  ProducerPipeline::ConfigFunc bind(StreamElementEncoder<T> encoder) && {
    DCHECK(encoder.encodeValue != nullptr);
    DCHECK(encoder.encodeError != nullptr);
    return ProducerPipeline::ConfigFunc(
        [composer = std::move(composer_), encoder = std::move(encoder)](
            ProducerPipeline::Builder& builder) mutable {
          composer(builder, encoder);
        });
  }

 private:
  Composer composer_;
};

} // namespace apache::thrift::fast_thrift::thrift::stream
