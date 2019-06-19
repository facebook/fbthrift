/*
 * Copyright 2018-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace apache {
namespace thrift {

namespace {
rocket::Payload pack(FirstResponsePayload&& firstResponse) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  firstResponse.metadata.write(&writer);
  return rocket::Payload::makeFromMetadataAndData(
      queue.move(), std::move(firstResponse.payload));
}
} // namespace

RocketStreamClientCallback::RocketStreamClientCallback(
    rocket::RocketServerFrameContext&& context,
    uint32_t initialRequestN)
    : context_(std::make_unique<rocket::RocketServerFrameContext>(
          std::move(context))),
      tokens_(initialRequestN) {}

void RocketStreamClientCallback::onFirstResponse(
    FirstResponsePayload&& firstResponse,
    folly::EventBase* /* unused */,
    StreamServerCallback* serverCallback) {
  serverCallback_ = serverCallback;

  if (!context_) {
    // Canceled before first response
    return cancel();
  }

  DCHECK(tokens_ != 0);
  if (--tokens_) {
    request(std::exchange(tokens_, 0));
  } else {
    scheduleTimeout();
  }

  context_->sendPayload(
      pack(std::move(firstResponse)), rocket::Flags::none().next(true));
}

void RocketStreamClientCallback::onFirstResponseError(
    folly::exception_wrapper ew) {
  SCOPE_EXIT {
    delete this;
  };

  cancelTimeout();

  if (context_) {
    ew.with_exception<thrift::detail::EncodedError>([&](auto&& encodedError) {
      context_->sendPayload(
          rocket::Payload::makeFromData(std::move(encodedError.encoded)),
          rocket::Flags::none().next(true).complete(true));
    });
    context_->detachStreamFromConnection();
  }
}

void RocketStreamClientCallback::onStreamNext(StreamPayload&& payload) {
  DCHECK(tokens_ != 0);
  if (!--tokens_) {
    scheduleTimeout();
  }

  if (context_) {
    context_->sendPayload(
        rocket::Payload::makeFromData(std::move(payload.payload)),
        rocket::Flags::none().next(true));
  }
}

void RocketStreamClientCallback::onStreamComplete() {
  SCOPE_EXIT {
    delete this;
  };

  cancelTimeout();
  serverCallback_ = nullptr;

  if (context_) {
    context_->sendPayload(
        rocket::Payload::makeFromData(std::unique_ptr<folly::IOBuf>{}),
        rocket::Flags::none().complete(true));
    context_->detachStreamFromConnection();
  }
}

void RocketStreamClientCallback::onStreamError(folly::exception_wrapper ew) {
  SCOPE_EXIT {
    delete this;
  };

  cancelTimeout();
  serverCallback_ = nullptr;

  if (!context_) {
    return;
  }

  if (!ew.with_exception<rocket::RocketException>([this](auto&& rex) {
        context_->sendError(rocket::RocketException(
            rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      })) {
    context_->sendError(rocket::RocketException(
        rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
  }
  context_->detachStreamFromConnection();
}

void RocketStreamClientCallback::request(uint32_t tokens) {
  if (!tokens) {
    return;
  }

  cancelTimeout();
  tokens_ += tokens;
  if (serverCallback_) {
    serverCallback_->onStreamRequestN(tokens);
  }
}

void RocketStreamClientCallback::cancel() {
  cancelTimeout();
  context_.reset();

  if (auto* serverCallback = std::exchange(serverCallback_, nullptr)) {
    serverCallback->onStreamCancel();
    delete this;
  }
}

void RocketStreamClientCallback::timeoutExpired() noexcept {
  DCHECK_EQ(0, tokens_);

  if (auto* serverCallback = std::exchange(serverCallback_, nullptr)) {
    serverCallback->onStreamCancel();
  }
  onStreamError(folly::make_exception_wrapper<TApplicationException>(
      TApplicationException::TApplicationExceptionType::TIMEOUT));
}

void RocketStreamClientCallback::scheduleTimeout() {
  if (context_) {
    context_->scheduleStreamTimeout(this);
  }
}

} // namespace thrift
} // namespace apache
