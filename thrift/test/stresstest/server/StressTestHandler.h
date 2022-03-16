/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/test/stresstest/if/gen-cpp2/StressTest.h>

namespace apache {
namespace thrift {
namespace stress {

class StressTestHandler : public StressTestSvIf {
 public:
  StressTestHandler() {}

  void async_eb_ping(std::unique_ptr<HandlerCallback<void>> callback) override {
    callback->done();
  }

  void async_tm_requestResponseTm(
      std::unique_ptr<HandlerCallback<std::unique_ptr<BasicResponse>>> callback,
      std::unique_ptr<BasicRequest> request) override {
    if (request->processInfo()->processingMode() == ProcessingMode::Async) {
      auto* tm = callback->getThreadManager();
      tm->add([this,
               callback = std::move(callback),
               request = std::move(request)]() mutable {
        requestResponseImpl(std::move(callback), std::move(request));
      });
      return;
    }
    requestResponseImpl(std::move(callback), std::move(request));
  }

  void async_eb_requestResponseEb(
      std::unique_ptr<HandlerCallback<std::unique_ptr<BasicResponse>>> callback,
      std::unique_ptr<BasicRequest> request) override {
    if (request->processInfo()->processingMode() == ProcessingMode::Async) {
      auto* evb = callback->getEventBase();
      evb->add([this,
                callback = std::move(callback),
                request = std::move(request)]() mutable {
        requestResponseImpl(std::move(callback), std::move(request));
      });
      return;
    }
    requestResponseImpl(std::move(callback), std::move(request));
  }

 private:
  void requestResponseImpl(
      std::unique_ptr<HandlerCallback<std::unique_ptr<BasicResponse>>> callback,
      std::unique_ptr<BasicRequest> request) const {
    if (auto processingTime = *request->processInfo()->processingTimeMs();
        processingTime > 0) {
      switch (*request->processInfo()->workSimulationMode()) {
        case WorkSimulationMode::Default: {
          auto deadline = std::chrono::steady_clock::now() +
              std::chrono::milliseconds(processingTime);
          while (std::chrono::steady_clock::now() < deadline) {
            // wait for deadline
          }
          break;
        }
        case WorkSimulationMode::Sleep: {
          /* sleep override */ std::this_thread::sleep_for(
              std::chrono::milliseconds(processingTime));
          break;
        }
      }
    }
    BasicResponse response;
    if (auto responseSize = *request->processInfo()->responseSize();
        responseSize > 0) {
      response.payload() = std::string('x', responseSize);
    }
    callback->result(std::move(response));
  }
};

} // namespace stress
} // namespace thrift
} // namespace apache
