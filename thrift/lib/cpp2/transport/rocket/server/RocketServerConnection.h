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
      std::chrono::milliseconds streamStarvationTimeout,
      std::chrono::milliseconds writeBatchingInterval =
          std::chrono::milliseconds::zero(),
      size_t writeBatchingSize = 0);

  void send(std::unique_ptr<folly::IOBuf> data);

  RocketStreamClientCallback& createStreamClientCallback(
      StreamId streamId,
      RocketServerConnection& connection,
      uint32_t initialRequestN);

  RocketSinkClientCallback& createSinkClientCallback(
      StreamId streamId,
      RocketServerConnection& connection);

  // Parser callbacks
  void handleFrame(std::unique_ptr<folly::IOBuf> frame);
  void close(folly::exception_wrapper ew);

  void incInflightRequests() {
    ++inflightRequests_;
  }

  // return true if connection still alive (not closed)
  bool decInflightRequests() {
    DCHECK(inflightRequests_ != 0);
    --inflightRequests_;
    return !closeIfNeeded();
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

  void sendPayload(StreamId streamId, Payload&& payload, Flags flags);
  void sendError(StreamId streamId, RocketException&& rex);
  void sendRequestN(StreamId streamId, int32_t n);
  void sendCancel(StreamId streamId);
  void sendExt(
      StreamId streamId,
      Payload&& payload,
      Flags flags,
      ExtFrameType extFrameType);

  void freeStream(StreamId streamId);

  void scheduleStreamTimeout(folly::HHWheelTimer::Callback*);
  void scheduleSinkTimeout(
      folly::HHWheelTimer::Callback*,
      std::chrono::milliseconds timeout);

 private:
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

  class WriteBatcher : private folly::EventBase::LoopCallback,
                       private folly::HHWheelTimer::Callback {
   public:
    WriteBatcher(
        RocketServerConnection& connection,
        std::chrono::milliseconds batchingInterval,
        size_t batchingSize)
        : connection_(connection),
          batchingInterval_(batchingInterval),
          batchingSize_(batchingSize) {}

    void enqueueWrite(std::unique_ptr<folly::IOBuf> data) {
      if (!bufferedWrites_) {
        bufferedWrites_ = std::move(data);
        if (batchingInterval_ != std::chrono::milliseconds::zero()) {
          connection_.getEventBase().timer().scheduleTimeout(
              this, batchingInterval_);
        } else {
          connection_.getEventBase().runInLoop(this, true /* thisIteration */);
        }
      } else {
        bufferedWrites_->prependChain(std::move(data));
      }
      ++bufferedWritesCount_;
      if (batchingInterval_ != std::chrono::milliseconds::zero() &&
          bufferedWritesCount_ == batchingSize_) {
        cancelTimeout();
        connection_.getEventBase().runInLoop(this, true /* thisIteration */);
      }
    }

    void drain() noexcept {
      if (!bufferedWrites_) {
        return;
      }
      cancelLoopCallback();
      cancelTimeout();
      flushPendingWrites();
    }

    bool empty() const {
      return !bufferedWrites_;
    }

   private:
    void runLoopCallback() noexcept final {
      flushPendingWrites();
    }

    void timeoutExpired() noexcept final {
      flushPendingWrites();
    }

    void flushPendingWrites() noexcept {
      bufferedWritesCount_ = 0;
      connection_.flushWrites(std::move(bufferedWrites_));
    }

    RocketServerConnection& connection_;
    std::chrono::milliseconds batchingInterval_;
    size_t batchingSize_;
    // Callback is scheduled iff bufferedWrites_ is not empty.
    std::unique_ptr<folly::IOBuf> bufferedWrites_;
    size_t bufferedWritesCount_{0};
  };
  WriteBatcher writeBatcher_;

  ~RocketServerConnection() final;

  // return true if connection closed
  bool closeIfNeeded();
  void flushWrites(std::unique_ptr<folly::IOBuf> writes) {
    ++inflightWrites_;
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

  friend class RocketServerFrameContext;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
