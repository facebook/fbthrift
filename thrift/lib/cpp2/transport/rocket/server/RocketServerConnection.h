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

#include <chrono>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <utility>

#include <boost/variant.hpp>

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
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class RocketSinkClientCallback;
class RocketStreamClientCallback;

namespace rocket {

class RocketServerConnection
    : public wangle::ManagedConnection,
      private folly::AsyncTransportWrapper::WriteCallback {
 public:
  using UniquePtr = std::
      unique_ptr<RocketServerConnection, folly::DelayedDestruction::Destructor>;

  RocketServerConnection(
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::shared_ptr<RocketServerHandler> frameHandler,
      std::chrono::milliseconds streamStarvationTimeout);

  void send(std::unique_ptr<folly::IOBuf> data);

  static RocketStreamClientCallback* createStreamClientCallback(
      RocketServerFrameContext&& context,
      uint32_t initialRequestN);

  static RocketSinkClientCallback* createSinkClientCallback(
      RocketServerFrameContext&& context);

  // Parser callbacks
  void handleFrame(std::unique_ptr<folly::IOBuf> frame);
  void close(folly::exception_wrapper ew);

  void onContextConstructed() {
    ++inflightRequests_;
  }

  void onContextDestroyed() {
    DCHECK(inflightRequests_ != 0);
    --inflightRequests_;
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

  size_t getNumStreams() const {
    return streams_.size();
  }

  void setNegotiatedCompressionAlgorithm(CompressionAlgorithm compressionAlgo) {
    negotiatedCompressionAlgo_ = compressionAlgo;
  }

  folly::Optional<CompressionAlgorithm> getNegotiatedCompressionAlgorithm() {
    return negotiatedCompressionAlgo_;
  }

  void setMinCompressBytes(uint32_t bytes) {
    minCompressBytes_ = bytes;
  }

  /**
   * Get the minimum response compression size
   */
  uint32_t getMinCompressBytes() const {
    return minCompressBytes_;
  }

 private:
  void freeStream(StreamId streamId);

  // Note that attachEventBase()/detachEventBase() are not supported in server
  // code
  folly::EventBase& evb_;
  folly::AsyncTransportWrapper::UniquePtr socket_;

  Parser<RocketServerConnection> parser_{*this};
  const std::shared_ptr<RocketServerHandler> frameHandler_;
  bool setupFrameReceived_{false};
  folly::F14NodeMap<StreamId, RocketServerPartialFrameContext>
      partialRequestFrames_;
  folly::F14FastMap<StreamId, Payload> bufferedFragments_;

  // Total number of active Request* frames ("streams" in protocol parlance)
  size_t inflightRequests_{0};
  // Total number of inflight writes to the underlying transport, i.e., writes
  // for which the writeSuccess()/writeErr() has not yet been called.
  size_t inflightWrites_{0};
  folly::Optional<CompressionAlgorithm> negotiatedCompressionAlgo_;
  uint32_t minCompressBytes_{0};

  enum class ConnectionState : uint8_t {
    ALIVE,
    CLOSING,
    CLOSED,
  };
  ConnectionState state_{ConnectionState::ALIVE};

  using ClientCallbackUniquePtr = boost::variant<
      std::unique_ptr<RocketStreamClientCallback>,
      std::unique_ptr<RocketSinkClientCallback>>;
  using ClientCallbackPtr =
      boost::variant<RocketStreamClientCallback*, RocketSinkClientCallback*>;
  folly::F14FastMap<StreamId, ClientCallbackUniquePtr> streams_;
  const std::chrono::milliseconds streamStarvationTimeout_;

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
    ++inflightWrites_;
    auto writes = std::move(bufferedWrites_);
    socket_->writeChain(this, std::move(writes));
  }

  void timeoutExpired() noexcept final;
  void describe(std::ostream&) const final {}
  bool isBusy() const final;
  void notifyPendingShutdown() final;
  void closeWhenIdle() final;
  void dropConnection() final;
  void dumpConnectionState(uint8_t) final {}
  folly::Optional<Payload> bufferOrGetFullPayload(PayloadFrame&& payloadFrame);

  void handleUntrackedFrame(
      std::unique_ptr<folly::IOBuf> frame,
      StreamId streamId,
      FrameType frameType,
      Flags flags,
      folly::io::Cursor cursor);
  void handleStreamFrame(
      std::unique_ptr<folly::IOBuf> frame,
      StreamId streamId,
      FrameType frameType,
      Flags flags,
      folly::io::Cursor cursor,
      RocketStreamClientCallback& clientCallback);
  void handleSinkFrame(
      std::unique_ptr<folly::IOBuf> frame,
      StreamId streamId,
      FrameType frameType,
      Flags flags,
      folly::io::Cursor cursor,
      RocketSinkClientCallback& clientCallback);

  void scheduleStreamTimeout(folly::HHWheelTimer::Callback*);
  void scheduleSinkTimeout(
      folly::HHWheelTimer::Callback*,
      std::chrono::milliseconds timeout);

  friend class RocketServerFrameContext;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
