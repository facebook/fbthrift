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

#include <boost/variant.hpp>
#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/Request.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace rocket {
class RocketClient;

enum class StreamChannelStatus { Alive, Complete, ContractViolation };

class RocketStreamServerCallback : public StreamServerCallback {
 public:
  RocketStreamServerCallback(
      rocket::StreamId streamId,
      rocket::RocketClient& client,
      StreamClientCallback& clientCallback)
      : client_(client),
        clientCallback_(&clientCallback),
        streamId_(streamId) {}

  bool onStreamRequestN(uint64_t tokens) override;
  void onStreamCancel() override;

  bool onSinkHeaders(HeadersPayload&& payload) override;

  void resetClientCallback(StreamClientCallback& clientCallback) override {
    clientCallback_ = &clientCallback;
  }

  void onInitialPayload(FirstResponsePayload&&, folly::EventBase*);
  void onInitialError(folly::exception_wrapper ew);
  void onStreamTransportError(folly::exception_wrapper);

  StreamChannelStatus onStreamPayload(StreamPayload&&);
  StreamChannelStatus onStreamFinalPayload(StreamPayload&&);
  StreamChannelStatus onStreamComplete();
  StreamChannelStatus onStreamError(folly::exception_wrapper);
  void onStreamHeaders(HeadersPayload&&);

  StreamChannelStatus onSinkRequestN(uint64_t tokens);
  StreamChannelStatus onSinkCancel();

  void timeoutExpired() noexcept;

  rocket::StreamId streamId() const noexcept {
    return streamId_;
  }

 protected:
  rocket::RocketClient& client_;

 private:
  StreamClientCallback* clientCallback_;
  rocket::StreamId streamId_;
};

class RocketStreamServerCallbackWithChunkTimeout
    : public RocketStreamServerCallback {
 public:
  RocketStreamServerCallbackWithChunkTimeout(
      rocket::StreamId streamId,
      rocket::RocketClient& client,
      StreamClientCallback& clientCallback,
      std::chrono::milliseconds chunkTimeout,
      uint64_t initialCredits)
      : RocketStreamServerCallback(streamId, client, clientCallback),
        chunkTimeout_(chunkTimeout),
        credits_(initialCredits) {}

  bool onStreamRequestN(uint64_t tokens) override;

  void onInitialPayload(FirstResponsePayload&&, folly::EventBase*);

  StreamChannelStatus onStreamPayload(StreamPayload&&);

  void timeoutExpired() noexcept;

 private:
  void scheduleTimeout();
  void cancelTimeout();

  const std::chrono::milliseconds chunkTimeout_;
  uint64_t credits_{0};
  std::unique_ptr<folly::HHWheelTimer::Callback> timeout_;
};

class RocketChannelServerCallback : public ChannelServerCallback {
 public:
  RocketChannelServerCallback(
      rocket::StreamId streamId,
      rocket::RocketClient& client,
      ChannelClientCallback& clientCallback)
      : client_(client), clientCallback_(clientCallback), streamId_(streamId) {}

  void onStreamRequestN(uint64_t tokens) override;
  void onStreamCancel() override;

  void onSinkNext(StreamPayload&&) override;
  void onSinkError(folly::exception_wrapper) override;
  void onSinkComplete() override;

  void onInitialPayload(FirstResponsePayload&&, folly::EventBase*);
  void onInitialError(folly::exception_wrapper);
  void onStreamTransportError(folly::exception_wrapper);

  StreamChannelStatus onStreamPayload(StreamPayload&&);
  StreamChannelStatus onStreamFinalPayload(StreamPayload&&);
  StreamChannelStatus onStreamComplete();
  StreamChannelStatus onStreamError(folly::exception_wrapper);
  void onStreamHeaders(HeadersPayload&&);

  StreamChannelStatus onSinkRequestN(uint64_t tokens);
  StreamChannelStatus onSinkCancel();

  rocket::StreamId streamId() const noexcept {
    return streamId_;
  }

 private:
  rocket::RocketClient& client_;
  ChannelClientCallback& clientCallback_;
  rocket::StreamId streamId_;
  enum class State { BothOpen, StreamOpen, SinkOpen };
  State state_{State::BothOpen};
};

class RocketSinkServerCallback : public SinkServerCallback {
 public:
  RocketSinkServerCallback(
      rocket::StreamId streamId,
      rocket::RocketClient& client,
      SinkClientCallback& clientCallback,
      bool pageAligned,
      std::unique_ptr<CompressionConfig> compressionConfig)
      : client_(client),
        clientCallback_(&clientCallback),
        streamId_(streamId),
        pageAligned_(pageAligned),
        compressionConfig_(std::move(compressionConfig)) {}

  bool onSinkNext(StreamPayload&&) override;
  void onSinkError(folly::exception_wrapper) override;
  bool onSinkComplete() override;

  void resetClientCallback(SinkClientCallback& clientCallback) override {
    clientCallback_ = &clientCallback;
  }

  void onInitialPayload(FirstResponsePayload&&, folly::EventBase*);
  void onInitialError(folly::exception_wrapper);
  void onStreamTransportError(folly::exception_wrapper);

  StreamChannelStatus onStreamPayload(StreamPayload&&);
  StreamChannelStatus onStreamFinalPayload(StreamPayload&&);
  StreamChannelStatus onStreamComplete();
  StreamChannelStatus onStreamError(folly::exception_wrapper);
  void onStreamHeaders(HeadersPayload&&);

  StreamChannelStatus onSinkRequestN(uint64_t tokens);
  StreamChannelStatus onSinkCancel();

  rocket::StreamId streamId() const noexcept {
    return streamId_;
  }

 private:
  rocket::RocketClient& client_;
  SinkClientCallback* clientCallback_;
  rocket::StreamId streamId_;
  bool pageAligned_;
  enum class State { BothOpen, StreamOpen };
  State state_{State::BothOpen};
  std::unique_ptr<CompressionConfig> compressionConfig_;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
