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
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/net/NetOps.h>

#include <wangle/acceptor/ManagedConnection.h>

#include <thrift/lib/cpp2/async/MessageChannel.h>
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

class RocketServerConnection final
    : public wangle::ManagedConnection,
      private folly::AsyncTransport::WriteCallback {
 public:
  using UniquePtr = std::
      unique_ptr<RocketServerConnection, folly::DelayedDestruction::Destructor>;

  RocketServerConnection(
      folly::AsyncTransport::UniquePtr socket,
      std::unique_ptr<RocketServerHandler> frameHandler,
      std::chrono::milliseconds streamStarvationTimeout,
      std::chrono::milliseconds writeBatchingInterval =
          std::chrono::milliseconds::zero(),
      size_t writeBatchingSize = 0);

  void send(
      std::unique_ptr<folly::IOBuf> data,
      apache::thrift::MessageChannel::SendCallbackPtr cb = nullptr);

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

  // AsyncTransport::WriteCallback implementation
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

  void sendPayload(
      StreamId streamId,
      Payload&& payload,
      Flags flags,
      apache::thrift::MessageChannel::SendCallbackPtr cb = nullptr);
  void sendError(
      StreamId streamId,
      RocketException&& rex,
      apache::thrift::MessageChannel::SendCallbackPtr cb = nullptr);
  void sendRequestN(StreamId streamId, int32_t n);
  void sendCancel(StreamId streamId);
  void sendExt(
      StreamId streamId,
      Payload&& payload,
      Flags flags,
      ExtFrameType extFrameType);

  void freeStream(StreamId streamId, bool markRequestComplete);

  void scheduleStreamTimeout(folly::HHWheelTimer::Callback*);
  void scheduleSinkTimeout(
      folly::HHWheelTimer::Callback*,
      std::chrono::milliseconds timeout);

  void incInflightFinalResponse() {
    inflightSinkFinalResponses_++;
  }
  void decInflightFinalResponse() {
    DCHECK(inflightSinkFinalResponses_ != 0);
    inflightSinkFinalResponses_--;
    closeIfNeeded();
  }

 private:
  // Note that attachEventBase()/detachEventBase() are not supported in server
  // code
  folly::EventBase& evb_;
  folly::AsyncTransport::UniquePtr socket_;

  Parser<RocketServerConnection> parser_{*this};
  std::unique_ptr<RocketServerHandler> frameHandler_;
  bool setupFrameReceived_{false};
  folly::F14NodeMap<
      StreamId,
      boost::variant<
          RequestResponseFrame,
          RequestFnfFrame,
          RequestStreamFrame,
          RequestChannelFrame>>
      partialRequestFrames_;
  folly::F14FastMap<StreamId, Payload> bufferedFragments_;

  // Total number of active Request* frames ("streams" in protocol parlance)
  size_t inflightRequests_{0};

  // Context for each inflight write to the underlying transport.
  struct WriteBatchContext {
    // the counts of completed requests in each inflight write
    size_t requestCompleteCount{0};
    // the counts of valid sendCallbacks in each inflight write
    std::vector<apache::thrift::MessageChannel::SendCallbackPtr> sendCallbacks;
  };
  // The size of the queue is equal to the total number of inflight writes to
  // the underlying transport, i.e., writes for which the
  // writeSuccess()/writeErr() has not yet been called.
  std::queue<WriteBatchContext> inflightWritesQueue_;
  // Totoal number of inflight final response for sink semantic, the counter
  // only bumps when sink is in waiting for final response state,
  // (onSinkComplete get called)
  size_t inflightSinkFinalResponses_{0};

  folly::Optional<CompressionAlgorithm> negotiatedCompressionAlgo_;

  enum class ConnectionState : uint8_t {
    ALIVE,
    DRAINING, // Rejecting all new requests, waiting for inflight requests to
              // complete.
    CLOSING, // No longer reading form the socket, waiting for pending writes to
             // complete.
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

    void enqueueWrite(
        std::unique_ptr<folly::IOBuf> data,
        apache::thrift::MessageChannel::SendCallbackPtr cb) {
      if (cb) {
        cb->sendQueued();
        bufferedWritesContext_.sendCallbacks.push_back(std::move(cb));
      }
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

    void enqueueRequestComplete() {
      DCHECK(!empty());
      bufferedWritesContext_.requestCompleteCount++;
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
      connection_.flushWrites(
          std::move(bufferedWrites_),
          std::exchange(bufferedWritesContext_, WriteBatchContext{}));
    }

    RocketServerConnection& connection_;
    std::chrono::milliseconds batchingInterval_;
    size_t batchingSize_;
    // Callback is scheduled iff bufferedWrites_ is not empty.
    std::unique_ptr<folly::IOBuf> bufferedWrites_;
    size_t bufferedWritesCount_{0};
    WriteBatchContext bufferedWritesContext_;
  };
  WriteBatcher writeBatcher_;
  class SocketDrainer : private folly::HHWheelTimer::Callback {
   public:
    explicit SocketDrainer(RocketServerConnection& connection)
        : connection_(connection) {}

    void activate() {
      deadline_ = std::chrono::steady_clock::now() + kTimeout;
      tryClose();
    }

   private:
    void tryClose() {
      if (shouldClose()) {
        connection_.destroy();
        return;
      }
      connection_.evb_.timer().scheduleTimeout(this, kRetryInterval);
    }

    bool shouldClose() {
      if (std::chrono::steady_clock::now() >= deadline_) {
        return true;
      }

#if defined(__linux__)
      if (auto socket =
              dynamic_cast<folly::AsyncSocket*>(connection_.socket_.get())) {
        tcp_info info;
        socklen_t infolen = sizeof(info);
        if (socket->getSockOpt(IPPROTO_TCP, TCP_INFO, &info, &infolen)) {
          return true;
        }
        DCHECK(infolen == sizeof(info));
        return info.tcpi_unacked == 0;
      }
#endif

      return true;
    }

    void timeoutExpired() noexcept final {
      tryClose();
    }

    RocketServerConnection& connection_;
    std::chrono::steady_clock::time_point deadline_;
    static constexpr std::chrono::milliseconds kRetryInterval{50};
    static constexpr std::chrono::seconds kTimeout{1};
  };
  SocketDrainer socketDrainer_;

  ~RocketServerConnection();

  void closeIfNeeded();
  void flushWrites(
      std::unique_ptr<folly::IOBuf> writes,
      WriteBatchContext&& context) {
    inflightWritesQueue_.push(std::move(context));
    socket_->writeChain(this, std::move(writes));
  }

  void timeoutExpired() noexcept final;
  void describe(std::ostream&) const final {}
  bool isBusy() const final;
  void notifyPendingShutdown() final;
  void closeWhenIdle() final;
  void dropConnection(const std::string& errorMsg = "") final;
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
  template <typename RequestFrame>
  void handleRequestFrame(RequestFrame&& frame) {
    auto streamId = frame.streamId();
    if (UNLIKELY(frame.hasFollows())) {
      partialRequestFrames_.emplace(
          streamId, std::forward<RequestFrame>(frame));
    } else {
      RocketServerFrameContext(*this, streamId)
          .onFullFrame(std::forward<RequestFrame>(frame));
    }
  }

  void incInflightRequests() {
    ++inflightRequests_;
  }

  void decInflightRequests() {
    --inflightRequests_;
    closeIfNeeded();
  }

  void requestComplete() {
    if (!writeBatcher_.empty()) {
      writeBatcher_.enqueueRequestComplete();
      return;
    }
    if (!inflightWritesQueue_.empty()) {
      inflightWritesQueue_.back().requestCompleteCount++;
      return;
    }
    frameHandler_->requestComplete();
  }

  friend class RocketServerFrameContext;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
