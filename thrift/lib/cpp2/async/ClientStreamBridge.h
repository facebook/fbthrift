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

class ClientStreamBridge;

// This template explicitly instantiated in ClientStreamBridge.cpp
extern template class TwoWayBridge<
    ClientStreamConsumer,
    folly::Try<StreamPayload>,
    ClientStreamBridge,
    int64_t,
    ClientStreamBridge>;

class ClientStreamBridge : public TwoWayBridge<
                               ClientStreamConsumer,
                               folly::Try<StreamPayload>,
                               ClientStreamBridge,
                               int64_t,
                               ClientStreamBridge>,
                           private StreamClientCallback {
 public:
  ~ClientStreamBridge() override;

  struct ClientDeleter : Deleter {
    void operator()(ClientStreamBridge* ptr);
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

  static StreamClientCallback* create(FirstResponseCallback* callback);

  bool wait(ClientStreamConsumer* consumer);

  ClientQueue getMessages();

  void requestN(int64_t credits);

  void cancel();

  bool isCanceled();

  void consume();

  void canceled();

 private:
  explicit ClientStreamBridge(FirstResponseCallback* callback);

  bool onFirstResponse(
      FirstResponsePayload&& payload,
      folly::EventBase* evb,
      StreamServerCallback* streamServerCallback) override;

  void onFirstResponseError(folly::exception_wrapper ew) override;

  bool onStreamNext(StreamPayload&& payload) override;

  void onStreamError(folly::exception_wrapper ew) override;

  void onStreamComplete() override;

  void resetServerCallback(StreamServerCallback& serverCallback) override;

  void processCredits();

  void serverCleanup();

  union {
    FirstResponseCallback* firstResponseCallback_;
    StreamServerCallback* streamServerCallback_;
  };
  folly::Executor::KeepAlive<> serverExecutor_;
};
} // namespace detail
} // namespace thrift
} // namespace apache
