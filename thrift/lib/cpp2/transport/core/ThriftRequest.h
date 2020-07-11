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

#include <stdint.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <glog/logging.h>

#include <folly/Portability.h>

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#if FOLLY_HAS_COROUTINES
#include <thrift/lib/cpp2/async/Sink.h>
#endif
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/ServerConfigs.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

/**
 * Manages per-RPC state.  There is one of these objects for each RPC.
 *
 * TODO: RSocket currently has a dependency to this class. We may want
 * to clean up our APIs to avoid the dependency to a ResponseChannel
 * object.
 */
class ThriftRequestCore : public ResponseChannelRequest {
 public:
  ThriftRequestCore(
      server::ServerConfigs& serverConfigs,
      RequestRpcMetadata&& metadata,
      Cpp2ConnContext& connContext)
      : serverConfigs_(serverConfigs),
        kind_(metadata.kind_ref().value_or(
            RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE)),
        active_(true),
        checksumRequested_(metadata.crc32c_ref().has_value()),
        requestFlags_(metadata.flags_ref().value_or(0)),
        loadMetric_(
            metadata.loadMetric_ref()
                ? folly::make_optional(std::move(*metadata.loadMetric_ref()))
                : folly::none),
        reqContext_(&connContext, &header_),
        queueTimeout_(serverConfigs_),
        taskTimeout_(serverConfigs_) {
    // Note that method name, RPC kind, and serialization protocol are validated
    // outside the ThriftRequestCore constructor.
    header_.setProtocolId(static_cast<int16_t>(
        metadata.protocol_ref().value_or(ProtocolId::BINARY)));

    if (auto clientTimeoutMs = metadata.clientTimeoutMs_ref()) {
      clientTimeout_ = std::chrono::milliseconds(*clientTimeoutMs);
      header_.setClientTimeout(clientTimeout_);
    }
    if (auto queueTimeoutMs = metadata.queueTimeoutMs_ref()) {
      clientQueueTimeout_ = std::chrono::milliseconds(*queueTimeoutMs);
      header_.setClientQueueTimeout(clientQueueTimeout_);
    }
    if (auto priority = metadata.priority_ref()) {
      header_.setCallPriority(static_cast<concurrency::PRIORITY>(*priority));
    }
    if (auto otherMetadata = metadata.otherMetadata_ref()) {
      header_.setReadHeaders(std::move(*otherMetadata));
    }

    // Store client's compression configs (if client explicitly requested
    // compression codec and size limit, use these settings to compress
    // response)
    if (auto compressionConfig = metadata.compressionConfig_ref()) {
      compressionConfig_ = *compressionConfig;
    }

    if (auto methodName = metadata.name_ref()) {
      reqContext_.setMethodName(std::move(*methodName));
    }

    if (auto* observer = serverConfigs_.getObserver()) {
      observer->receivedRequest();
    }
  }

  ~ThriftRequestCore() override {
    cancelTimeout();
  }

  bool isActive() const final {
    return active_.load();
  }

  void cancel() override {
    if (active_.exchange(false)) {
      cancelTimeout();
    }
  }

  RpcKind kind() const {
    return kind_;
  }

  bool isOneway() const final {
    return kind_ == RpcKind::SINGLE_REQUEST_NO_RESPONSE ||
        kind_ == RpcKind::STREAMING_REQUEST_NO_RESPONSE;
  }

  protocol::PROTOCOL_TYPES getProtoId() const {
    return static_cast<protocol::PROTOCOL_TYPES>(header_.getProtocolId());
  }

  Cpp2RequestContext* getRequestContext() {
    return &reqContext_;
  }

  const std::string& getMethodName() const {
    return reqContext_.getMethodName();
  }

  const folly::Optional<CompressionConfig>& getCompressionConfig() {
    return compressionConfig_;
  }

  // RequestTimestampSample is a wrapper for sampled requests
  class RequestTimestampSample : public MessageChannel::SendCallback {
   public:
    RequestTimestampSample(
        server::TServerObserver::CallTimestamps& timestamps,
        server::TServerObserver* observer,
        MessageChannel::SendCallback* chainedCallback = nullptr);

    void sendQueued() override;
    void messageSent() override;
    void messageSendError(folly::exception_wrapper&& e) override;
    ~RequestTimestampSample() override;

