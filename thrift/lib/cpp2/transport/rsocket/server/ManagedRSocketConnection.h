/*
 * Copyright 2018-present Facebook, Inc.
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

#include <wangle/acceptor/ManagedConnection.h>

#include <folly/io/async/AsyncTransport.h>
#include <rsocket/DuplexConnection.h>
#include <rsocket/statemachine/RSocketStateMachine.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

namespace apache {
namespace thrift {

class ManagedRSocketConnection
    : public wangle::ManagedConnection,
      private rsocket::RSocketStateMachine::CloseCallback {
 public:
  using OnNewSetupFn = folly::Function<std::shared_ptr<RSResponder>(
      const rsocket::SetupParameters&)>;

  ManagedRSocketConnection(
      folly::AsyncTransportWrapper::UniquePtr sock,
      OnNewSetupFn setupFunc);

  void timeoutExpired() noexcept override;
  bool isBusy() const override;
  void closeWhenIdle() override;
  void dropConnection() override;

  void describe(std::ostream&) const override {}
  void notifyPendingShutdown() override {}
  void dumpConnectionState(uint8_t) override {}

 protected:
  ~ManagedRSocketConnection() = default;

  void onSetup(
      std::unique_ptr<rsocket::DuplexConnection> connection,
      std::shared_ptr<RSResponder> responder,
      rsocket::SetupParameters setupParams);

  void removeConnection();

  // StateMachine wants to end the client connection
  void remove(rsocket::RSocketStateMachine&) override {
    removeConnection();
  }

  void stop(folly::exception_wrapper ew);

 private:
  std::shared_ptr<rsocket::DuplexConnection::Subscriber> setupSubscriber_;
  std::shared_ptr<rsocket::RSocketStateMachine> stateMachine_;

  class SetupSubscriber;
};

} // namespace thrift
} // namespace apache
