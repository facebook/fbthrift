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

#include <folly/ExceptionWrapper.h>
#include <folly/io/async/HHWheelTimer.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace folly {
class EventBase;
} // namespace folly

namespace apache {
namespace thrift {

class RocketStreamClientCallback final : public StreamClientCallback {
 public:
  RocketStreamClientCallback(
      rocket::RocketServerFrameContext&& context,
      uint32_t initialRequestN);
  ~RocketStreamClientCallback() override = default;

  void onFirstResponse(
      FirstResponsePayload&& firstResponse,
      folly::EventBase* evb,
      StreamServerCallback* serverCallback) override;
  void onFirstResponseError(folly::exception_wrapper ew) override;

  void onStreamNext(StreamPayload&& payload) override;
  void onStreamError(folly::exception_wrapper ew) override;
  void onStreamComplete() override;

  void request(uint32_t n);

  StreamServerCallback& getStreamServerCallback();
  void timeoutExpired() noexcept;

 private:
  rocket::RocketServerFrameContext context_;
  StreamServerCallback* serverCallback_{nullptr};
  uint64_t tokens_{0};
  std::unique_ptr<folly::HHWheelTimer::Callback> timeoutCallback_;

  void scheduleTimeout();
  void cancelTimeout();

  template <class Payload>
  void compressResponse(Payload& payload);
};

} // namespace thrift
} // namespace apache
