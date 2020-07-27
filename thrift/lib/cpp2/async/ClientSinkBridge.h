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

#include <boost/variant.hpp>

#include <folly/Portability.h>

#include <folly/Overload.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>
#endif

#include <thrift/lib/cpp2/async/SinkBridgeUtil.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>

namespace apache {
namespace thrift {
namespace detail {

#if FOLLY_HAS_COROUTINES
class ClientSinkBridge;

// Instantiated in ClientSinkBridge.cpp
extern template class TwoWayBridge<
    CoroConsumer,
    ClientMessage,
    ClientSinkBridge,
    ServerMessage,
    ClientSinkBridge>;

class ClientSinkBridge : public TwoWayBridge<
                             CoroConsumer,
                             ClientMessage,
                             ClientSinkBridge,
                             ServerMessage,
                             ClientSinkBridge>,
                         public SinkClientCallback {
 public:
  ~ClientSinkBridge() override;

  folly::coro::Task<folly::Try<FirstResponsePayload>> getFirstThriftResponse();

  static Ptr create();

  void close();

  folly::coro::Task<folly::Try<StreamPayload>> sink(
      folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> generator);

  void cancel(std::unique_ptr<folly::IOBuf> ex);

  // SinkClientCallback method
  bool onFirstResponse(
      FirstResponsePayload&& firstPayload,
      folly::EventBase* evb,
      SinkServerCallback* serverCallback) override;

  void onFirstResponseError(folly::exception_wrapper ew) override;

  void onFinalResponse(StreamPayload&& payload) override;

  void onFinalResponseError(folly::exception_wrapper ew) override;

  bool onSinkRequestN(uint64_t n) override;

  void resetServerCallback(SinkServerCallback& serverCallback) override;

  void consume();

  void canceled() {}

 private:
  ClientSinkBridge();

  void processServerMessages();

  folly::coro::Baton firstResponseBaton_{};
  folly::Try<FirstResponsePayload> firstResponse_;

  SinkServerCallback* serverCallback_{nullptr};
  folly::Executor::KeepAlive<folly::EventBase> evb_;
  folly::CancellationSource cancelSource_;
};
#else
class ClientSinkBridge {
 public:
  using Ptr = std::unique_ptr<ClientSinkBridge>;
};
#endif

} // namespace detail
} // namespace thrift
} // namespace apache
