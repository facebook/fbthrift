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

#include <fmt/core.h>
#include <folly/IntrusiveList.h>
#include <folly/SocketAddress.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <chrono>

namespace apache {
namespace thrift {

class Cpp2RequestContext;
class ResponseChannelRequest;

/**
 * Stores a list of request stubs in memory.
 *
 * Each IO worker stores a single RequestsRegistry instance as its
 * member, so that it can intercept and insert request data into the registry.
 *
 * Note that read operations to the request list should be always executed in
 * the same thread as write operations to avoid race condition, which means
 * most of the time reads should be issued to the event base which the
 * corresponding registry belongs to, as a task.
 */
class RequestsRegistry {
  /**
   * A wrapper class for request payload we are tracking, to encapsulate the
   * details of processing and storing the real request buffer.
   */
  class DebugPayload {
   public:
    DebugPayload(std::unique_ptr<folly::IOBuf> data) : data_(std::move(data)) {}
    DebugPayload(const DebugPayload&) = delete;
    DebugPayload& operator=(const DebugPayload&) = delete;
    bool hasData() const {
      return data_ != nullptr;
    }
    std::unique_ptr<folly::IOBuf> cloneData() const {
      if (!data_) {
        return std::make_unique<folly::IOBuf>();
      }
      return data_->clone();
    }
    size_t dataSize() const {
      if (!data_) {
        return 0;
      }
      return data_->computeChainDataLength();
    }
    /**
     * User must call hasData() to ensure the underlying buffer is present
     * before releasing the buffer.
     */
    void releaseData() {
      DCHECK(data_);
      folly::IOBuf::destroy(std::move(data_));
    }

   private:
    std::unique_ptr<folly::IOBuf> data_;
  };

 public:
  /**
   * A small piece of information associated with those thrift requests that
   * we are tracking in the registry. The stub lives alongside the request
   * in the same chunk of memory.
   * Requests Registry is just a fancy list of such DebugStubs.
   *
   * DebugStub tracks request payload to its corresponding thrift
   * request. Handles to the payloads can be optionally released by its
   * parent request registry, indicating the payload memory has been reclaimed
   * to control memory usage. DebugStub should be unlinked from lists
   * only during:
   *   1. Destruction of DebugStub.
   *   2. Memory collection from RequestsRegistry.
   */
  class DebugStub {
    friend class RequestsRegistry;

   public:
    DebugStub(
        RequestsRegistry& reqRegistry,
        const ResponseChannelRequest& req,
        const Cpp2RequestContext& reqContext,
        std::shared_ptr<folly::RequestContext> rctx,
        protocol::PROTOCOL_TYPES protoId,
        std::unique_ptr<folly::IOBuf> payload)
        : req_(&req),
          reqContext_(&reqContext),
          rctx_(std::move(rctx)),
          protoId_(protoId),
          payload_(std::move(payload)),
          timestamp_(std::chrono::steady_clock::now()),
          registry_(&reqRegistry),
          rootRequestContextId_(rctx_->getRootId()) {
      reqRegistry.registerStub(*this);
    }

    /**
     * DebugStub objects are oblivious to memory collection, but they should
     * notify their owner registry when unlinking themselves.
     */
    ~DebugStub() {
      if (payload_.hasData()) {
        DCHECK(activeRequestsPayloadHook_.is_linked());
        registry_->onStubPayloadUnlinked(*this);
      }
    }

    const ResponseChannelRequest& getRequest() const {
      return *req_;
    }

    const Cpp2RequestContext& getCpp2RequestContext() const {
      return *reqContext_;
    }

    std::chrono::steady_clock::time_point getTimestamp() const {
      return timestamp_;
    }

    std::chrono::steady_clock::time_point getFinished() const {
      return finished_;
    }

    intptr_t getRootRequestContextId() const {
      return rootRequestContextId_;
    }

    std::shared_ptr<folly::RequestContext> getRequestContext() const {
      return rctx_;
    }

    const std::string& getMethodName() const;
    const folly::SocketAddress* getPeerAddress() const;

    /**
     * Clones the payload buffer to data accessors. If the buffer is already
     * released by memory collection, returns an empty unique_ptr.
     * Since RequestsRegistry doesn'y provide synchronization by default,
     * this should be called from the IO worker which also owns the same
     * RequestsRegistry.
     */
    std::unique_ptr<folly::IOBuf> clonePayload() const {
      return payload_.cloneData();
    }

    protocol::PROTOCOL_TYPES getProtoId() const {
      return protoId_;
    }

   private:
    uint64_t getPayloadSize() const {
      return payload_.dataSize();
    }
    void releasePayload() {
      payload_.releaseData();
    }

