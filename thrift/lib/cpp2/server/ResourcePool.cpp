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

#include <stdexcept>
#include <thrift/lib/cpp2/server/ResourcePool.h>

namespace apache::thrift {

// ResourcePool

ResourcePool::ResourcePool(
    std::unique_ptr<RequestPileInterface>&& requestPile,
    std::shared_ptr<folly::ThreadPoolExecutor> executor,
    std::unique_ptr<ConcurrencyControllerInterface>&& concurrencyController,
    std::string_view name)
    : requestPile_(std::move(requestPile)),
      executor_(executor),
      concurrencyController_(std::move(concurrencyController)),
      name_(name) {
  // Current preconditions - either we have all three of these or none of them
  if (requestPile_ && concurrencyController_ && executor_) {
    // This is an async pool - that's allowed.
  } else {
    // This is a sync/eb pool.
    DCHECK(!requestPile_ && !concurrencyController && !executor_);
  }
}

ResourcePool::~ResourcePool() {
  if (concurrencyController_) {
    concurrencyController_->stop();
  }
}

std::optional<ServerRequestRejection> ResourcePool::accept(
    ServerRequest&& request) {
  if (requestPile_) {
    // This pool is async, enqueue it on the requestPile
    auto result = requestPile_->enqueue(std::move(request));
    if (result) {
      return result;
    }
    concurrencyController_->onEnqueued();
    return std::optional<ServerRequestRejection>(std::nullopt);
  } else {
    // Trigger processing of request and check for queue timeouts.
    if (!request.request()->getShouldStartProcessing()) {
      auto eb = detail::ServerRequestHelper::eventBase(request);
      HandlerCallbackBase::releaseRequest(
          detail::ServerRequestHelper::request(std::move(request)), eb);
      return std::optional<ServerRequestRejection>(std::nullopt);
    }

    // This pool is sync, just now we execute the request inline.
    AsyncProcessorHelper::executeRequest(std::move(request));
    return std::optional<ServerRequestRejection>(std::nullopt);
  }
}

// ResourcePoolSet

void ResourcePoolSet::setResourcePool(
    ResourcePoolHandle const& handle,
    std::unique_ptr<RequestPileInterface>&& requestPile,
    std::shared_ptr<folly::ThreadPoolExecutor> executor,
    std::unique_ptr<ConcurrencyControllerInterface>&& concurrencyController) {
  if (resourcePoolsLock_) {
    throw std::logic_error("Cannot setResourcePool() after lock()");
  }
  auto rp = resourcePools_.wlock();
  rp->resize(std::max(rp->size(), handle.index() + 1));
  if (rp->at(handle.index())) {
    LOG(ERROR) << "Cannot overwrite resourcePool:" << handle.name();
    throw std::invalid_argument("Cannot overwrite resourcePool");
  }
  std::unique_ptr<ResourcePool> pool{new ResourcePool{
      std::move(requestPile),
      executor,
      std::move(concurrencyController),
      handle.name()}};
  rp->at(handle.index()) = std::move(pool);
}

ResourcePoolHandle ResourcePoolSet::addResourcePool(
    std::string_view poolName,
    std::unique_ptr<RequestPileInterface>&& requestPile,
    std::shared_ptr<folly::ThreadPoolExecutor> executor,
    std::unique_ptr<ConcurrencyControllerInterface>&& concurrencyController) {
  if (resourcePoolsLock_) {
    throw std::logic_error("Cannot addResourcePool() after lock()");
  }
  auto rp = resourcePools_.wlock();
  std::unique_ptr<ResourcePool> pool{new ResourcePool{
      std::move(requestPile),
      executor,
      std::move(concurrencyController),
      poolName}};
  // Ensure that any default slots have been initialized (with empty unique_ptr
  // if necessary).
  rp->resize(std::max(rp->size(), ResourcePoolHandle::kMaxReservedHandle + 1));
  rp->emplace_back(std::move(pool));
  return ResourcePoolHandle::makeHandle(poolName, rp->size() - 1);
}

void ResourcePoolSet::lock() const {
  auto lock = resourcePools_.rlock();
  resourcePoolsLock_ = std::move(lock);
}

size_t ResourcePoolSet::numQueued() const {
  size_t sum = 0;

  auto lResourcePool = resourcePools_.rlock();

  for (auto& pool : *lResourcePool) {
    if (pool->requestPile_) {
      sum += pool->requestPile_->requestCount();
    }
  }
  return sum;
}

size_t ResourcePoolSet::numInExecution() const {
  size_t sum = 0;

  auto lResourcePool = resourcePools_.rlock();

  for (auto& pool : *lResourcePool) {
    if (pool->concurrencyController_) {
      sum += pool->concurrencyController_->requestCount();
    }
  }
  return sum;
}

std::optional<ResourcePoolHandle> ResourcePoolSet::findResourcePool(
    std::string_view poolName) const {
  ResourcePools::ConstLockedPtr localLock;
  auto& lock = resourcePoolsLock_ ? resourcePoolsLock_
                                  : localLock = resourcePools_.rlock();
  for (std::size_t i = 0; i < lock->size(); ++i) {
    if (lock->at(i) && lock->at(i)->name() == poolName) {
      return ResourcePoolHandle::makeHandle(poolName, i);
    }
  }
  return std::nullopt;
}

bool ResourcePoolSet::hasResourcePool(const ResourcePoolHandle& handle) const {
  ResourcePools::ConstLockedPtr localLock;
  auto& lock = resourcePoolsLock_ ? resourcePoolsLock_
                                  : localLock = resourcePools_.rlock();
  if (handle.index() >= lock->size()) {
    return false;
  }
  return static_cast<bool>((*lock)[handle.index()]);
}

ResourcePool& ResourcePoolSet::resourcePool(
    const ResourcePoolHandle& handle) const {
  ResourcePools::ConstLockedPtr localLock;
  auto& lock = resourcePoolsLock_ ? resourcePoolsLock_
                                  : localLock = resourcePools_.rlock();
  DCHECK_LT(handle.index(), lock->size());
  DCHECK((*lock)[handle.index()]);
  return *(*lock)[handle.index()].get();
}

bool ResourcePoolSet::empty() const {
  ResourcePools::ConstLockedPtr localLock;
  auto& lock = resourcePoolsLock_ ? resourcePoolsLock_
                                  : localLock = resourcePools_.rlock();
  return lock->empty();
}

void ResourcePoolSet::stopAndJoin() {
  resourcePoolsLock_.unlock();
  auto rp = resourcePools_.wlock();
  for (auto& resourcePool : *rp) {
    if (resourcePool && resourcePool->concurrencyController()) {
      resourcePool->concurrencyController().value()->stop();
    }
  }
  for (auto& resourcePool : *rp) {
    if (resourcePool && resourcePool->executor()) {
      resourcePool->executor().value()->join();
    }
  }
}

} // namespace apache::thrift
