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
#include <thrift/lib/cpp2/server/RequestDebugStub.h>

namespace apache {
namespace thrift {

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
  using ActiveRequestDebugStubList = folly::IntrusiveList<
      RequestDebugStub,
      &RequestDebugStub::activeRequestsRegistryHook_>;
  void addRequestDebugStub(RequestDebugStub& req) {
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
