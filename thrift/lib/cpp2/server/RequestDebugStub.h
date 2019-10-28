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

#include <folly/IntrusiveList.h>
#include <chrono>

namespace apache {
namespace thrift {

class ActiveRequestsRegistry;
class Cpp2RequestContext;
class ResponseChannelRequest;

/**
 * A piece of information which should be embedded into thrift request objects.
 *
 * NOTE: Place this as the last member of a request object to ensure we never
 *       try to examine half destructed objects.
 */
class RequestDebugStub {
  friend class ActiveRequestsRegistry;

 public:
  RequestDebugStub(
      ActiveRequestsRegistry& reqRegistry,
      const ResponseChannelRequest& req,
      const Cpp2RequestContext& reqContext);

  const ResponseChannelRequest& getRequest() {
    return *req_;
  }

  const Cpp2RequestContext& getRequestContext() {
    return *reqContext_;
  }

  std::chrono::steady_clock::time_point getTimestamp() {
    return timestamp_;
  }

 private:
  const ResponseChannelRequest* req_;
  const Cpp2RequestContext* reqContext_;
  std::chrono::steady_clock::time_point timestamp_;
  folly::IntrusiveListHook activeRequestsRegistryHook_;
};

} // namespace thrift
} // namespace apache