    void prepareAsFinished();

    void incRef() noexcept;
    void decRef() noexcept;

    std::string methodNameIfFinished_;
    folly::SocketAddress peerAddressIfFinished_;
    const ResponseChannelRequest* req_;
    const Cpp2RequestContext* reqContext_;
    std::shared_ptr<folly::RequestContext> rctx_;
    const protocol::PROTOCOL_TYPES protoId_;
    DebugPayload payload_;
    std::chrono::steady_clock::time_point timestamp_;
    std::chrono::steady_clock::time_point finished_{
        std::chrono::steady_clock::duration::zero()};
    RequestsRegistry* registry_;
    const intptr_t rootRequestContextId_;
    folly::IntrusiveListHook activeRequestsPayloadHook_;
    folly::IntrusiveListHook activeRequestsRegistryHook_;
    size_t refCount_{1};
  };

  class Deleter {
   public:
    Deleter(DebugStub* stub = nullptr) : stub_(stub) {}
    template <typename U>
    /* implicit */ Deleter(std::default_delete<U>&&) : stub_(nullptr) {}

    template <typename T>
    void operator()(T* p) {
      if (!stub_) {
        delete p;
      } else {
        stub_->registry_->moveToFinishedList(*stub_);
        p->~T();
        // We release ownership over the stub, but it still may be held alive
        // by reqFinishedList_
        stub_->decRef();
      }
    }

    template <typename U>
    Deleter& operator=(std::default_delete<U>&&) {
      stub_ = nullptr;
      return *this;
    }

   private:
    DebugStub* stub_;
  };

  template <typename T, typename... Args>
  static std::unique_ptr<T, Deleter> makeRequest(Args&&... args) {
    static_assert(std::is_base_of<ResponseChannelRequest, T>::value, "");
    auto offset = sizeof(std::aligned_storage_t<sizeof(DebugStub), alignof(T)>);
    DebugStub* pStub = reinterpret_cast<DebugStub*>(malloc(offset + sizeof(T)));
    T* pT = reinterpret_cast<T*>(reinterpret_cast<char*>(pStub) + offset);
    new (pT) T(*pStub, std::forward<Args>(args)...);
    return std::unique_ptr<T, Deleter>(pT, pStub);
  }

  intptr_t genRootId();
  static std::string getRequestId(intptr_t rootid);

  using ActiveRequestDebugStubList =
      folly::IntrusiveList<DebugStub, &DebugStub::activeRequestsRegistryHook_>;
  using ActiveRequestPayloadList =
      folly::IntrusiveList<DebugStub, &DebugStub::activeRequestsPayloadHook_>;

  RequestsRegistry(
      uint64_t requestPayloadMem,
      uint64_t totalPayloadMem,
      uint16_t finishedRequestsLimit);
  ~RequestsRegistry();

  const ActiveRequestDebugStubList& getActive() {
    return reqActiveList_;
  }

  const ActiveRequestDebugStubList& getFinished() {
    return reqFinishedList_;
  }

  void registerStub(DebugStub& req) {
    uint64_t payloadSize = req.getPayloadSize();
    reqActiveList_.push_back(req);
    if (payloadSize > payloadMemoryLimitPerRequest_) {
      req.releasePayload();
      return;
    }
    reqPayloadList_.push_back(req);
    payloadMemoryUsage_ += payloadSize;
    evictStubPayloads();
  }

 private:
  void moveToFinishedList(DebugStub& stub);

  void evictStubPayloads() {
    while (payloadMemoryUsage_ > payloadMemoryLimitTotal_) {
      auto& stub = nextStubToEvict();

      onStubPayloadUnlinked(stub);
      reqPayloadList_.erase(reqPayloadList_.iterator_to(stub));
      stub.releasePayload();
    }
  }
  DebugStub& nextStubToEvict() {
    return reqPayloadList_.front();
  }
  void onStubPayloadUnlinked(const DebugStub& stub) {
    uint64_t payloadSize = stub.getPayloadSize();
    DCHECK(payloadMemoryUsage_ >= payloadSize);
    payloadMemoryUsage_ -= payloadSize;
  }
  uint32_t registryId_;
  uint64_t nextLocalId_{0};
  uint64_t payloadMemoryLimitPerRequest_;
  uint64_t payloadMemoryLimitTotal_;
  uint64_t payloadMemoryUsage_{0};
  ActiveRequestDebugStubList reqActiveList_;
  ActiveRequestPayloadList reqPayloadList_;
  ActiveRequestDebugStubList reqFinishedList_;
  uint16_t finishedRequestsCount_{0};
  uint16_t finishedRequestsLimit_;
};

} // namespace thrift
} // namespace apache
