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

#include <thrift/lib/cpp2/server/ActiveRequestsRegistry.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/RequestId.h>
#include <atomic>

namespace apache {
namespace thrift {

namespace {
// Reserve some high bits for future use. Currently the maximum id supported
// is 10^52, so thrift servers theoretically can generate unique request id
// for ~12 years, assuming the QPS is ~10 million.
const size_t RequestLocalIdBits = 52;
const uint64_t RequestLocalIdMax = (1ull << RequestLocalIdBits) - 1;

std::atomic<uint32_t> nextRegistryId{0};
} // namespace

ActiveRequestsRegistry::ActiveRequestsRegistry(
    uint64_t requestPayloadMem,
    uint64_t totalPayloadMem,
    uint16_t finishedRequestsLimit)
    : registryId_(nextRegistryId++),
      payloadMemoryLimitPerRequest_(requestPayloadMem),
      payloadMemoryLimitTotal_(totalPayloadMem),
      finishedRequestsLimit_(finishedRequestsLimit) {}

ActiveRequestsRegistry::~ActiveRequestsRegistry() {
  while (!reqFinishedList_.empty()) {
    reqFinishedList_.front().dispose();
  }
}

RequestId ActiveRequestsRegistry::genRequestId() {
  return RequestId(registryId_, (nextLocalId_++) & RequestLocalIdMax);
}

void ActiveRequestsRegistry::moveToFinishedList(
    ActiveRequestsRegistry::DebugStub& stub) {
  stub.activeRequestsRegistryHook_.unlink();
  stub.prepareAsFinished();
  ++finishedRequestsCount_;
  reqFinishedList_.push_back(stub);
}

void ActiveRequestsRegistry::evictFromFinishedList() {
  if (finishedRequestsCount_ > finishedRequestsLimit_) {
    --finishedRequestsCount_;
    reqFinishedList_.front().dispose();
  }
}

const std::string& ActiveRequestsRegistry::DebugStub::getMethodName() const {
  return methodNameIfFinished_.empty() ? getRequestContext().getMethodName()
                                       : methodNameIfFinished_;
}

const folly::SocketAddress* ActiveRequestsRegistry::DebugStub::getPeerAddress()
    const {
  return methodNameIfFinished_.empty() ? getRequestContext().getPeerAddress()
                                       : &peerAddressIfFinished_;
}

void ActiveRequestsRegistry::DebugStub::prepareAsFinished() {
  finished_ = std::chrono::steady_clock::now();
  methodNameIfFinished_ =
      const_cast<Cpp2RequestContext*>(reqContext_)->releaseMethodName();
  peerAddressIfFinished_ =
      *const_cast<Cpp2RequestContext*>(reqContext_)->getPeerAddress();
  reqContext_ = nullptr;
  req_ = nullptr;
}

void ActiveRequestsRegistry::DebugStub::dispose() {
  this->~DebugStub();
  free(this);
}

} // namespace thrift
} // namespace apache
