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
    rocket::RocketServerFrameContext&& context,
    uint32_t initialRequestN)
    : context_(std::move(context)), tokens_(initialRequestN) {}

void RocketStreamClientCallback::onFirstResponse(
    FirstResponsePayload&& firstResponse,
    folly::EventBase* /* unused */,
    StreamServerCallback* serverCallback) {
  serverCallback_ = serverCallback;

  DCHECK(tokens_ != 0);
  int tokens = 0;
  if (--tokens_) {
    tokens = std::exchange(tokens_, 0);
  } else {
    scheduleTimeout();
  }

  // compress the response if needed
  compressResponse(firstResponse);

  context_.sendPayload(
      rocket::pack(std::move(firstResponse)).value(),
      rocket::Flags::none().next(true));
  // ownership of the RocketStreamClientCallback transfers to connection
  // after onFirstResponse.
  bool selfAlive = context_.takeOwnership(this);
  if (selfAlive && tokens) {
    request(tokens);
  }
}

void RocketStreamClientCallback::onFirstResponseError(
    folly::exception_wrapper ew) {
  SCOPE_EXIT {
    delete this;
  };

  ew.with_exception<thrift::detail::EncodedError>([&](auto&& encodedError) {
    context_.sendPayload(
        rocket::Payload::makeFromData(std::move(encodedError.encoded)),
        rocket::Flags::none().next(true).complete(true));
  });
}

void RocketStreamClientCallback::onStreamNext(StreamPayload&& payload) {
  DCHECK(tokens_ != 0);
  if (!--tokens_) {
    scheduleTimeout();
  }
  // compress the response if needed
  compressResponse(payload);

  context_.sendPayload(
      rocket::pack(std::move(payload)).value(),
      rocket::Flags::none().next(true));
}

void RocketStreamClientCallback::onStreamComplete() {
  context_.sendPayload(
      rocket::Payload::makeFromData(std::unique_ptr<folly::IOBuf>{}),
      rocket::Flags::none().complete(true));
  context_.freeStream();
}

void RocketStreamClientCallback::onStreamError(folly::exception_wrapper ew) {
  if (!ew.with_exception<rocket::RocketException>([this](auto&& rex) {
        context_.sendError(rocket::RocketException(
            rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      })) {
    context_.sendError(rocket::RocketException(
        rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
  }
  context_.freeStream();
}

void RocketStreamClientCallback::resetServerCallback(
    StreamServerCallback& serverCallback) {
  serverCallback_ = &serverCallback;
}

void RocketStreamClientCallback::request(uint32_t tokens) {
  if (!tokens) {
    return;
  }

  cancelTimeout();
  tokens_ += tokens;
  serverCallback_->onStreamRequestN(tokens);
}

void RocketStreamClientCallback::timeoutExpired() noexcept {
  DCHECK_EQ(0, tokens_);

  serverCallback_->onStreamCancel();
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
  DCHECK(serverCallback_ != nullptr);
  return *serverCallback_;
}

void RocketStreamClientCallback::scheduleTimeout() {
  if (!timeoutCallback_) {
    timeoutCallback_ = std::make_unique<TimeoutCallback>(*this);
  }
  context_.scheduleStreamTimeout(timeoutCallback_.get());
}

void RocketStreamClientCallback::cancelTimeout() {
  timeoutCallback_.reset();
}

template <class Payload>
void RocketStreamClientCallback::compressResponse(Payload& payload) {
  rocket::RocketServerConnection& connection = context_.connection();
  folly::Optional<CompressionAlgorithm> compression =
      connection.getNegotiatedCompressionAlgorithm();

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

} // namespace thrift
} // namespace apache
