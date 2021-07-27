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

#pragma once

#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Utility.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

struct FirstResponsePayload {
  FirstResponsePayload(
      std::unique_ptr<folly::IOBuf> p, ResponseRpcMetadata&& md)
      : payload(std::move(p)), metadata(std::move(md)) {}

  std::unique_ptr<folly::IOBuf> payload;
  ResponseRpcMetadata metadata;
};

struct StreamPayload {
  StreamPayload(std::unique_ptr<folly::IOBuf> p, StreamPayloadMetadata&& md)
      : payload(std::move(p)), metadata(std::move(md)) {}

  std::unique_ptr<folly::IOBuf> payload;
  StreamPayloadMetadata metadata;
};

struct HeadersPayload {
  HeadersPayload(HeadersPayloadContent&& p, HeadersPayloadMetadata&& md)
      : payload(std::move(p)), metadata(std::move(md)) {}

  explicit HeadersPayload(HeadersPayloadContent&& p) : payload(std::move(p)) {}

  explicit HeadersPayload(StreamPayloadMetadata&& sp) {
    payload.otherMetadata_ref().copy_from(sp.otherMetadata_ref());
    metadata.compression_ref().copy_from(sp.compression_ref());
  }

  HeadersPayloadContent payload;
  HeadersPayloadMetadata metadata;

  explicit operator StreamPayload() && {
    StreamPayloadMetadata md;
    md.otherMetadata_ref().copy_from(payload.otherMetadata_ref());
    md.compression_ref().copy_from(metadata.compression_ref());
    return StreamPayload(nullptr, std::move(md));
  }
};

namespace detail {

struct FOLLY_EXPORT EncodedError : std::exception {
  explicit EncodedError(std::unique_ptr<folly::IOBuf> buf)
      : encoded(std::move(buf)) {}

  EncodedError(const EncodedError& oth) : encoded(oth.encoded->clone()) {}
  EncodedError& operator=(const EncodedError& oth) {
    encoded = oth.encoded->clone();
    return *this;
  }
  EncodedError(EncodedError&&) = default;
  EncodedError& operator=(EncodedError&&) = default;

  std::unique_ptr<folly::IOBuf> encoded;
};

struct FOLLY_EXPORT EncodedFirstResponseError : std::exception {
  explicit EncodedFirstResponseError(FirstResponsePayload&& payload)
      : encoded(std::move(payload)) {}

  EncodedFirstResponseError(const EncodedFirstResponseError& other)
      : encoded(
            other.encoded.payload->clone(),
            folly::copy(other.encoded.metadata)) {}
  EncodedFirstResponseError& operator=(const EncodedFirstResponseError& other) {
    encoded = FirstResponsePayload(
        other.encoded.payload->clone(), folly::copy(other.encoded.metadata));
    return *this;
  }
  EncodedFirstResponseError(EncodedFirstResponseError&&) = default;
  EncodedFirstResponseError& operator=(EncodedFirstResponseError&&) = default;

  FirstResponsePayload encoded;
};

struct FOLLY_EXPORT EncodedStreamError : std::exception {
  explicit EncodedStreamError(StreamPayload&& payload)
      : encoded(std::move(payload)) {}

  EncodedStreamError(const EncodedStreamError& other)
      : encoded(
            other.encoded.payload->clone(),
            folly::copy(other.encoded.metadata)) {}
  EncodedStreamError& operator=(const EncodedStreamError& other) {
    encoded = StreamPayload(
        other.encoded.payload->clone(), folly::copy(other.encoded.metadata));
    return *this;
  }
  EncodedStreamError(EncodedStreamError&&) = default;
  EncodedStreamError& operator=(EncodedStreamError&&) = default;

  StreamPayload encoded;
};

struct FOLLY_EXPORT EncodedStreamRpcError : std::exception {
  explicit EncodedStreamRpcError(std::unique_ptr<folly::IOBuf> rpcError)
      : encoded(std::move(rpcError)) {}
  EncodedStreamRpcError(const EncodedStreamRpcError& oth)
      : encoded(oth.encoded->clone()) {}
  EncodedStreamRpcError& operator=(const EncodedStreamRpcError& oth) {
    encoded = oth.encoded->clone();
    return *this;
  }
  EncodedStreamRpcError(EncodedStreamRpcError&&) = default;
  EncodedStreamRpcError& operator=(EncodedStreamRpcError&&) = default;
  std::unique_ptr<folly::IOBuf> encoded;
};

inline bool hasException(const FirstResponsePayload& payload) {
  auto payloadMetadataRef = payload.metadata.payloadMetadata_ref();
  return payloadMetadataRef &&
      payloadMetadataRef->getType() == PayloadMetadata::exceptionMetadata;
}

} // namespace detail

