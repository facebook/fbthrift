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

#include <rust/cxx.h>
#include <folly/Function.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/rust/CallbackContext.h>
#include <thrift/lib/rust/channel_pipeline/src/ffi.rs.h>

namespace channel_pipeline_rust {

/**
 * Tail endpoint shim for an EventBase-local Rust application endpoint.
 *
 * The borrowed context and erased message are valid only for `onRead`. A Rust
 * endpoint that suspends must move the message and continuation into
 * `DeferredRead`; lifecycle methods intentionally receive no context, matching
 * the native TailEndpointHandler contract.
 */
class RustTailEndpoint final {
 public:
  explicit RustTailEndpoint(rust::Box<RustTailEndpointOpaque> endpoint)
      : endpoint_{std::move(endpoint)} {}

  ~RustTailEndpoint() = default;

  RustTailEndpoint(const RustTailEndpoint&) = delete;
  RustTailEndpoint& operator=(const RustTailEndpoint&) = delete;
  RustTailEndpoint(RustTailEndpoint&&) = delete;
  RustTailEndpoint& operator=(RustTailEndpoint&&) = delete;

  apache::thrift::fast_thrift::channel_pipeline::Result onRead(
      apache::thrift::fast_thrift::channel_pipeline::detail::ContextImpl& ctx,
      apache::thrift::fast_thrift::channel_pipeline::TypeErasedBox&&
          message) noexcept {
    using apache::thrift::fast_thrift::channel_pipeline::Result;
    try {
      if (message.empty()) {
        return Result::Error;
      }
      CallbackContext context{ctx, message};
      return static_cast<Result>(
          rust_tail_endpoint_on_read(*endpoint_, context, message));
    } catch (...) {
      return Result::Error;
    }
  }

  void onException(folly::exception_wrapper&& error) noexcept {
    rust_tail_endpoint_on_exception(*endpoint_);
    if (onException_) {
      onException_(std::move(error));
    }
  }

  void setOnException(
      folly::Function<void(folly::exception_wrapper&&) noexcept> callback) {
    onException_ = std::move(callback);
  }

  void onWriteReady() noexcept {
    rust_tail_endpoint_on_write_ready(*endpoint_);
  }

  void onPipelineActive() noexcept {
    rust_tail_endpoint_on_pipeline_active(*endpoint_);
  }

  void onPipelineInactive() noexcept {
    rust_tail_endpoint_on_pipeline_inactive(*endpoint_);
  }

  void handlerAdded() noexcept { rust_tail_endpoint_handler_added(*endpoint_); }

  void handlerRemoved() noexcept {
    rust_tail_endpoint_handler_removed(*endpoint_);
  }

 private:
  rust::Box<RustTailEndpointOpaque> endpoint_;
  folly::Function<void(folly::exception_wrapper&&) noexcept> onException_;
};

static_assert(
    apache::thrift::fast_thrift::channel_pipeline::TailEndpointHandler<
        RustTailEndpoint>);

} // namespace channel_pipeline_rust
