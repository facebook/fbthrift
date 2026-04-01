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
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>

#include <functional>

namespace apache::thrift::fast_thrift::common::test {

/**
 * MockAppAdapter satisfies InboundAppHandler (EndpointHandler) concept.
 * Used for testing app-side pipeline integration.
 */
class MockAppAdapter {
 public:
  using OnMessageCallback = std::function<channel_pipeline::Result(
      channel_pipeline::TypeErasedBox&&)>;
  using OnExceptionCallback = std::function<void(folly::exception_wrapper&&)>;

  MockAppAdapter() = default;

  channel_pipeline::Result onMessage(
      channel_pipeline::TypeErasedBox&& msg) noexcept {
    messageCount_++;
    if (onMessageCallback_) {
      return onMessageCallback_(std::move(msg));
    }
    return messageResult_;
  }

  void onException(folly::exception_wrapper&& e) noexcept {
    exceptionCount_++;
    if (onExceptionCallback_) {
      onExceptionCallback_(std::move(e));
    }
  }

  void setMessageResult(channel_pipeline::Result result) {
    messageResult_ = result;
  }

  void setOnMessageCallback(OnMessageCallback cb) {
    onMessageCallback_ = std::move(cb);
  }

  void setOnExceptionCallback(OnExceptionCallback cb) {
    onExceptionCallback_ = std::move(cb);
  }

  int messageCount() const { return messageCount_; }

  int exceptionCount() const { return exceptionCount_; }

  void reset() {
    messageCount_ = 0;
    exceptionCount_ = 0;
    messageResult_ = channel_pipeline::Result::Success;
    onMessageCallback_ = nullptr;
    onExceptionCallback_ = nullptr;
  }

 private:
  int messageCount_{0};
  int exceptionCount_{0};
  channel_pipeline::Result messageResult_{channel_pipeline::Result::Success};
  OnMessageCallback onMessageCallback_;
  OnExceptionCallback onExceptionCallback_;
};

} // namespace apache::thrift::fast_thrift::common::test
