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

#ifndef THRIFT_ASYNC_CPP2CONNECTION_H_
#define THRIFT_ASYNC_CPP2CONNECTION_H_ 1

#include <memory>
#include <unordered_set>

#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/HHWheelTimer.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <wangle/acceptor/ManagedConnection.h>

namespace apache {
namespace thrift {

constexpr folly::StringPiece kQueueLatencyHeader("queue_latency_us");
constexpr folly::StringPiece kProcessLatencyHeader("process_latency_us");

/**
 * Represents a connection that is handled via libevent. This connection
 * essentially encapsulates a socket that has some associated libevent state.
 */
class Cpp2Connection : public HeaderServerChannel::Callback,
                       public wangle::ManagedConnection {
 public:
  /**
   * Constructor for Cpp2Connection.
   *
   * @param asyncSocket shared pointer to the async socket
   * @param address the peer address of this connection
   * @param worker the worker instance that is handling this connection
   * @param serverChannel server channel to use in duplex mode,
   *        should be nullptr in normal mode
   */
  Cpp2Connection(
      const std::shared_ptr<folly::AsyncTransport>& transport,
      const folly::SocketAddress* address,
      std::shared_ptr<Cpp2Worker> worker,
      const std::shared_ptr<HeaderServerChannel>& serverChannel = nullptr);

  /// Destructor -- close down the connection.
  ~Cpp2Connection() override;

  // HeaderServerChannel callbacks
  void requestReceived(
      std::unique_ptr<HeaderServerChannel::HeaderRequest>&&) override;
  void channelClosed(folly::exception_wrapper&&) override;

  void start() {
    channel_->setCallback(this);
  }

  void stop();

  void timeoutExpired() noexcept override;

  void requestTimeoutExpired();

  void queueTimeoutExpired();

  bool pending();

  // Managed Connection callbacks
  void describe(std::ostream&) const override {}
  bool isBusy() const override {
    return activeRequests_.empty();
  }
  void notifyPendingShutdown() override {}
  void closeWhenIdle() override {
    stop();
  }
  void dropConnection(const std::string& /* errorMsg */ = "") override {
    stop();
  }
  void dumpConnectionState(uint8_t /* loglevel */) override {}
  void addConnection(std::shared_ptr<Cpp2Connection> conn) {
    this_ = conn;
  }

  void setNegotiatedCompressionAlgorithm(CompressionAlgorithm compressionAlgo) {
    negotiatedCompressionAlgo_ = compressionAlgo;
  }

 protected:
  std::unique_ptr<apache::thrift::AsyncProcessor> processor_;
  std::unique_ptr<DuplexChannel> duplexChannel_;
  std::shared_ptr<apache::thrift::HeaderServerChannel> channel_;

  std::shared_ptr<Cpp2Worker> worker_;
  Cpp2Worker* getWorker() {
    return worker_.get();
  }
  Cpp2ConnContext context_;

  std::shared_ptr<folly::AsyncTransport> transport_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  folly::Optional<CompressionAlgorithm> negotiatedCompressionAlgo_;

  /**
   * Wrap the request in our own request.  This is done for 2 reasons:
   * a) To have task timeouts for all requests,
   * b) To ensure the channel is not destroyed before callback is called
   */
  class Cpp2Request final : public ResponseChannelRequest {
   public:
    friend class Cpp2Connection;

    class QueueTimeout : public folly::HHWheelTimer::Callback {
      Cpp2Request* request_;
      void timeoutExpired() noexcept override;
      friend class Cpp2Request;
    };
    class TaskTimeout : public folly::HHWheelTimer::Callback {
      Cpp2Request* request_;
      void timeoutExpired() noexcept override;
      friend class Cpp2Request;
    };
    friend class QueueTimeout;
    friend class TaskTimeout;

    Cpp2Request(
        RequestsRegistry::DebugStub& debugStubToInit,
        std::unique_ptr<HeaderServerChannel::HeaderRequest> req,
        std::shared_ptr<folly::RequestContext> rctx,
        std::shared_ptr<Cpp2Connection> con,
        std::unique_ptr<folly::IOBuf> debugPayload);

    // Delegates to wrapped request.
    bool isActive() const override {
      return req_->isActive();
    }
    void cancel() override {
      req_->cancel();
    }

    bool isOneway() const override {
      return req_->isOneway();
    }

    bool isStream() const override {
      return req_->isStream();
    }

    void sendReply(
        std::unique_ptr<folly::IOBuf>&& buf,
        MessageChannel::SendCallback* notUsed = nullptr,
        folly::Optional<uint32_t> crc32c = folly::none) override;
    void sendErrorWrapped(folly::exception_wrapper ew, std::string exCode)
        override;
    void sendTimeoutResponse(
        apache::thrift::HeaderServerChannel::HeaderRequest::TimeoutResponseType
            responseType);

    ~Cpp2Request() override;

    // Cancel request is ususally called from a different thread than sendReply.
    virtual void cancelRequest();

    Cpp2RequestContext* getContext() {
      return &reqContext_;
    }

    server::TServerObserver::CallTimestamps& getTimestamps() {
      return static_cast<server::TServerObserver::CallTimestamps&>(
          reqContext_.getTimestamps());
    }

   private:
    MessageChannel::SendCallback* prepareSendCallback(
        MessageChannel::SendCallback* sendCallback,
        apache::thrift::server::TServerObserver* observer);

    std::unique_ptr<HeaderServerChannel::HeaderRequest> req_;

    // The order of these two fields matters; to save a shared_ptr operation, we
    // move into connection_ first and then use the pointer in connection_ to
    // initialize reqContext_; since field initialization happens in order of
    // definition, connection_ needs to appear before reqContext_.
    std::shared_ptr<Cpp2Connection> connection_;
    Cpp2RequestContext reqContext_;

    QueueTimeout queueTimeout_;
    TaskTimeout taskTimeout_;

    Cpp2Worker::ActiveRequestsGuard activeRequestsGuard_;

    void cancelTimeout() {
      queueTimeout_.cancelTimeout();
      taskTimeout_.cancelTimeout();
    }
    void markProcessEnd(
        std::map<std::string, std::string>* newHeaders = nullptr);
    void setLatencyHeaders(
        const apache::thrift::server::TServerObserver::CallTimestamps&,
        std::map<std::string, std::string>* newHeaders = nullptr) const;
    void setLatencyHeader(
        const std::string& key,
        const std::string& value,
        std::map<std::string, std::string>* newHeaders = nullptr) const;
  };

  class Cpp2Sample : public MessageChannel::SendCallback {
   public:
    Cpp2Sample(
        apache::thrift::server::TServerObserver::CallTimestamps& timestamps,
        apache::thrift::server::TServerObserver* observer,
        MessageChannel::SendCallback* chainedCallback = nullptr);

    void sendQueued() override;
    void messageSent() override;
    void messageSendError(folly::exception_wrapper&& e) override;
    ~Cpp2Sample() override;

   private:
    apache::thrift::server::TServerObserver::CallTimestamps timestamps_;
    apache::thrift::server::TServerObserver* observer_;
    MessageChannel::SendCallback* chainedCallback_;
  };

  std::unordered_set<Cpp2Request*> activeRequests_;

  void removeRequest(Cpp2Request* req);
  void handleAppError(
      std::unique_ptr<HeaderServerChannel::HeaderRequest> req,
      const std::string& name,
      const std::string& message,
      bool isClientError);
  void killRequest(
      std::unique_ptr<HeaderServerChannel::HeaderRequest> req,
      TApplicationException::TApplicationExceptionType reason,
      const std::string& errorCode,
      const char* comment);
  void disconnect(const char* comment) noexcept;

  void setServerHeaders(std::map<std::string, std::string>& writeHeaders);
  void setServerHeaders(HeaderServerChannel::HeaderRequest& request);

  friend class Cpp2Request;

  std::shared_ptr<Cpp2Connection> this_;
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_CPP2CONNECTION_H_
