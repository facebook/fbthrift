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

#include <memory>

#include <folly/ExceptionWrapper.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>

namespace apache::thrift {
class Cpp2ConnContext;
class Cpp2RequestContext;
} // namespace apache::thrift

namespace apache::thrift::fast_thrift::thrift::server {

struct Cpp2BridgeRequestException {
  folly::exception_wrapper exception;
  bool declared{false};
};

struct Cpp2BridgeRequestState {
  apache::thrift::Cpp2RequestContext* context{nullptr};
  std::unique_ptr<Cpp2BridgeRequestException> exception;
};

struct Cpp2BridgeExtension {
  EXTENSION_ID(cpp2_bridge);
  using ConnState = apache::thrift::Cpp2ConnContext;
  using RequestState = Cpp2BridgeRequestState;
};

inline void recordCpp2BridgeException(
    ThriftRequestContext* requestContext,
    const folly::exception_wrapper& exception,
    bool declared) noexcept {
  if (requestContext == nullptr) {
    return;
  }
  auto* state = requestContext->tryState<Cpp2BridgeExtension>();
  if (state == nullptr) {
    return;
  }
  try {
    state->exception = std::make_unique<Cpp2BridgeRequestException>(
        Cpp2BridgeRequestException{exception, declared});
  } catch (...) {
    // Event-handler instrumentation is best effort under allocation failure.
  }
}

} // namespace apache::thrift::fast_thrift::thrift::server
