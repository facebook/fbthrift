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

#include <folly/Try.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>

namespace apache {
namespace thrift {
namespace detail {

class ClientStreamConsumer {
 public:
  virtual ~ClientStreamConsumer() = default;
  virtual void consume() = 0;
  virtual void canceled() = 0;
};

class ClientStreamBridge : public TwoWayBridge<
                               ClientStreamConsumer,
                               folly::Try<StreamPayload>,
                               ClientStreamBridge,
                               int64_t,
                               ClientStreamBridge>,
                           private StreamClientCallback {
 public:
  struct ClientDeleter : Deleter {
    void operator()(ClientStreamBridge* ptr) {
      ptr->cancel();
      Deleter::operator()(ptr);
    }
  };
  using ClientPtr = std::unique_ptr<ClientStreamBridge, ClientDeleter>;

  class FirstResponseCallback {
   public:
    virtual ~FirstResponseCallback() = default;
    virtual void onFirstResponse(
        FirstResponsePayload&&,
        ClientPtr clientStreamBridge) = 0;
    virtual void onFirstResponseError(folly::exception_wrapper) = 0;
  };

  static StreamClientCallback* create(FirstResponseCallback* callback) {
    return new ClientStreamBridge(callback);
  }

  bool wait(ClientStreamConsumer* consumer) {
    return clientWait(consumer);
  }

  ClientQueue getMessages() {
    return clientGetMessages();
  }

  void requestN(int64_t credits) {
    clientPush(std::move(credits));
  }

  void cancel() {
    clientPush(-1);
    clientClose();
  }
  bool isCanceled() {
    return isClientClosed();
  }

  void consume() {
    DCHECK(serverExecutor_);
    serverExecutor_->add([this]() { processCredits(); });
  }

  void canceled() {
    serverCleanup();
  }

 private:
  explicit ClientStreamBridge(FirstResponseCallback* callback)
      : firstResponseCallback_(callback) {}

  bool onFirstResponse(
      FirstResponsePayload&& payload,
      folly::EventBase* evb,
      StreamServerCallback* streamServerCallback) override {
    auto firstResponseCallback = firstResponseCallback_;
    serverExecutor_ = evb;
    streamServerCallback_ = streamServerCallback;
    auto scheduledWait = serverWait(this);
    DCHECK(scheduledWait);
    firstResponseCallback->onFirstResponse(
        std::move(payload), ClientPtr(copy().release()));
    return true;
  }

  void onFirstResponseError(folly::exception_wrapper ew) override {
    firstResponseCallback_->onFirstResponseError(std::move(ew));
    serverCleanup();
  }

  bool onStreamNext(StreamPayload&& payload) override {
    serverPush(folly::Try<StreamPayload>(std::move(payload)));
    return true;
  }

  void onStreamError(folly::exception_wrapper ew) override {
    serverPush(folly::Try<StreamPayload>(std::move(ew)));
    serverClose();
  }

  void onStreamComplete() override {
    serverPush(folly::Try<StreamPayload>());
    serverClose();
  }

  void resetServerCallback(StreamServerCallback& serverCallback) override {
    streamServerCallback_ = &serverCallback;
  }

  void processCredits() {
    if (isServerClosed()) {
      serverCleanup();
      return;
    }

    // serverClose() can't be called until this loop finishes
    int64_t credits = 0;
    while (!serverWait(this)) {
      for (auto messages = serverGetMessages(); !messages.empty();
           messages.pop()) {
        if (messages.front() == -1) {
          streamServerCallback_->onStreamCancel();
          serverCleanup();
          return;
        }
        credits += messages.front();
      }
    }

    std::ignore = streamServerCallback_->onStreamRequestN(credits);
  }

  void serverCleanup() {
    streamServerCallback_ = nullptr;
    serverExecutor_.reset();
    Ptr(this);
  }

  union {
    FirstResponseCallback* firstResponseCallback_;
    StreamServerCallback* streamServerCallback_;
  };
  folly::Executor::KeepAlive<> serverExecutor_;
};
} // namespace detail
} // namespace thrift
} // namespace apache
