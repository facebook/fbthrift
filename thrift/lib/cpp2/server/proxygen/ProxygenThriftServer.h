/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/lib/cpp2/server/BaseThriftServer.h>

#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/Request.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/session/HTTPSession.h>
#include <wangle/concurrent/IOThreadPoolExecutor.h>

namespace apache {
namespace thrift {

/**
 *   This is yet another thrift server, using the Proxygen networking stack
 *   Uses cpp2 style generated code.
 */

class ProxygenThriftServer : public BaseThriftServer,
                             public proxygen::HTTPSession::InfoCallback {
 protected:
  class ThriftRequestHandler;

  class ThriftRequestHandlerFactory : public proxygen::RequestHandlerFactory {
    friend class ThriftRequestHandler;

   public:
    explicit ThriftRequestHandlerFactory(ProxygenThriftServer* server)
        : server_(server),
          processor_(server_->getCpp2Processor()),
          evb_(nullptr),
          timer_(nullptr) {}

    void onServerStart(folly::EventBase* evb) noexcept override {
      evb_ = evb;
      timer_ = folly::HHWheelTimer::newTimer(evb);
      server_->threadManager_->start();
    }

    void onServerStop() noexcept override {
      evb_ = nullptr;
      timer_.reset();
      server_->threadManager_->stop();
    }

    proxygen::RequestHandler* onRequest(
        proxygen::RequestHandler*, proxygen::HTTPMessage*) noexcept override {
      return new ThriftRequestHandler(
          this, timer_.get(), server_->threadManager_.get());
    }

    apache::thrift::AsyncProcessor* getProcessor() { return processor_.get(); }

   private:
    ProxygenThriftServer* server_;
    std::unique_ptr<apache::thrift::AsyncProcessor> processor_;
    folly::EventBase* evb_;
    folly::HHWheelTimer::UniquePtr timer_;
  };

  class ThriftRequestHandler : public proxygen::RequestHandler,
                               public apache::thrift::ResponseChannel::Request {
   public:
    explicit ThriftRequestHandler(
        ThriftRequestHandlerFactory* worker,
        folly::HHWheelTimer* timer,
        apache::thrift::concurrency::ThreadManager* threadManager)
        : worker_(worker),
          timer_(timer),
          threadManager_(threadManager),
          header_(),
          active_(true),
          cb_(nullptr),
          softTimeout_(this, false),
          hardTimeout_(this, true),
          request_(nullptr) {}

    virtual ~ThriftRequestHandler() {
      softTimeout_.cancelTimeout();
      hardTimeout_.cancelTimeout();
    }

    void onRequest(
        std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

    void onEOM() noexcept override;

    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;

    void requestComplete() noexcept override;

    void onError(proxygen::ProxygenError err) noexcept override;

    // apache::thrift::ResponseChannel::Request
    bool isActive() override { return active_; }
    void cancel() override { active_ = false; }
    bool isOneway() override { return false; }

    void sendReply(
        std::unique_ptr<folly::IOBuf>&&, // && from ResponseChannel.h
        apache::thrift::MessageChannel::SendCallback* cb = nullptr) override;

    void sendErrorWrapped(
        folly::exception_wrapper ex,
        std::string exCode,
        apache::thrift::MessageChannel::SendCallback* cb = nullptr) override;

    class ProxygenRequest : public apache::thrift::ResponseChannel::Request {
     public:
      explicit ProxygenRequest(ThriftRequestHandler* handler)
          : handler_(handler) {}

      bool isActive() override {
        if (handler_) {
          return handler_->isActive();
        }

        return false;
      }

      void cancel() override {
        if (handler_) {
          handler_->cancel();
        }
      }

      bool isOneway() override {
        if (handler_) {
          return handler_->isOneway();
        }

        return false;
      }

      void sendReply(
          std::unique_ptr<folly::IOBuf>&& buf, // && from ResponseChannel.h
          apache::thrift::MessageChannel::SendCallback* cb = nullptr) override {
        if (handler_) {
          handler_->sendReply(std::move(buf), cb);
        }
      }

      void sendErrorWrapped(
          folly::exception_wrapper ex,
          std::string exCode,
          apache::thrift::MessageChannel::SendCallback* cb = nullptr) override {
        if (handler_) {
          handler_->sendErrorWrapped(ex, exCode, cb);
        }
      }

      void clearHandler() { handler_ = nullptr; }

     private:
      ThriftRequestHandler* handler_;
    };

    class TaskTimeout : public folly::HHWheelTimer::Callback {
     public:
      explicit TaskTimeout(ThriftRequestHandler* request, bool hard)
          : request_(request), hard_(hard) {}

      void timeoutExpired() noexcept override;

     private:
      ThriftRequestHandler* request_;
      bool hard_;
    };

   private:
    ThriftRequestHandlerFactory* worker_;
    folly::HHWheelTimer* timer_;
    apache::thrift::concurrency::ThreadManager* threadManager_;

    std::unique_ptr<proxygen::HTTPMessage> msg_;
    apache::thrift::transport::THeader header_;

    std::unique_ptr<folly::IOBuf> body_;

    std::unique_ptr<apache::thrift::Cpp2ConnContext> connCtx_;
    std::unique_ptr<apache::thrift::Cpp2RequestContext> reqCtx_;

    std::atomic<bool> active_;
    apache::thrift::MessageChannel::SendCallback* cb_;

    TaskTimeout softTimeout_;
    TaskTimeout hardTimeout_;

    ProxygenRequest* request_;
  };

  class ConnectionContext : public apache::thrift::server::TConnectionContext {
   public:
    explicit ConnectionContext(const proxygen::HTTPSession& session)
        : apache::thrift::server::TConnectionContext() {
      peerAddress_ = session.getPeerAddress();
    }
    ~ConnectionContext() {}
  };

  bool isOverloaded(
      uint32_t workerActiveRequests = 0,
      const apache::thrift::transport::THeader* header = nullptr) override;

  // Get load percent of the server.  Must be a number between 0 and 100:
  // 0 - no load, 100-fully loaded.
  virtual int64_t getRequestLoad() override;
  virtual int64_t getConnectionLoad() override;

  /**
   * Get the number of connections dropped by the AsyncServerSocket
   */
  virtual uint64_t getNumDroppedConnections() const override;

  // proxygen::HTTPSession::InfoCallback methods
  void onCreate(const proxygen::HTTPSession& session) override {
    auto ctx = folly::make_unique<ConnectionContext>(session);
    if (eventHandler_) {
      eventHandler_->newConnection(ctx.get());
    }
    connectionMap_[&session] = std::move(ctx);
  }
  void onIngressError(const proxygen::HTTPSession&,
                      proxygen::ProxygenError) override {}
  void onRead(const proxygen::HTTPSession&, size_t) override {}
  void onWrite(const proxygen::HTTPSession&, size_t) override {}
  void onRequestBegin(const proxygen::HTTPSession&) override {}
  void onRequestEnd(const proxygen::HTTPSession&,
                    uint32_t) override {}
  void onActivateConnection(const proxygen::HTTPSession&) override {}
  void onDeactivateConnection(const proxygen::HTTPSession&,
                              const proxygen::TransactionInfo&) override {}
  void onDestroy(const proxygen::HTTPSession& session) override {
    auto itr = connectionMap_.find(&session);
    DCHECK(itr != connectionMap_.end());
    if (eventHandler_) {
      eventHandler_->connectionDestroyed(itr->second.get());
    }
    connectionMap_.erase(itr);
  }
  void onIngressMessage(const proxygen::HTTPSession&,
                        const proxygen::HTTPMessage&) override {}
  void onIngressLimitExceeded(const proxygen::HTTPSession&) override {}
  void onIngressPaused(const proxygen::HTTPSession&) override {}
  void onTransactionDetached(const proxygen::HTTPSession&,
                             const proxygen::TransactionInfo&) override {}
  void onPingReplySent(int64_t) override {}
  void onPingReplyReceived() override {}
  void onSettingsOutgoingStreamsFull(const proxygen::HTTPSession&) override {}
  void onSettingsOutgoingStreamsNotFull(const proxygen::HTTPSession&) override {
  }
  void onFlowControlWindowClosed(const proxygen::HTTPSession&) override {}
  void onEgressBuffered(const proxygen::HTTPSession&) override {}

  std::unique_ptr<proxygen::HTTPServer> server_;

  size_t initialReceiveWindow_{65536};

  std::unordered_map<const proxygen::HTTPSession*,
                     std::unique_ptr<ConnectionContext>> connectionMap_;

 public:
  void setInitialReceiveWindow(size_t window) {
    initialReceiveWindow_ = window;
  }

  size_t getInitialReceiveWindow() { return initialReceiveWindow_; }

  virtual void serve() override;

  virtual void stop() override;

  // This API is intended to stop listening on the server
  // socket and stop accepting new connection first while
  // still letting the established connections to be
  // processed on the server.
  virtual void stopListening() override;
};
}
} // apache::thrift