   private:
    server::TServerObserver::CallTimestamps timestamps_;
    server::TServerObserver* observer_;
    MessageChannel::SendCallback* chainedCallback_;
  };

  void sendReply(
      std::unique_ptr<folly::IOBuf>&& buf,
      apache::thrift::MessageChannel::SendCallback* cb,
      folly::Optional<uint32_t> crc32c) override final;

  bool sendStreamReply(
      std::unique_ptr<folly::IOBuf> response,
      StreamServerCallbackPtr stream,
      folly::Optional<uint32_t> crc32c) override final {
    if (active_.exchange(false)) {
      cancelTimeout();
      auto metadata = makeResponseRpcMetadata(header_.extractAllWriteHeaders());
      if (crc32c) {
        metadata.crc32c_ref() = *crc32c;
      }
      auto alive = sendReplyInternal(
          std::move(metadata), std::move(response), std::move(stream));

      if (auto* observer = serverConfigs_.getObserver()) {
        observer->sentReply();
      }
      return alive;
    }
    return false;
  }

  void sendStreamReply(
      std::unique_ptr<folly::IOBuf>&& buf,
      detail::ServerStreamFactory&& stream,
      folly::Optional<uint32_t> crc32c) override final {
    if (active_.exchange(false)) {
      cancelTimeout();
      auto metadata = makeResponseRpcMetadata(header_.extractAllWriteHeaders());
      if (crc32c) {
        metadata.crc32c_ref() = *crc32c;
      }
      sendReplyInternal(std::move(metadata), std::move(buf), std::move(stream));

      if (auto* observer = serverConfigs_.getObserver()) {
        observer->sentReply();
      }
    }
  }

#if FOLLY_HAS_COROUTINES
  void sendSinkReply(
      std::unique_ptr<folly::IOBuf>&& buf,
      detail::SinkConsumerImpl&& consumerImpl,
      folly::Optional<uint32_t> crc32c) override final {
    if (active_.exchange(false)) {
      cancelTimeout();
      auto metadata = makeResponseRpcMetadata(header_.extractAllWriteHeaders());
      if (crc32c) {
        metadata.crc32c_ref() = *crc32c;
      }
      sendReplyInternal(
          std::move(metadata), std::move(buf), std::move(consumerImpl));

      if (auto* observer = serverConfigs_.getObserver()) {
        observer->sentReply();
      }
    }
  }
#endif

  void sendErrorWrapped(folly::exception_wrapper ew, std::string exCode) final {
    if (active_.exchange(false)) {
      cancelTimeout();
      sendErrorWrappedInternal(
          std::move(ew), exCode, header_.extractAllWriteHeaders());
    }
  }

  bool isReplyChecksumNeeded() const override {
    return checksumRequested_;
  }

 protected:
  virtual void sendThriftResponse(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> response,
      MessageChannel::SendCallbackPtr) noexcept = 0;

  virtual void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept = 0;

  virtual bool sendStreamThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      StreamServerCallbackPtr) noexcept {
    folly::terminate_with<std::runtime_error>(
        "sendStreamThriftResponse not implemented");
  }

  virtual void sendStreamThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      detail::ServerStreamFactory&&) noexcept {
    LOG(FATAL) << "sendStreamThriftResponse not implemented";
  }

#if FOLLY_HAS_COROUTINES
  virtual void sendSinkThriftResponse(
      ResponseRpcMetadata&&,
      std::unique_ptr<folly::IOBuf>,
      detail::SinkConsumerImpl&&) noexcept {
    LOG(FATAL) << "sendSinkThriftResponse not implemented";
  }
