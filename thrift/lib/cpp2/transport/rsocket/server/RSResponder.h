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
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ServerConfigs.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <yarpl/Observable.h>
#include <yarpl/Single.h>

namespace apache {
namespace thrift {

// One instance of RSResponder per client connection.
class RSResponder : public rsocket::RSocketResponderCore {
 public:
  RSResponder(
      std::shared_ptr<Cpp2Worker> worker,
      const folly::SocketAddress& clientAddress);

  virtual ~RSResponder() = default;

  void handleRequestResponse(
      rsocket::Payload request,
      rsocket::StreamId streamId,
      std::shared_ptr<yarpl::single::SingleObserver<rsocket::Payload>>
          response) noexcept override;

  void handleFireAndForget(rsocket::Payload request, rsocket::StreamId streamId)
      override;

  void handleRequestStream(
      rsocket::Payload request,
      rsocket::StreamId streamId,
      std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>>
          response) noexcept override;

 private:
  void onThriftRequest(
      std::unique_ptr<ThriftRequestCore> request,
      std::unique_ptr<folly::IOBuf> buf,
      bool invalidMetadata);

 private:
  std::unique_ptr<Cpp2ConnContext> createConnContext() const;

  std::shared_ptr<Cpp2Worker> worker_;
  std::shared_ptr<AsyncProcessor> cpp2Processor_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  std::shared_ptr<apache::thrift::server::TServerObserver> observer_;
  server::ServerConfigs* serverConfigs_;
  const folly::SocketAddress clientAddress_;
};
} // namespace thrift
} // namespace apache
