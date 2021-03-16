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

#include <atomic>

#include <folly/io/async/EventBase.h>

namespace apache {
namespace thrift {

class RequestStateMachine {
 public:
  bool isActive() const {
    return active_.load(std::memory_order_relaxed);
  }

  // Instruct whether request no longer requires processing.
  // This API may only be called from IO worker thread of the request.
  // @return: whether request has already been in "cancelled" stage
  // before calling the API.
  // Suggested usages of the API:
  // * queue/task timeout has sent load shedding response, and no further
  //   response is needed
  // * client has closed its connection and does not expect a response
  [[nodiscard]] bool tryCancel(folly::EventBase* eb) {
    eb->dcheckIsInEventBaseThread();
    return active_.exchange(false, std::memory_order_relaxed);
  }

 private:
  std::atomic<bool> active_{true};
};

} // namespace thrift
} // namespace apache