#endif

  virtual folly::EventBase* getEventBase() noexcept = 0;

  void scheduleTimeouts() {
    queueTimeout_.request_ = this;
    taskTimeout_.request_ = this;
    std::chrono::milliseconds queueTimeout;
    std::chrono::milliseconds taskTimeout;
    auto differentTimeouts = serverConfigs_.getTaskExpireTimeForRequest(
        clientQueueTimeout_, clientTimeout_, queueTimeout, taskTimeout);

    auto reqContext = getRequestContext();
    reqContext->setRequestTimeout(taskTimeout);

    if (differentTimeouts) {
      if (queueTimeout > std::chrono::milliseconds(0)) {
        getEventBase()->timer().scheduleTimeout(&queueTimeout_, queueTimeout);
      }
    }
    if (taskTimeout > std::chrono::milliseconds(0)) {
      getEventBase()->timer().scheduleTimeout(&taskTimeout_, taskTimeout);
    }
  }

 private:
  MessageChannel::SendCallbackPtr prepareSendCallback(
      MessageChannel::SendCallbackPtr&& sendCallback,
      server::TServerObserver* observer);

  void sendReplyInternal(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> buf,
      MessageChannel::SendCallbackPtr cb);

  bool sendReplyInternal(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> buf,
      StreamServerCallbackPtr stream) {
    if (!checkResponseSize(*buf)) {
      sendResponseTooBigEx();
      return false;
    }
    return sendStreamThriftResponse(
        std::move(metadata), std::move(buf), std::move(stream));
  }

  void sendReplyInternal(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> buf,
      detail::ServerStreamFactory&& stream) {
    if (checkResponseSize(*buf)) {
      sendStreamThriftResponse(
          std::move(metadata), std::move(buf), std::move(stream));
    } else {
      sendResponseTooBigEx();
    }
  }

#if FOLLY_HAS_COROUTINES
  void sendReplyInternal(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> buf,
      detail::SinkConsumerImpl sink) {
    if (checkResponseSize(*buf)) {
      sendSinkThriftResponse(
          std::move(metadata), std::move(buf), std::move(sink));
    } else {
      sendResponseTooBigEx();
    }
  }
#endif

  void sendResponseTooBigEx() {
    sendErrorWrappedInternal(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::INTERNAL_ERROR,
            "Response size too big"),
        kResponseTooBigErrorCode,
        header_.extractAllWriteHeaders());
  }

  ResponseRpcMetadata makeResponseRpcMetadata(
      transport::THeader::StringToStringMap&& writeHeaders) {
    ResponseRpcMetadata metadata;

    if ((requestFlags_ &
         static_cast<uint64_t>(RequestRpcMetadataFlags::QUERY_SERVER_LOAD)) ||
        loadMetric_) {
      metadata.load_ref() = serverConfigs_.getLoad(
          loadMetric_.value_or(transport::THeader::QUERY_LOAD_HEADER));
    }

    if (!writeHeaders.empty()) {
      metadata.otherMetadata_ref() = std::move(writeHeaders);
    }

    return metadata;
  }

  void sendErrorWrappedInternal(
      folly::exception_wrapper ew,
      const std::string& exCode,
      transport::THeader::StringToStringMap&& writeHeaders) {
    DCHECK(ew.is_compatible_with<TApplicationException>());
    writeHeaders["ex"] = exCode;
    ew.with_exception([&](TApplicationException& tae) {
      std::unique_ptr<folly::IOBuf> exbuf;
      auto proto = getProtoId();
      try {
        exbuf = serializeError(proto, tae, getMethodName(), 0);
      } catch (const protocol::TProtocolException& pe) {
        // Should never happen.  Log an error and return an empty
        // payload.
        LOG(ERROR) << "serializeError failed. type=" << pe.getType()
                   << " what()=" << pe.what();
      }

      if (tae.getType() ==
              TApplicationException::TApplicationExceptionType::UNKNOWN &&
          exbuf && !checkResponseSize(*exbuf)) {
        sendResponseTooBigEx();
        return;
      }

      sendSerializedError(
          makeResponseRpcMetadata(std::move(writeHeaders)), std::move(exbuf));
    });
  }

  void cancelTimeout() {
    queueTimeout_.cancelTimeout();
    taskTimeout_.cancelTimeout();
  }

  bool checkResponseSize(const folly::IOBuf& buf) {
    auto maxResponseSize = serverConfigs_.getMaxResponseSize();
    return maxResponseSize == 0 ||
        buf.computeChainDataLength() <= maxResponseSize;
  }

  class QueueTimeout : public folly::HHWheelTimer::Callback {
    ThriftRequestCore* request_;
    const server::ServerConfigs& serverConfigs_;
    QueueTimeout(const server::ServerConfigs& serverConfigs)
        : serverConfigs_(serverConfigs) {}
    void timeoutExpired() noexcept override {
      if (!request_->getStartedProcessing() &&
          request_->active_.exchange(false) && !request_->isOneway()) {
        if (auto* observer = serverConfigs_.getObserver()) {
          observer->queueTimeout();
        }
        request_->sendErrorWrappedInternal(
            TApplicationException(
                TApplicationException::TApplicationExceptionType::TIMEOUT,
                "Queue Timeout"),
            kServerQueueTimeoutErrorCode,
            {});
      }
    }
    friend class ThriftRequestCore;
  };
  class TaskTimeout : public folly::HHWheelTimer::Callback {
    ThriftRequestCore* request_;
    const server::ServerConfigs& serverConfigs_;
    TaskTimeout(const server::ServerConfigs& serverConfigs)
        : serverConfigs_(serverConfigs) {}
    void timeoutExpired() noexcept override {
      if (request_->active_.exchange(false) && !request_->isOneway()) {
        if (auto* observer = serverConfigs_.getObserver()) {
          observer->taskTimeout();
        }
        request_->sendErrorWrappedInternal(
            TApplicationException(
                TApplicationException::TApplicationExceptionType::TIMEOUT,
                "Task expired"),
            kTaskExpiredErrorCode,
            {});
      }
    }
    friend class ThriftRequestCore;
  };
  friend class QueueTimeout;
  friend class TaskTimeout;
  friend class ThriftProcessor;

  server::TServerObserver::CallTimestamps& getTimestamps() {
    return static_cast<server::TServerObserver::CallTimestamps&>(
        reqContext_.getTimestamps());
  }

 protected:
  server::ServerConfigs& serverConfigs_;
  const RpcKind kind_;

 private:
  std::atomic<bool> active_;
  bool checksumRequested_{false};
  transport::THeader header_;
  const uint64_t requestFlags_{0};
  folly::Optional<std::string> loadMetric_;
  Cpp2RequestContext reqContext_;
  folly::Optional<CompressionConfig> compressionConfig_;

  QueueTimeout queueTimeout_;
  TaskTimeout taskTimeout_;
  std::chrono::milliseconds clientQueueTimeout_{0};
  std::chrono::milliseconds clientTimeout_{0};
};

