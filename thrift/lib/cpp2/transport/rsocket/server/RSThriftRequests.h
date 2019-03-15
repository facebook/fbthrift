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

#include <rsocket/Payload.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <yarpl/flowable/Subscriber.h>
#include <yarpl/single/SingleObserver.h>

namespace apache {
namespace thrift {

namespace detail {
std::unique_ptr<folly::IOBuf> serializeMetadata(
    const ResponseRpcMetadata& responseMetadata);

// Deserializes metadata returning an invalid object with the kind field unset
// on error.
std::unique_ptr<RequestRpcMetadata> deserializeMetadata(
    const folly::IOBuf& buffer);
} // namespace detail

class RSOneWayRequest final : public ThriftRequestCore {
 public:
  RSOneWayRequest(
      const apache::thrift::server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      folly::EventBase* evb,
      folly::Function<void(RSOneWayRequest*)> onDestroy);

  virtual ~RSOneWayRequest();

  void cancel() override;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::SemiStream<
          std::unique_ptr<folly::IOBuf>>) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

 private:
  folly::EventBase* evb_;
  folly::Function<void(RSOneWayRequest*)> onDestroy_;
};

class RSSingleRequest final : public ThriftRequestCore {
 public:
  RSSingleRequest(
      const apache::thrift::server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      folly::EventBase* evb,
      std::shared_ptr<yarpl::single::SingleObserver<rsocket::Payload>>
          singleObserver);

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::SemiStream<
          std::unique_ptr<folly::IOBuf>>) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

 private:
  folly::EventBase* evb_;
  std::shared_ptr<yarpl::single::SingleObserver<rsocket::Payload>>
      singleObserver_;
};

class RSStreamRequest final : public ThriftRequestCore {
 public:
  RSStreamRequest(
      const apache::thrift::server::ServerConfigs& serverConfigs,
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::shared_ptr<Cpp2ConnContext> connContext,
      folly::EventBase* evb,
      std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>>
          subscriber);

  bool isStream() const override {
    return true;
  }

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> response) noexcept override;

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> response,
      apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>
          stream) noexcept override;

  folly::EventBase* getEventBase() noexcept override;

 protected:
  folly::EventBase* evb_;
  std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>> subscriber_;
};

} // namespace thrift
} // namespace apache
