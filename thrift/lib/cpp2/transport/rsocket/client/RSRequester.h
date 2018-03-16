/*
 * Copyright 2017-present Facebook, Inc.
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

#include <rsocket/RSocket.h>
#include <rsocket/RSocketRequester.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>

namespace apache {
namespace thrift {

// Decorates RSocketRequester to enable EventBase switching.
class RSRequester : public rsocket::RSocketRequester {
 public:
  RSRequester(
      apache::thrift::async::TAsyncTransport::UniquePtr socket,
      folly::EventBase* evb,
      std::shared_ptr<rsocket::RSocketConnectionEvents> status);

  virtual ~RSRequester();

  virtual void closeNow();
  virtual void attachEventBase(folly::EventBase* evb);
  virtual void detachEventBase();
  virtual bool isDetachable();

  // If we observe any Stream call, then disable eventbase switching
  // We need to do better implementation to support eventbase switching for
  // Stream RPC calls.
  std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>> requestStream(
      rsocket::Payload request) override;

  std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>> requestChannel(
      rsocket::Payload request,
      std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>> requests)
      override;

 private:
  // Call closeNow instead of closeSocket
  using rsocket::RSocketRequester::closeSocket;

  std::shared_ptr<rsocket::RSocketConnectionEvents> connectionStatus_;

  bool isDetachable_{true};
};
} // namespace thrift
} // namespace apache
