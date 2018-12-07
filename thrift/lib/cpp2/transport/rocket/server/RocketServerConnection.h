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

#pragma once

#include <memory>
#include <ostream>
#include <unordered_map>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/container/F14Map.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>

#include <wangle/acceptor/ManagedConnection.h>

#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>

namespace apache {
namespace thrift {
namespace rocket {

class RocketServerConnection
    : public wangle::ManagedConnection,
      private folly::AsyncTransportWrapper::WriteCallback {
 public:
  using UniquePtr = std::
      unique_ptr<RocketServerConnection, folly::DelayedDestruction::Destructor>;

  RocketServerConnection(
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::shared_ptr<RocketServerHandler> frameHandler);

  void send(std::unique_ptr<folly::IOBuf> data);

  // Create a stream subscriber with initialRequestN credits
  static std::shared_ptr<RocketServerStreamSubscriber> createStreamSubscriber(
      RocketServerFrameContext&& context,
      uint32_t initialRequestN);

  // Parser callbacks
  void handleFrame(std::unique_ptr<folly::IOBuf> frame);
  void close(folly::exception_wrapper ew);

  void onContextConstructed() {
    ++inflight_;
  }

  void onContextDestroyed() {
    DCHECK(inflight_ != 0);
    --inflight_;
    closeIfNeeded();
  }

  // AsyncTransportWrapper::WriteCallback implementation
  void writeSuccess() noexcept final;
  void writeErr(
      size_t bytesWritten,
      const folly::AsyncSocketException& ex) noexcept final;

  folly::EventBase& getEventBase() const {
    return evb_;
  }

 private:
  // Note that attachEventBase()/detachEventBase() are not supported in server
  // code
  folly::EventBase& evb_;
  folly::AsyncTransportWrapper::UniquePtr socket_;

  Parser<RocketServerConnection> parser_{*this};
  const std::shared_ptr<RocketServerHandler> frameHandler_;
  bool setupFrameReceived_{false};
  folly::F14NodeMap<StreamId, RocketServerFrameContext> partialFrames_;

  // Total number of active Request* frames ("streams" in protocol parlance)
  size_t inflight_{0};
  enum class ConnectionState : uint8_t {
    ALIVE,
    CLOSING,
    CLOSED,
  };
  ConnectionState state_{ConnectionState::ALIVE};

  // streams_ map only maintains entries for REQUEST_STREAM streams
  std::unordered_map<StreamId, std::shared_ptr<RocketServerStreamSubscriber>>
      streams_;

  class BatchWriteLoopCallback : public folly::EventBase::LoopCallback {
   public:
    explicit BatchWriteLoopCallback(RocketServerConnection& connection)
        : connection_(connection) {}

    void enqueueWrite(std::unique_ptr<folly::IOBuf> data) {
      auto& writes = connection_.bufferedWrites_;
      if (!writes) {
        writes = std::move(data);
      } else {
        writes->prependChain(std::move(data));
      }
    }

    void runLoopCallback() noexcept final {
      connection_.flushPendingWrites();
    }

    bool empty() const {
      return !connection_.bufferedWrites_;
    }

   private:
    RocketServerConnection& connection_;
  };
  BatchWriteLoopCallback batchWriteLoopCallback_{*this};
  std::unique_ptr<folly::IOBuf> bufferedWrites_;

  ~RocketServerConnection() final;

  void closeIfNeeded();
  void flushPendingWrites() {
    socket_->writeChain(this, std::move(bufferedWrites_));
  }

  void timeoutExpired() noexcept final;
  void describe(std::ostream&) const final {}
  bool isBusy() const final;
  void notifyPendingShutdown() final;
  void closeWhenIdle() final;
  void dropConnection() final;
  void dumpConnectionState(uint8_t) final {}

  friend class RocketServerFrameContext;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
