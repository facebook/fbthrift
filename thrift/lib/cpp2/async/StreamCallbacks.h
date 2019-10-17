/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

struct FirstResponsePayload {
  FirstResponsePayload(std::unique_ptr<folly::IOBuf> p, ResponseRpcMetadata md)
      : payload(std::move(p)), metadata(std::move(md)) {}

  std::unique_ptr<folly::IOBuf> payload;
  ResponseRpcMetadata metadata;
};

struct StreamPayload {
  StreamPayload(std::unique_ptr<folly::IOBuf> p, StreamPayloadMetadata md)
      : payload(std::move(p)), metadata(std::move(md)) {}

  std::unique_ptr<folly::IOBuf> payload;
  StreamPayloadMetadata metadata;
};

class StreamClientCallback;

class StreamServerCallback {
 public:
  virtual ~StreamServerCallback() = default;

  virtual void onStreamRequestN(uint64_t) = 0;
  virtual void onStreamCancel() = 0;

  virtual void resetClientCallback(StreamClientCallback&) = 0;
};

class ChannelServerCallback {
 public:
  virtual ~ChannelServerCallback() = default;

  virtual void onStreamRequestN(uint64_t) = 0;
  virtual void onStreamCancel() = 0;

  virtual void onSinkNext(StreamPayload&&) = 0;
  virtual void onSinkError(folly::exception_wrapper) = 0;
  virtual void onSinkComplete() = 0;
};

class SinkServerCallback {
 public:
  virtual ~SinkServerCallback() = default;

  virtual void onSinkNext(StreamPayload&&) = 0;
  virtual void onSinkError(folly::exception_wrapper) = 0;
  virtual void onSinkComplete() = 0;

  virtual void onStreamCancel() = 0;
};

class StreamClientCallback {
 public:
  virtual ~StreamClientCallback() = default;

  // StreamClientCallback must remain alive until onFirstResponse or
  // onFirstResponseError callback runs.
  virtual void onFirstResponse(
      FirstResponsePayload&&,
      folly::EventBase*,
      StreamServerCallback*) = 0;
  virtual void onFirstResponseError(folly::exception_wrapper) = 0;

  virtual void onStreamNext(StreamPayload&&) = 0;
  virtual void onStreamError(folly::exception_wrapper) = 0;
  virtual void onStreamComplete() = 0;

  virtual void resetServerCallback(StreamServerCallback&) = 0;
};

class ChannelClientCallback {
 public:
  virtual ~ChannelClientCallback() = default;

  // ChannelClientCallback must remain alive until onFirstResponse or
  // onFirstResponseError callback runs.
  virtual void onFirstResponse(
      FirstResponsePayload&&,
      folly::EventBase*,
      ChannelServerCallback*) = 0;
  virtual void onFirstResponseError(folly::exception_wrapper) = 0;

  virtual void onStreamNext(StreamPayload&&) = 0;
  virtual void onStreamError(folly::exception_wrapper) = 0;
  virtual void onStreamComplete() = 0;

  virtual void onSinkRequestN(uint64_t) = 0;
  virtual void onSinkCancel() = 0;
};

class SinkClientCallback {
 public:
  virtual ~SinkClientCallback() = default;
  virtual void onFirstResponse(
      FirstResponsePayload&&,
      folly::EventBase*,
      SinkServerCallback*) = 0;
  virtual void onFirstResponseError(folly::exception_wrapper) = 0;

  virtual void onFinalResponse(StreamPayload&&) = 0;
  virtual void onFinalResponseError(folly::exception_wrapper) = 0;

  virtual void onSinkRequestN(uint64_t) = 0;
};

} // namespace thrift
} // namespace apache
