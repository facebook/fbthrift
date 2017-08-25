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

#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>

#include <folly/Executor.h>
#include <folly/io/async/EventBase.h>
#include <glog/logging.h>

namespace apache {
namespace thrift {

/**
 * A simple channel that collects the response and makes it available
 * to test code.
 */
class FakeChannel : public ThriftChannelIf {
 public:
  explicit FakeChannel(folly::EventBase* evb)
      : evb_(evb), keepAliveToken_(evb_->getKeepAliveToken()) {}
  ~FakeChannel() override = default;

  void sendThriftResponse(
      uint32_t /*seqId*/,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload) noexcept override {
    headers_ = std::move(headers);
    payload_ = std::move(payload);
    // Tests that use this class are expected to be done at this point.
    // So we shut down the event base.
    keepAliveToken_.reset();
  }

  void cancel(uint32_t /*seqId*/) noexcept override {
    LOG(ERROR) << "cancel() unused in this fake object.";
  }

  void sendThriftRequest(
      std::unique_ptr<FunctionInfo> /*functionInfo*/,
      std::unique_ptr<std::map<std::string, std::string>> /*headers*/,
      std::unique_ptr<folly::IOBuf> /*payload*/,
      std::unique_ptr<ThriftClientCallback> /*callback*/) noexcept override {
    LOG(FATAL) << "sendThriftRequest() unused in this fake object.";
  }

  void cancel(ThriftClientCallback* /*callback*/) noexcept override {
    LOG(ERROR) << "cancel() unused in this fake object.";
  }

  folly::EventBase* getEventBase() noexcept override {
    return evb_;
  }

/*
  void setInput(uint32_t, SubscriberRef) noexcept override {
    LOG(FATAL) << "setInput() unused in this fake object.";
  }

  SubscriberRef getOutput(uint32_t) noexcept override {
    LOG(FATAL) << "getOutput() unused in this fake object.";
  }
*/

  std::map<std::string, std::string>* getHeaders() {
    return headers_.get();
  }

  folly::IOBuf* getPayloadBuf() {
    return payload_.get();
  }

 private:
  std::unique_ptr<std::map<std::string, std::string>> headers_;
  std::unique_ptr<folly::IOBuf> payload_;
  folly::EventBase* evb_;
  folly::Executor::KeepAlive keepAliveToken_;
};

} // namespace thrift
} // namespace apache
