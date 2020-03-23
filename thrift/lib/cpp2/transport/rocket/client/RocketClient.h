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
#include <string>
#include <unordered_map>
#include <utility>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Overload.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>
#include <folly/Try.h>
#include <folly/container/F14Set.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContextQueue.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketStreamServerCallback.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>

namespace folly {
class IOBuf;

namespace fibers {
class FiberManager;
} // namespace fibers
} // namespace folly

namespace apache {
namespace thrift {
namespace rocket {

class RocketClientWriteCallback;

class RocketClient : public folly::DelayedDestruction,
                     private folly::AsyncTransportWrapper::WriteCallback {
 public:
  using FlushList = boost::intrusive::list<
      folly::EventBase::LoopCallback,
      boost::intrusive::constant_time_size<false>>;

  RocketClient(const RocketClient&) = delete;
  RocketClient(RocketClient&&) = delete;
  RocketClient& operator=(const RocketClient&) = delete;
  RocketClient& operator=(RocketClient&&) = delete;

  ~RocketClient() override;

  using Ptr =
      std::unique_ptr<RocketClient, folly::DelayedDestruction::Destructor>;
  static Ptr create(
      folly::EventBase& evb,
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::unique_ptr<SetupFrame> setupFrame);

  // Main send*Sync() API. Must be called on the EventBase's FiberManager.
  FOLLY_NODISCARD folly::Try<Payload> sendRequestResponseSync(
      Payload&& request,
      std::chrono::milliseconds timeout,
      RocketClientWriteCallback* writeCallback);

  FOLLY_NODISCARD folly::Try<void> sendRequestFnfSync(
      Payload&& request,
      RocketClientWriteCallback* writeCallback);

  void sendRequestStream(
      Payload&& request,
      std::chrono::milliseconds firstResponseTimeout,
      std::chrono::milliseconds chunkTimeout,
      int32_t initialRequestN,
      StreamClientCallback* clientCallback);

  void sendRequestChannel(
      Payload&& request,
      std::chrono::milliseconds firstResponseTimeout,
      ChannelClientCallback* clientCallback);

  void sendRequestSink(
      Payload&& request,
      std::chrono::milliseconds firstResponseTimeout,
      SinkClientCallback* clientCallback,
      bool pageAligned = false);

  FOLLY_NODISCARD bool sendRequestN(StreamId streamId, int32_t n);
  void cancelStream(StreamId streamId);
  void sendPayload(StreamId streamId, StreamPayload&& payload, Flags flags);
  void sendError(StreamId streamId, RocketException&& rex);
  void sendComplete(StreamId streamId, bool closeStream);
  void sendExtAlignedPage(
      StreamId streamId,
      std::unique_ptr<folly::IOBuf> payload,
      Flags flags);
  FOLLY_NODISCARD bool sendExtHeaders(
      StreamId streamId,
      HeadersPayload&& payload);

  bool streamExists(StreamId streamId) const;

  // AsyncTransportWrapper::WriteCallback implementation
  void writeSuccess() noexcept override;
  void writeErr(
      size_t bytesWritten,
      const folly::AsyncSocketException& e) noexcept override;

  void setCloseCallback(folly::Function<void()> closeCallback) {
    closeCallback_ = std::move(closeCallback);
  }

  bool isAlive() const {
    return state_ == ConnectionState::CONNECTED;
  }

  size_t streams() const {
    return streams_.size();
  }

  const folly::AsyncTransportWrapper* getTransportWrapper() const {
    return socket_.get();
  }

  folly::AsyncTransportWrapper* getTransportWrapper() {
    return socket_.get();
  }

  bool isDetachable() const {
    if (!evb_) {
      return false;
    }
    evb_->dcheckIsInEventBaseThread();

    // Client is only detachable if there are no inflight requests, no active
    // streams, and if the underlying transport is detachable, i.e., has no
    // inflight writes of its own.
    return !writeLoopCallback_.isLoopCallbackScheduled() && !requests_ &&
        streams_.empty() && (!socket_ || socket_->isDetachable()) &&
        parser_.getReadBuffer().empty();
  }

  void attachEventBase(folly::EventBase& evb);
  void detachEventBase();

  void setOnDetachable(folly::Function<void()> onDetachable) {
    onDetachable_ = std::move(onDetachable);
  }

  void notifyIfDetachable() {
    if (!onDetachable_ || !isDetachable()) {
      return;
    }
    if (detachableLoopCallback_.isLoopCallbackScheduled()) {
      return;
    }
    evb_->runInLoop(&detachableLoopCallback_);
  }

  /**
   * Set a centrilized list for flushing all pending writes.
   *
   * By default EventBase itself is used to schedule flush callback at the end
   * of the next event loop. If flushList is not null, RocketClient would attach
   * callbacks to it intstead, thus granting control over when writes happen to
   * the application.
   *
   * Note: the caller is responsible for making sure that the list outlives the
   * client.
   *
   * Note2: call to detachEventBase() would reset the list back to nullptr.
   */
  void setFlushList(FlushList* flushList) {
    flushList_ = flushList;
  }

  void scheduleTimeout(
      folly::HHWheelTimer::Callback* callback,
      const std::chrono::milliseconds& timeout) {
    if (timeout != std::chrono::milliseconds::zero()) {
      evb_->timer().scheduleTimeout(callback, timeout);
    }
  }

  // RocketClient needs to know the negotiated compression algorithm in order to
  // send compressed requests (e.g. sink)
  void setNegotiatedCompressionAlgorithm(CompressionAlgorithm compressionAlgo) {
    negotiatedCompressionAlgo_ = compressionAlgo;
  }

  void setAutoCompressSizeLimit(int32_t size) {
    autoCompressSizeLimit_ = size;
  }

  folly::Optional<CompressionAlgorithm> getNegotiatedCompressionAlgorithm() {
    return negotiatedCompressionAlgo_;
  }

  folly::Optional<int32_t> getAutoCompressSizeLimit() {
    return autoCompressSizeLimit_;
  }

 private:
  folly::EventBase* evb_;
  folly::fibers::FiberManager* fm_;
  folly::AsyncTransportWrapper::UniquePtr socket_;
  folly::Function<void()> onDetachable_;
  size_t requests_{0};
  StreamId nextStreamId_{1};
  std::unique_ptr<SetupFrame> setupFrame_;
  folly::Optional<CompressionAlgorithm> negotiatedCompressionAlgo_;
  folly::Optional<int32_t> autoCompressSizeLimit_;
  FlushList* flushList_{nullptr};
  enum class ConnectionState : uint8_t {
    CONNECTED,
    CLOSED,
    ERROR,
  };
  // Client must be constructed with an already open socket
  ConnectionState state_{ConnectionState::CONNECTED};
  transport::TTransportException error_;

  RequestContextQueue queue_;

  class FirstResponseTimeout : public folly::HHWheelTimer::Callback {
   public:
    FirstResponseTimeout(RocketClient& client, StreamId streamId)
        : client_(client), streamId_(streamId) {}

    void timeoutExpired() noexcept override;

   private:
    RocketClient& client_;
    StreamId streamId_;
  };

  struct ServerCallbackUniquePtr {
    explicit ServerCallbackUniquePtr(
        std::unique_ptr<RocketStreamServerCallback> ptr) noexcept
        : storage_(
              reinterpret_cast<intptr_t>(ptr.release()) |
              static_cast<intptr_t>(CallbackType::STREAM)) {}
    explicit ServerCallbackUniquePtr(
        std::unique_ptr<RocketStreamServerCallbackWithChunkTimeout>
            ptr) noexcept
        : storage_(
              reinterpret_cast<intptr_t>(ptr.release()) |
              static_cast<intptr_t>(CallbackType::STREAM_WITH_CHUNK_TIMEOUT)) {}
    explicit ServerCallbackUniquePtr(
        std::unique_ptr<RocketChannelServerCallback> ptr) noexcept
        : storage_(
              reinterpret_cast<intptr_t>(ptr.release()) |
              static_cast<intptr_t>(CallbackType::CHANNEL)) {}
    explicit ServerCallbackUniquePtr(
        std::unique_ptr<RocketSinkServerCallback> ptr) noexcept
        : storage_(
              reinterpret_cast<intptr_t>(ptr.release()) |
              static_cast<intptr_t>(CallbackType::SINK)) {}

    ServerCallbackUniquePtr(ServerCallbackUniquePtr&& other) noexcept
        : storage_(std::exchange(other.storage_, 0)) {}

    template <typename F>
    auto match(F&& f) const {
      switch (static_cast<CallbackType>(storage_ & kTypeMask)) {
        case CallbackType::STREAM:
          return f(reinterpret_cast<RocketStreamServerCallback*>(
              storage_ & kPointerMask));
        case CallbackType::STREAM_WITH_CHUNK_TIMEOUT:
          return f(
              reinterpret_cast<RocketStreamServerCallbackWithChunkTimeout*>(
                  storage_ & kPointerMask));
        case CallbackType::CHANNEL:
          return f(reinterpret_cast<RocketChannelServerCallback*>(
              storage_ & kPointerMask));
        case CallbackType::SINK:
          return f(reinterpret_cast<RocketSinkServerCallback*>(
              storage_ & kPointerMask));
        default:
          folly::assume_unreachable();
      };
    }

    ServerCallbackUniquePtr& operator=(
        ServerCallbackUniquePtr&& other) noexcept {
      match([](auto* ptr) { delete ptr; });
      storage_ = std::exchange(other.storage_, 0);

      return *this;
    }

    ~ServerCallbackUniquePtr() {
      match([](auto* ptr) { delete ptr; });
    }

   private:
    enum class CallbackType {
      STREAM,
      STREAM_WITH_CHUNK_TIMEOUT,
      CHANNEL,
      SINK,
    };

    static constexpr intptr_t kTypeMask = 3;
    static constexpr intptr_t kPointerMask = ~kTypeMask;

    intptr_t storage_;
  };
  struct StreamIdResolver {
    StreamId operator()(StreamId streamId) const {
      return streamId;
    }

    StreamId operator()(const ServerCallbackUniquePtr& callbackVariant) const {
      return callbackVariant.match(
          [](const auto* callback) { return callback->streamId(); });
    }
  };
  struct StreamMapHasher : private folly::f14::DefaultHasher<StreamId> {
    template <typename K>
    auto operator()(const K& key) const {
      return folly::f14::DefaultHasher<StreamId>::operator()(
          streamIdResolver_(key));
    }

   private:
    StreamIdResolver streamIdResolver_;
  };
  struct StreamMapEquals {
    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const {
      return streamIdResolver_(a) == streamIdResolver_(b);
    }

   private:
    StreamIdResolver streamIdResolver_;
  };
  using StreamMap = folly::F14FastSet<
      ServerCallbackUniquePtr,
      folly::transparent<StreamMapHasher>,
      folly::transparent<StreamMapEquals>>;
  StreamMap streams_;

  folly::F14FastMap<StreamId, Payload> bufferedFragments_;
  using FirstResponseTimeoutMap =
      folly::F14FastMap<StreamId, std::unique_ptr<FirstResponseTimeout>>;
  FirstResponseTimeoutMap firstResponseTimeouts_;

  Parser<RocketClient> parser_;

  class WriteLoopCallback : public folly::EventBase::LoopCallback {
   public:
    explicit WriteLoopCallback(RocketClient& client) : client_(client) {}
    ~WriteLoopCallback() override = default;
    void runLoopCallback() noexcept override;

   private:
    RocketClient& client_;
    bool rescheduled_{false};
  };
  WriteLoopCallback writeLoopCallback_;
  class DetachableLoopCallback : public folly::EventBase::LoopCallback {
   public:
    explicit DetachableLoopCallback(RocketClient& client) : client_(client) {}
    void runLoopCallback() noexcept override;

   private:
    RocketClient& client_;
  };
  DetachableLoopCallback detachableLoopCallback_;
  class CloseLoopCallback : public folly::EventBase::LoopCallback,
                            public folly::AsyncTransportWrapper::ReadCallback {
   public:
    explicit CloseLoopCallback(RocketClient& client) : client_(client) {}
    void runLoopCallback() noexcept override;

    // AsyncTransportWrapper::ReadCallback implementation
    void getReadBuffer(void** bufout, size_t* lenout) override;
    void readDataAvailable(size_t nbytes) noexcept override;
    void readEOF() noexcept override;
    void readErr(const folly::AsyncSocketException&) noexcept override;

   private:
    RocketClient& client_;
    bool reschedule_{false};
  };
  CloseLoopCallback closeLoopCallback_;
  class OnEventBaseDestructionCallback
      : public folly::EventBase::OnDestructionCallback {
   public:
    explicit OnEventBaseDestructionCallback(RocketClient& client)
        : client_(client) {}

    void onEventBaseDestruction() noexcept override final;

   private:
    RocketClient& client_;
  };
  OnEventBaseDestructionCallback eventBaseDestructionCallback_;
  folly::Function<void()> closeCallback_;

  RocketClient(
      folly::EventBase& evb,
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::unique_ptr<SetupFrame> setupFrame);

  template <typename Frame, typename OnError>
  FOLLY_NODISCARD bool sendFrame(Frame&& frame, OnError&& onError);

  FOLLY_NODISCARD folly::Try<void>
  sendPayloadSync(StreamId streamId, Payload&& payload, Flags flags);
  FOLLY_NODISCARD folly::Try<void> sendErrorSync(
      StreamId streamId,
      RocketException&& rex);

  FOLLY_NODISCARD folly::Try<void> scheduleWrite(RequestContext& ctx);

  StreamId makeStreamId() {
    const StreamId rid = nextStreamId_;
    // rsocket protocol specifies that clients must generate odd stream IDs
    nextStreamId_ += 2;
    return rid;
  }

  template <typename ServerCallback>
  void sendRequestStreamChannel(
      const StreamId& streamId,
      Payload&& request,
      std::chrono::milliseconds firstResponseTimeout,
      int32_t initialRequestN,
      std::unique_ptr<ServerCallback> serverCallback);

  void freeStream(StreamId streamId);

  void handleFrame(std::unique_ptr<folly::IOBuf> frame);
  void handleRequestResponseFrame(
      RequestContext& ctx,
      FrameType frameType,
      std::unique_ptr<folly::IOBuf> frame);
  void handleStreamChannelFrame(
      StreamId streamId,
      FrameType frameType,
      std::unique_ptr<folly::IOBuf> frame);

  template <typename CallbackType>
  StreamChannelStatus handlePayloadFrame(
      CallbackType& serverCallback,
      std::unique_ptr<folly::IOBuf> frame);

  template <typename CallbackType>
  StreamChannelStatus handleErrorFrame(
      CallbackType& serverCallback,
      std::unique_ptr<folly::IOBuf> frame);

  template <typename CallbackType>
  StreamChannelStatus handleRequestNFrame(
      CallbackType& serverCallback,
      std::unique_ptr<folly::IOBuf> frame);

  template <typename CallbackType>
  StreamChannelStatus handleCancelFrame(
      CallbackType& serverCallback,
      std::unique_ptr<folly::IOBuf> frame);

  template <typename CallbackType>
  StreamChannelStatus handleExtFrame(
      CallbackType& serverCallback,
      std::unique_ptr<folly::IOBuf> frame);

  void writeScheduledRequestsToSocket() noexcept;

  void scheduleFirstResponseTimeout(
      StreamId streamId,
      std::chrono::milliseconds timeout);
  folly::Optional<Payload> bufferOrGetFullPayload(PayloadFrame&& payloadFrame);

  FOLLY_NODISCARD auto makeRequestCountGuard() {
    ++requests_;
    return folly::makeGuard([this] {
      if (!--requests_) {
        notifyIfDetachable();
      }
    });
  }

  // Request connection close and fail all the requests.
  void close(apache::thrift::transport::TTransportException ex) noexcept;
  // Close the connection and fail all the requests *inline*. This should not be
  // called inline from any of the callbacks triggered by RocketClient.
  void closeNow(apache::thrift::transport::TTransportException ex) noexcept;

  bool setError(apache::thrift::transport::TTransportException ex) noexcept;
  void closeNowImpl() noexcept;

  template <class T>
  friend class Parser;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
