/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef THRIFT_ASYNC_CPP2CONNECTION_H_
#define THRIFT_ASYNC_CPP2CONNECTION_H_ 1

#include <thrift/lib/cpp/async/HHWheelTimer.h>
#include <thrift/lib/cpp/async/TEventConnection.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>
#include <memory>
#include <unordered_set>

#include <wangle/acceptor/ManagedConnection.h>

namespace apache { namespace thrift {
/**
 * Represents a connection that is handled via libevent. This connection
 * essentially encapsulates a socket that has some associated libevent state.
 */
class Cpp2Connection
    : public ResponseChannel::Callback
    , public wangle::ManagedConnection {
 public:

  static const std::string loadHeader;
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
    const std::shared_ptr<apache::thrift::async::TAsyncSocket>& asyncSocket,
      const folly::SocketAddress* address,
      Cpp2Worker* worker,
      const std::shared_ptr<HeaderServerChannel>& serverChannel = nullptr);

  /// Destructor -- close down the connection.
  ~Cpp2Connection() override;

  // ResponseChannel callbacks
  void requestReceived(std::unique_ptr<ResponseChannel::Request>&&) override;
  void channelClosed(folly::exception_wrapper&&) override;

  void start() {
    channel_->setCallback(this);
  }

  void stop();

  void timeoutExpired() noexcept override;

  void requestTimeoutExpired();

  bool pending();

  // Managed Connection callbacks
  void describe(std::ostream& os) const override{}
  bool isBusy() const override {
    return activeRequests_.empty();
  }
  void notifyPendingShutdown() override {}
  void closeWhenIdle() override {
    stop();
  }
  void dropConnection() override {
    stop();
  }
  void dumpConnectionState(uint8_t loglevel) override {}
  void addConnection(std::shared_ptr<Cpp2Connection> conn) {
    this_ = conn;
  }

 protected:
  std::unique_ptr<apache::thrift::AsyncProcessor> processor_;
  std::unique_ptr<DuplexChannel> duplexChannel_;
  std::shared_ptr<apache::thrift::HeaderServerChannel> channel_;

  Cpp2Worker* worker_;
  Cpp2Worker* getWorker() {
    return worker_;
  }
  Cpp2ConnContext context_;

  std::shared_ptr<apache::thrift::async::TAsyncSocket> socket_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;

  /**
   * Wrap the request in our own request.  This is done for 2 reasons:
   * a) To have task timeouts for all requests,
   * b) To ensure the channel is not destroyed before callback is called
   */
  class Cpp2Request
      : public ResponseChannel::Request {
   public:
    friend class Cpp2Connection;

    class SoftTimeout
      : public apache::thrift::async::HHWheelTimer::Callback {
      Cpp2Request* request_;
      void timeoutExpired() noexcept override;
      friend class Cpp2Request;
    };
    class HardTimeout
      : public apache::thrift::async::HHWheelTimer::Callback {
      Cpp2Request* request_;
      void timeoutExpired() noexcept override;
      friend class Cpp2Request;
    };
    friend class SoftTimeout;
    friend class HardTimeout;

    Cpp2Request(std::unique_ptr<ResponseChannel::Request> req,
                   std::shared_ptr<Cpp2Connection> con);

    // Delegates to wrapped request.
    bool isActive() override { return req_->isActive(); }
    void cancel() override { req_->cancel(); }

    bool isOneway() override { return req_->isOneway(); }

    void sendReply(std::unique_ptr<folly::IOBuf>&& buf,
                   MessageChannel::SendCallback* notUsed = nullptr) override;
    void sendErrorWrapped(
        folly::exception_wrapper ew,
        std::string exCode,
        MessageChannel::SendCallback* notUsed = nullptr) override;

    ~Cpp2Request() override;

    // Cancel request is ususally called from a different thread than sendReply.
    virtual void cancelRequest();

    Cpp2RequestContext* getContext() {
      return &reqContext_;
    }

    apache::thrift::server::TServerObserver::CallTimestamps& getTimestamps()
        override {
      return req_->getTimestamps();
    }

   private:
    MessageChannel::SendCallback* prepareSendCallback(
        MessageChannel::SendCallback* sendCallback,
        apache::thrift::server::TServerObserver* observer);

    std::unique_ptr<HeaderServerChannel::HeaderRequest> req_;
    std::shared_ptr<Cpp2Connection> connection_;
    Cpp2RequestContext reqContext_;
    SoftTimeout softTimeout_;
    HardTimeout hardTimeout_;

    void cancelTimeout() {
      softTimeout_.cancelTimeout();
      hardTimeout_.cancelTimeout();
    }
  };

  class Cpp2Sample
      : public MessageChannel::SendCallback {
   public:
    Cpp2Sample(
      apache::thrift::server::TServerObserver::CallTimestamps&& timestamps,
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
  void killRequest(ResponseChannel::Request& req,
                   TApplicationException::TApplicationExceptionType reason,
                   const char* comment);
  void disconnect(const char* comment) noexcept;

  // Set any error headers necessary, based on the received headers
  void setErrorHeaders(apache::thrift::transport::THeader* recv_headers);

  friend class Cpp2Request;

  std::shared_ptr<Cpp2Connection> this_;
};

}} // apache::thrift

#endif // #ifndef THRIFT_ASYNC_CPP2CONNECTION_H_