/**
 * The Stream contract is a flow-controlled server-to-client channel
 *
 * The contract is initiated when the client sends a stream request.
 * The server must calls onFirstResponse or onFirstResponseError.
 * After the first response the server may call onStreamNext repeatedly
 * (as many times as it has tokens).
 * It may always call onStreamError/Complete (even without tokens).
 * It may always call onSinkHeaders (does not use tokens).
 * After the first response the client may call onStreamRequestN repeatedly
 * and onStreamCancel once.
 *
 * on* methods that return void terminate the contract. No method may be called
 * on either callback after a terminating method is called on either callback.
 * Methods that return bool are not terminating unless they return false.
 * These methods must return false if they called a terminating method inline,
 * or true otherwise.
 */

class StreamClientCallback;

class StreamServerCallback {
 public:
  virtual ~StreamServerCallback() = default;

  FOLLY_NODISCARD virtual bool onStreamRequestN(uint64_t) = 0;
  virtual void onStreamCancel() = 0;

  FOLLY_NODISCARD virtual bool onSinkHeaders(HeadersPayload&&) { return true; }

  // not terminating
  virtual void resetClientCallback(StreamClientCallback&) = 0;

  virtual void pauseStream() {}
  virtual void resumeStream() {}
};

class StreamClientCallback {
 public:
  virtual ~StreamClientCallback() = default;

  // StreamClientCallback must remain alive until onFirstResponse or
  // onFirstResponseError callback runs.
  FOLLY_NODISCARD virtual bool onFirstResponse(
      FirstResponsePayload&&, folly::EventBase*, StreamServerCallback*) = 0;
  virtual void onFirstResponseError(folly::exception_wrapper) = 0;

  FOLLY_NODISCARD virtual bool onStreamNext(StreamPayload&&) = 0;
  virtual void onStreamError(folly::exception_wrapper) = 0;
  virtual void onStreamComplete() = 0;
  FOLLY_NODISCARD virtual bool onStreamHeaders(HeadersPayload&&) {
    return true;
  }

  // not terminating
  virtual void resetServerCallback(StreamServerCallback&) = 0;
};

struct StreamServerCallbackCancel {
  void operator()(StreamServerCallback* streamServerCallback) noexcept {
    streamServerCallback->onStreamCancel();
  }
};

using StreamServerCallbackPtr =
    std::unique_ptr<StreamServerCallback, StreamServerCallbackCancel>;

/**
 * The Sink contract is a flow-controlled client-to-server channel
 *
 * The contract is initiated when the client sends a sink request.
 * The server must calls onFirstResponse or onFirstResponseError.
 * After the first response the client may call onSinkNext repeatedly
 * (as many times as it has tokens).
 * It may always call onSinkError/onStreamCancel, which terminate the contract,
 * and onSinkComplete, after which the server must call onFinalResponse/Error
 * to terminate the contract.
 * After the first response the server may call onSinkRequestN repeatedly.
 *
 * on* methods that return void terminate the contract. No method may be called
 * on either callback after a terminating method is called on either callback.
 * Methods that return bool are not terminating unless they return false.
 * These methods must return false if they called a terminating method inline,
 * or true otherwise.
 */

class SinkClientCallback;

class SinkServerCallback {
 public:
  virtual ~SinkServerCallback() = default;

  FOLLY_NODISCARD virtual bool onSinkNext(StreamPayload&&) = 0;
  virtual void onSinkError(folly::exception_wrapper) = 0;
  FOLLY_NODISCARD virtual bool onSinkComplete() = 0;

  // not terminating
  virtual void resetClientCallback(SinkClientCallback&) = 0;
};

class SinkClientCallback {
 public:
  virtual ~SinkClientCallback() = default;
  FOLLY_NODISCARD virtual bool onFirstResponse(
      FirstResponsePayload&&, folly::EventBase*, SinkServerCallback*) = 0;
  virtual void onFirstResponseError(folly::exception_wrapper) = 0;

  virtual void onFinalResponse(StreamPayload&&) = 0;
  virtual void onFinalResponseError(folly::exception_wrapper) = 0;

  FOLLY_NODISCARD virtual bool onSinkRequestN(uint64_t) = 0;

  // not terminating
  virtual void resetServerCallback(SinkServerCallback&) = 0;
};

struct SinkServerCallbackSendError {
  void operator()(SinkServerCallback* sinkServerCallback) noexcept {
    TApplicationException ex(
        TApplicationException::TApplicationExceptionType::INTERRUPTION,
        "Sink server callback canceled");
    sinkServerCallback->onSinkError(std::move(ex));
  }
};

using SinkServerCallbackPtr =
    std::unique_ptr<SinkServerCallback, SinkServerCallbackSendError>;
} // namespace thrift
} // namespace apache
