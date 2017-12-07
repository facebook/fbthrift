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

#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/transport/core/StreamRequestCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSRequester.h>

namespace apache {
namespace thrift {

namespace detail {
class ChannelCounters {
 public:
  ChannelCounters();
  void setMaxPendingRequests(uint32_t);
  uint32_t getMaxPendingRequests();
  uint32_t getPendingRequests();
  bool incPendingRequests();
  void decPendingRequests();

 private:
  uint32_t maxPendingRequests_;
  uint32_t pendingRequests_{0u};
};
} // namespace detail

class RSClientThriftChannel : public ThriftChannelIf {
 public:
  using FlowableRef = yarpl::Reference<
      yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>>;

  explicit RSClientThriftChannel(
      std::shared_ptr<RSRequester> rsRequester,
      apache::thrift::detail::ChannelCounters& counters,
      folly::EventBase* evb);

  virtual ~RSClientThriftChannel() = default;

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

  void sendStreamThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<StreamRequestCallback> callback) noexcept;

  static std::unique_ptr<folly::IOBuf> serializeMetadata(
      const RequestRpcMetadata& requestMetadata);

  static std::unique_ptr<ResponseRpcMetadata> deserializeMetadata(
      const folly::IOBuf& buffer);

  bool isDetachable();
  bool attachEventBase(folly::EventBase* evb);
  void detachEventBase();

 protected:
  void sendSingleRequestNoResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendSingleRequestResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept;

  void sendStreamRequestStreamResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<StreamRequestCallback> callback) noexcept;

  void sendSingleRequestStreamResponse(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<StreamRequestCallback> callback) noexcept;

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override {
    LOG(FATAL) << "Server side function is called in client side.";
  }

  folly::EventBase* getEventBase() noexcept override {
    LOG(FATAL) << "not implemented";
  }

 protected:
  std::shared_ptr<RSRequester> rsRequester_;
  apache::thrift::detail::ChannelCounters& channelCounters_;
  folly::EventBase* evb_;
};
} // namespace thrift
} // namespace apache
