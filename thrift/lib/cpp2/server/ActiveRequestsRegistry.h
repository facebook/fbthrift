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

class Cpp2RequestContext;
class ResponseChannelRequest;

/**
 * Stores a list of request stubs in memory.
 *
 * Each IO worker stores a single ActiveRequestsRegistry instance as its
 * member, so that it can intercept and insert request data into the registry.
 *
 * Note that read operations to the request list should be always executed in
 * the same thread as write operations to avoid race condition, which means
 * most of the time reads should be issued to the event base which the
 * corresponding registry belongs to, as a task.
 */
class ActiveRequestsRegistry {
 public:
  /**
   * A piece of information which should be embedded into thrift request
   * objects.
   *
   * NOTE: Place this as the last member of a request object to ensure we never
   *       try to examine half destructed objects.
   */
  class DebugStub {
    friend class ActiveRequestsRegistry;

   public:
    DebugStub(
        ActiveRequestsRegistry& reqRegistry,
        const ResponseChannelRequest& req,
        const Cpp2RequestContext& reqContext)
        : req_(&req),
          reqContext_(&reqContext),
          timestamp_(std::chrono::steady_clock::now()) {
      reqRegistry.addDebugStub(*this);
    }

    const ResponseChannelRequest& getRequest() const {
      return *req_;
    }

    const Cpp2RequestContext& getRequestContext() const {
      return *reqContext_;
    }

    std::chrono::steady_clock::time_point getTimestamp() const {
      return timestamp_;
    }

   private:
    const ResponseChannelRequest* req_;
    const Cpp2RequestContext* reqContext_;
    std::chrono::steady_clock::time_point timestamp_;
    folly::IntrusiveListHook activeRequestsRegistryHook_;
  };
  using ActiveRequestDebugStubList =
      folly::IntrusiveList<DebugStub, &DebugStub::activeRequestsRegistryHook_>;
  void addDebugStub(DebugStub& req) {
    reqDebugStubList_.push_back(req);
  }

  const ActiveRequestDebugStubList& getDebugStubList() {
    return reqDebugStubList_;
  }

 private:
  ActiveRequestDebugStubList reqDebugStubList_;
};

} // namespace thrift
} // namespace apache
