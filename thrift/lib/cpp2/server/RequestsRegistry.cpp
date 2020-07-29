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

#include <thrift/lib/cpp2/server/RequestsRegistry.h>

#include <fmt/format.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <atomic>

namespace apache {
namespace thrift {

namespace {
// RequestId storage.
// Reserve some high bits for future use. Currently the maximum id supported
// is 10^52, so thrift servers theoretically can generate unique request id
// for ~12 years, assuming the QPS is ~10 million.
const size_t kLsbBits = 52;
const uintptr_t kLsbMask = (1ull << kLsbBits) - 1;

struct RegistryIdManager {
 public:
  uint32_t getId() {
    if (!freeIds_.empty()) {
      auto id = *freeIds_.begin();
      freeIds_.erase(freeIds_.begin());
      return id;
    }

    auto id = nextId_++;
    CHECK(id < 4096);
    return id;
  }

  void returnId(uint32_t id) {
    freeIds_.insert(id);
    while (!freeIds_.empty()) {
      auto largestId = *(--freeIds_.end());
      if (largestId < nextId_ - 1) {
        return;
      }
      DCHECK(largestId == nextId_ - 1);
      --nextId_;
      freeIds_.erase(largestId);
    }
  }

 private:
  std::set<uint32_t> freeIds_;
  uint32_t nextId_;
};

folly::Synchronized<RegistryIdManager>& registryIdManager() {
  static auto* registryIdManagerPtr =
      new folly::Synchronized<RegistryIdManager>();
  return *registryIdManagerPtr;
}

} // namespace

RequestsRegistry::RequestsRegistry(
    uint64_t requestPayloadMem,
    uint64_t totalPayloadMem,
    uint16_t finishedRequestsLimit)
    : registryId_(registryIdManager().wlock()->getId()),
      payloadMemoryLimitPerRequest_(requestPayloadMem),
      payloadMemoryLimitTotal_(totalPayloadMem),
      finishedRequestsLimit_(finishedRequestsLimit) {}

RequestsRegistry::~RequestsRegistry() {
  while (!reqFinishedList_.empty()) {
    --finishedRequestsCount_;
    auto& front = reqFinishedList_.front();
    reqFinishedList_.pop_front();
    front.decRef();
  }
  DCHECK(finishedRequestsCount_ == 0);
  registryIdManager().wlock()->returnId(registryId_);
}

/* static */ std::string RequestsRegistry::getRequestId(intptr_t rootid) {
  return fmt::format("{:016x}", static_cast<uintptr_t>(rootid));
}

intptr_t RequestsRegistry::genRootId() {
  // Ensure rootid's LSB is always 1.
  // This is to prevent any collision with rootids on folly::RequestsContext() -
  // those are addresses of folly::RequestContext objects.
  return 0x1 | ((nextLocalId_++ << 1) & kLsbMask) |
      (static_cast<uintptr_t>(registryId_) << kLsbBits);
}

void RequestsRegistry::moveToFinishedList(RequestsRegistry::DebugStub& stub) {
  if (finishedRequestsLimit_ == 0) {
    return;
  }

  stub.activeRequestsRegistryHook_.unlink();
  stub.incRef();
  stub.prepareAsFinished();
  ++finishedRequestsCount_;
  reqFinishedList_.push_back(stub);

  if (finishedRequestsCount_ > finishedRequestsLimit_) {
    DCHECK(finishedRequestsLimit_ > 0);
    --finishedRequestsCount_;
    auto& front = reqFinishedList_.front();
    reqFinishedList_.pop_front();
    front.decRef();
  }
}

const std::string& RequestsRegistry::DebugStub::getMethodName() const {
  return methodNameIfFinished_.empty() ? getCpp2RequestContext().getMethodName()
                                       : methodNameIfFinished_;
}

const folly::SocketAddress* RequestsRegistry::DebugStub::getPeerAddress()
    const {
  return methodNameIfFinished_.empty()
      ? getCpp2RequestContext().getPeerAddress()
      : &peerAddressIfFinished_;
}

void RequestsRegistry::DebugStub::prepareAsFinished() {
  finished_ = std::chrono::steady_clock::now();
  rctx_.reset();
  methodNameIfFinished_ =
      const_cast<Cpp2RequestContext*>(reqContext_)->releaseMethodName();
  peerAddressIfFinished_ =
      *const_cast<Cpp2RequestContext*>(reqContext_)->getPeerAddress();
  reqContext_ = nullptr;
  req_ = nullptr;
}

void RequestsRegistry::DebugStub::incRef() noexcept {
  refCount_++;
}

void RequestsRegistry::DebugStub::decRef() noexcept {
  if (--refCount_ == 0) {
    this->~DebugStub();
    free(this);
  }
}

} // namespace thrift
} // namespace apache
