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

#include <thrift/lib/cpp2/async/Interaction.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/Sink.h>

namespace apache {
namespace thrift {

class AppOverloadExceptionInfo {
 public:
  AppOverloadExceptionInfo(
      ResponseChannelRequest::UniquePtr req, std::string message)
      : req_(std::move(req)), message_(std::move(message)) {}

  static void send(
      ResponseChannelRequest::UniquePtr req,
      const std::string& message) noexcept {
    req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::LOADSHEDDING, message),
        kAppOverloadedErrorCode);
  }
  void operator()(folly::EventBase&) noexcept {
    send(std::move(req_), message_);
  }

  ResponseChannelRequest::UniquePtr req_;
  std::string message_;
};

class QueueReplyInfo {
 public:
  QueueReplyInfo(
      ResponseChannelRequest::UniquePtr req,
      LegacySerializedResponse response,
      folly::Optional<uint32_t> crc32c)
      : req_(std::move(req)), response_(std::move(response)), crc32c_(crc32c) {}

  void operator()(folly::EventBase&) noexcept {
    req_->sendReply(std::move(response_.buffer), nullptr, crc32c_);
  }

  ResponseChannelRequest::UniquePtr req_;
  LegacySerializedResponse response_;
  folly::Optional<uint32_t> crc32c_;
};

class StreamReplyInfo {
 public:
  StreamReplyInfo(
      ResponseChannelRequest::UniquePtr req,
      apache::thrift::detail::ServerStreamFactory stream,
      LegacySerializedResponse response,
      folly::Optional<uint32_t> crc32c)
      : req_(std::move(req)),
        stream_(std::move(stream)),
        response_(std::move(response)),
        crc32c_(crc32c) {}

  void operator()(folly::EventBase&) noexcept {
    req_->sendStreamReply(
        std::move(response_.buffer), std::move(stream_), crc32c_);
  }

  ResponseChannelRequest::UniquePtr req_;
  apache::thrift::detail::ServerStreamFactory stream_;
  LegacySerializedResponse response_;
  folly::Optional<uint32_t> crc32c_;
};

class SinkConsumerReplyInfo {
 public:
  SinkConsumerReplyInfo(
      ResponseChannelRequest::UniquePtr req,
      apache::thrift::detail::SinkConsumerImpl sinkConsumer,
      LegacySerializedResponse response,
      folly::Optional<uint32_t> crc32c)
      : req_(std::move(req)),
        sinkConsumer_(std::move(sinkConsumer)),
        response_(std::move(response)),
        crc32c_(crc32c) {}

  void operator()(folly::EventBase&) noexcept {
#if FOLLY_HAS_COROUTINES
    req_->sendSinkReply(
        std::move(response_.buffer), std::move(sinkConsumer_), crc32c_);
#endif
  }

  ResponseChannelRequest::UniquePtr req_;
  apache::thrift::detail::SinkConsumerImpl sinkConsumer_;
  LegacySerializedResponse response_;
  folly::Optional<uint32_t> crc32c_;
};

using ReplyInfo = std::variant<
    AppOverloadExceptionInfo,
    QueueReplyInfo,
    StreamReplyInfo,
    SinkConsumerReplyInfo>;

/**
 * Used in EventBaseAtomicNotificationQueue to process each dequeued item
 */
class ReplyInfoConsumer {
 public:
  explicit ReplyInfoConsumer(folly::EventBase& evb) : evb_(evb) {}
  void operator()(ReplyInfo&& info) noexcept {
    std::visit([&evb = evb_](auto&& visitInfo) { visitInfo(evb); }, info);
  }

 private:
  folly::EventBase& evb_;
};

} // namespace thrift
} // namespace apache