// HTTP2 uses this
class ThriftRequest final : public ThriftRequestCore {
 public:
  ThriftRequest(
      server::ServerConfigs& serverConfigs,
      std::shared_ptr<ThriftChannelIf> channel,
      RequestRpcMetadata&& metadata,
      std::unique_ptr<Cpp2ConnContext> connContext)
      : ThriftRequestCore(
            serverConfigs,
            std::move(metadata),
            [&]() -> Cpp2ConnContext& {
              if (!connContext) {
                connContext = std::make_unique<Cpp2ConnContext>();
              }
              return *connContext;
            }()),
        channel_(std::move(channel)),
        connContext_(std::move(connContext)) {
    serverConfigs_.incActiveRequests();
    scheduleTimeouts();
  }

  ~ThriftRequest() {
    serverConfigs_.decActiveRequests();
  }

 private:
  void sendThriftResponse(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> response,
      MessageChannel::SendCallbackPtr) noexcept override {
    channel_->sendThriftResponse(std::move(metadata), std::move(response));
  }

  void sendSerializedError(
      ResponseRpcMetadata&& metadata,
      std::unique_ptr<folly::IOBuf> exbuf) noexcept override {
    switch (kind_) {
      case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      case RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE:
        sendThriftResponse(std::move(metadata), std::move(exbuf), nullptr);
        break;
      case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      case RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE:
        sendStreamThriftResponse(
            std::move(metadata),
            std::move(exbuf),
            StreamServerCallbackPtr(nullptr));
        break;
#if FOLLY_HAS_COROUTINES
      case RpcKind::SINK:
        sendSinkThriftResponse(std::move(metadata), std::move(exbuf), {});
        break;
#endif
      default: // Don't send error back for one-way.
        LOG(ERROR) << "unknown rpckind " << static_cast<int32_t>(kind_);
        break;
    }
  }

  // Don't allow hiding of overloaded method.
  using ThriftRequestCore::sendStreamThriftResponse;

  folly::EventBase* getEventBase() noexcept override {
    return channel_->getEventBase();
  }

 private:
  std::shared_ptr<ThriftChannelIf> channel_;
  std::unique_ptr<Cpp2ConnContext> connContext_;
};

} // namespace thrift
} // namespace apache
