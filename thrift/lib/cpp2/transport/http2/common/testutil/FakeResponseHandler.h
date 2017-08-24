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

#include <proxygen/httpserver/ResponseHandler.h>

#include <folly/Executor.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/Mocks.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace apache {
namespace thrift {

/**
 * A simple response handler that collects the response from the channel
 * and makes it available to the test code.
 */
class FakeResponseHandler : public proxygen::ResponseHandler {
 public:
  explicit FakeResponseHandler(folly::EventBase* evb)
      : proxygen::ResponseHandler(&dummyRequestHandler_),
        evb_(evb),
        keepAliveToken_(evb_->getKeepAliveToken()) {}

  ~FakeResponseHandler() override = default;

  // proxygen::ResponseHandler methods

  void sendHeaders(proxygen::HTTPMessage& msg) noexcept override;

  void sendChunkHeader(size_t /*len*/) noexcept override {
    // unimplemented
    CHECK(false);
  }

  void sendBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

  void sendChunkTerminator() noexcept override {
    // unimplemented
    CHECK(false);
  }

  void sendEOM() noexcept override;

  void sendAbort() noexcept override {
    // unimplemented
    CHECK(false);
  }

  void refreshTimeout() noexcept override {
    // unimplemented
    CHECK(false);
  }

  void pauseIngress() noexcept override {
    // unimplemented
    CHECK(false);
  }

  void resumeIngress() noexcept override {
    // unimplemented
    CHECK(false);
  }

  proxygen::ResponseHandler* newPushedResponse(proxygen::PushHandler* /*ph*/)
      noexcept override {
    // unimplemented
    CHECK(false);
  }

  const wangle::TransportInfo& getSetupTransportInfo() const noexcept override {
    // unimplemented
    CHECK(false);
  }

  void getCurrentTransportInfo(wangle::TransportInfo* /*ti*/) const override {
    // unimplemented
    CHECK(false);
  }

  // end of proxygen::ResponseHandler methods

  std::unordered_map<std::string, std::string>* getHeaders();

  folly::IOBuf* getBodyBuf();

  std::string getBody();

  bool eomReceived();

 private:
  proxygen::MockRequestHandler dummyRequestHandler_;
  wangle::TransportInfo dummyTransportInfo_;

  folly::EventBase* evb_;
  folly::Executor::KeepAlive keepAliveToken_;

  std::unordered_map<std::string, std::string> headers_;
  std::unique_ptr<folly::IOBuf> body_;
  bool eomReceived_{false};
};

} // namespace thrift
} // namespace apache
