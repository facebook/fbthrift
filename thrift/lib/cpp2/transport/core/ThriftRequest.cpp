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

#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>

namespace apache {
namespace thrift {

ThriftRequestCore::RequestTimestampSample::RequestTimestampSample(
    server::TServerObserver::CallTimestamps& timestamps,
    server::TServerObserver* observer,
    MessageChannel::SendCallback* chainedCallback)
    : timestamps_(timestamps),
      observer_(observer),
      chainedCallback_(chainedCallback) {
  DCHECK(observer != nullptr);
}

void ThriftRequestCore::RequestTimestampSample::sendQueued() {
  timestamps_.writeBegin = std::chrono::steady_clock::now();
  if (chainedCallback_ != nullptr) {
    chainedCallback_->sendQueued();
  }
}

void ThriftRequestCore::RequestTimestampSample::messageSent() {
  SCOPE_EXIT {
    delete this;
  };
  timestamps_.writeEnd = std::chrono::steady_clock::now();
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSent();
  }
}

void ThriftRequestCore::RequestTimestampSample::messageSendError(
    folly::exception_wrapper&& e) {
  SCOPE_EXIT {
    delete this;
  };
  timestamps_.writeEnd = std::chrono::steady_clock::now();
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSendError(std::move(e));
  }
}

ThriftRequestCore::RequestTimestampSample::~RequestTimestampSample() {
  if (observer_) {
    observer_->callCompleted(timestamps_);
  }
}

MessageChannel::SendCallbackPtr ThriftRequestCore::prepareSendCallback(
    MessageChannel::SendCallbackPtr&& cb,
    server::TServerObserver* observer) {
  auto cbPtr = std::move(cb);
  // If we are sampling this call, wrap it with a RequestTimestampSample,
  // which also implements MessageChannel::SendCallback. Callers of
  // sendReply/sendError are responsible for cleaning up their own callbacks.
  auto& timestamps = getTimestamps();
  if (timestamps.getSamplingStatus().isEnabledByServer()) {
    auto chainedCallback = cbPtr.release();
    return MessageChannel::SendCallbackPtr(
        new ThriftRequestCore::RequestTimestampSample(
            timestamps, observer, chainedCallback));
  }
  return cbPtr;
}

void ThriftRequestCore::sendReplyInternal(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> buf,
    MessageChannel::SendCallbackPtr cb) {
  if (checkResponseSize(*buf)) {
    cb = prepareSendCallback(std::move(cb), serverConfigs_.getObserver());
    sendThriftResponse(std::move(metadata), std::move(buf), std::move(cb));
  } else {
    sendResponseTooBigEx();
  }
}

void ThriftRequestCore::sendReply(
    std::unique_ptr<folly::IOBuf>&& buf,
    MessageChannel::SendCallback* cb,
    folly::Optional<uint32_t> crc32c) {
  auto cbWrapper = MessageChannel::SendCallbackPtr(cb);
  if (active_.exchange(false)) {
    cancelTimeout();
    // Mark processEnd for the request.
    // Note: this processEnd time unfortunately does not account for the time
    // to compress the response in rocket today (which happens in
    // ThriftServerRequestResponse::sendThriftResponse).
    // TODO: refactor to move response compression to CPU thread.
    ;
    auto& timestamps = getTimestamps();
    if (UNLIKELY(timestamps.getSamplingStatus().isEnabled())) {
      timestamps.processEnd = std::chrono::steady_clock::now();
    }
    if (!isOneway()) {
      auto metadata = makeResponseRpcMetadata(header_.extractAllWriteHeaders());
      if (crc32c) {
        metadata.crc32c_ref() = *crc32c;
      }
      sendReplyInternal(
          std::move(metadata), std::move(buf), std::move(cbWrapper));
      if (auto* observer = serverConfigs_.getObserver()) {
        observer->sentReply();
      }
    }
  }
}

} // namespace thrift
} // namespace apache
