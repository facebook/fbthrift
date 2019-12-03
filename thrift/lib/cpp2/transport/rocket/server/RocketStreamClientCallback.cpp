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

#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>

#include <functional>
#include <memory>
#include <utility>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Range.h>
#include <folly/ScopeGuard.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace apache {
namespace thrift {
namespace rocket {

class TimeoutCallback : public folly::HHWheelTimer::Callback {
 public:
  explicit TimeoutCallback(RocketStreamClientCallback& parent)
      : parent_(parent) {}
  void timeoutExpired() noexcept override {
    parent_.timeoutExpired();
  }

 private:
  RocketStreamClientCallback& parent_;
};

RocketStreamClientCallback::RocketStreamClientCallback(
    StreamId streamId,
    RocketServerConnection& connection,
    uint32_t initialRequestN)
    : streamId_(streamId), connection_(connection), tokens_(initialRequestN) {}

void RocketStreamClientCallback::onFirstResponse(
    FirstResponsePayload&& firstResponse,
    folly::EventBase* /* unused */,
    StreamServerCallback* serverCallback) {
  if (UNLIKELY(serverCallbackOrCancelled_ == kCancelledFlag)) {
    serverCallback->onStreamCancel();
    auto& connection = connection_;
    connection_.freeStream(streamId_);
    connection.decInflightRequests();
    return;
  }

  serverCallbackOrCancelled_ = reinterpret_cast<intptr_t>(serverCallback);

  DCHECK(tokens_ != 0);
  int tokens = 0;
  if (--tokens_) {
    tokens = std::exchange(tokens_, 0);
  } else {
    scheduleTimeout();
  }

  // compress the response if needed
  compressResponse(firstResponse);

  connection_.sendPayload(
      streamId_,
      pack(std::move(firstResponse)).value(),
      Flags::none().next(true));

  bool selfAlive = connection_.decInflightRequests();
  if (selfAlive && tokens) {
    request(tokens);
  }
}

void RocketStreamClientCallback::onFirstResponseError(
    folly::exception_wrapper ew) {
  ew.with_exception<thrift::detail::EncodedError>([&](auto&& encodedError) {
    connection_.sendPayload(
        streamId_,
        Payload::makeFromData(std::move(encodedError.encoded)),
        Flags::none().next(true).complete(true));
  });

  auto& connection = connection_;
  connection_.freeStream(streamId_);
  connection.decInflightRequests();
}

void RocketStreamClientCallback::onStreamNext(StreamPayload&& payload) {
  DCHECK(tokens_ != 0);
  if (!--tokens_) {
    scheduleTimeout();
  }
  // compress the response if needed
  compressResponse(payload);
  connection_.sendPayload(
      streamId_, pack(std::move(payload)).value(), Flags::none().next(true));
}

void RocketStreamClientCallback::onStreamComplete() {
  connection_.sendPayload(
      streamId_,
      Payload::makeFromData(std::unique_ptr<folly::IOBuf>{}),
      Flags::none().complete(true));
  connection_.freeStream(streamId_);
}

void RocketStreamClientCallback::onStreamError(folly::exception_wrapper ew) {
  ew.handle(
      [this](RocketException& rex) {
        connection_.sendError(
            streamId_,
            RocketException(ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      },
      [this](::apache::thrift::detail::EncodedError& err) {
        connection_.sendError(
            streamId_,
            RocketException(
                ErrorCode::APPLICATION_ERROR, std::move(err.encoded)));
      },
      [this, &ew](...) {
        connection_.sendError(
            streamId_,
            RocketException(ErrorCode::APPLICATION_ERROR, ew.what()));
      });
  connection_.freeStream(streamId_);
}

void RocketStreamClientCallback::onStreamHeaders(HeadersPayload&& payload) {
  connection_.sendExt(
      streamId_,
      pack(payload).value(),
      Flags::none().ignore(true),
      ExtFrameType::HEADERS_PUSH);
}

void RocketStreamClientCallback::resetServerCallback(
    StreamServerCallback& serverCallback) {
  serverCallbackOrCancelled_ = reinterpret_cast<intptr_t>(&serverCallback);
}

void RocketStreamClientCallback::request(uint32_t tokens) {
  if (!tokens) {
    return;
  }

  cancelTimeout();
  tokens_ += tokens;
  serverCallback()->onStreamRequestN(tokens);
}

void RocketStreamClientCallback::headers(HeadersPayload&& payload) {
  serverCallback()->onSinkHeaders(std::move(payload));
}

void RocketStreamClientCallback::onStreamCancel() {
  serverCallback()->onStreamCancel();
}

void RocketStreamClientCallback::timeoutExpired() noexcept {
  DCHECK_EQ(0, tokens_);

  serverCallback()->onStreamCancel();
  onStreamError(folly::make_exception_wrapper<rocket::RocketException>(
      rocket::ErrorCode::APPLICATION_ERROR,
      serializeErrorStruct(
          protoId_,
          TApplicationException(
              TApplicationException::TApplicationExceptionType::TIMEOUT))));
}

void RocketStreamClientCallback::setProtoId(protocol::PROTOCOL_TYPES protoId) {
  protoId_ = protoId;
}

StreamServerCallback& RocketStreamClientCallback::getStreamServerCallback() {
  DCHECK(serverCallbackReady());
  return *serverCallback();
}

void RocketStreamClientCallback::scheduleTimeout() {
  if (!timeoutCallback_) {
    timeoutCallback_ = std::make_unique<TimeoutCallback>(*this);
  }
  connection_.scheduleStreamTimeout(timeoutCallback_.get());
}

void RocketStreamClientCallback::cancelTimeout() {
  timeoutCallback_.reset();
}

template <class Payload>
void RocketStreamClientCallback::compressResponse(Payload& payload) {
  folly::Optional<CompressionAlgorithm> compression =
      connection_.getNegotiatedCompressionAlgorithm();

  if (compression.hasValue()) {
    folly::io::CodecType codec;
    switch (*compression) {
      case CompressionAlgorithm::ZSTD:
        codec = folly::io::CodecType::ZSTD;
        payload.metadata.compression_ref() = *compression;
        break;
      case CompressionAlgorithm::ZLIB:
        codec = folly::io::CodecType::ZLIB;
        payload.metadata.compression_ref() = *compression;
        break;
      case CompressionAlgorithm::NONE:
        codec = folly::io::CodecType::NO_COMPRESSION;
        break;
    }
    payload.payload =
        folly::io::getCodec(codec)->compress(payload.payload.get());
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
