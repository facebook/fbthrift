/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "thrift/lib/cpp/transport/TMemPagedFactory.h"

#include <cstdlib>
#include "thrift/lib/cpp/transport/TTransportException.h"

namespace apache { namespace thrift { namespace transport {

/**
 * Constructor
 */
FixedSizeMemoryPageFactory::FixedSizeMemoryPageFactory(
                          size_t pageSize, //!< page size in bytes
                          size_t maxMemoryUsage, //!< max memory usage in bytes
                          size_t cacheMemorySize) : //!< max memory in cache
  pageSize_(pageSize),
  maxMemoryUsage_(maxMemoryUsage),
  cacheMemorySize_(cacheMemorySize),
  numAllocatedPages_(0),
  numCachedPages_(0),
  cachedPages_(nullptr) {
}

/**
 * destructor
 */
FixedSizeMemoryPageFactory::~FixedSizeMemoryPageFactory() {
  releaseMemory();
}

/**
 * release all unused memory
 */
void FixedSizeMemoryPageFactory::releaseMemory() {
  facebook::SpinLockHolder guard(&lock_);
  while (cachedPages_) { // release all cached pages
    FixedSizeMemoryPage* page = cachedPages_;
    cachedPages_ = cachedPages_->next_;
    --numCachedPages_;
    free(page);
    --numAllocatedPages_;
  }
  GlobalOutput.printf("allocated pages %d, cached pages %d",
        numAllocatedPages_,
        numCachedPages_);
}

/*
 * request for a page allocation
 */
FixedSizeMemoryPage* FixedSizeMemoryPageFactory::getPage(bool throwOnError) {
  FixedSizeMemoryPage* page = nullptr;
  // lock
  {
    facebook::SpinLockHolder guard(&lock_);
    if ((page = cachedPages_) != nullptr) { // cache is available
      cachedPages_ = cachedPages_->next_;
      --numCachedPages_; // get from cache
    } else { // allocate new page
      // check capacity
      if (numAllocatedPages_ * pageSize_ >= maxMemoryUsage_) {
        GlobalOutput.printf("FixedSizeMemoryPage::getPage: alloc %d, max %d",
          numAllocatedPages_ * pageSize_, maxMemoryUsage_);
        if (throwOnError) {
          throw TTransportException(TTransportException::INTERNAL_ERROR);
        }
        return nullptr;
      }

      page = (FixedSizeMemoryPage*)malloc(pageSize_ // memory itself
             + sizeof(FixedSizeMemoryPage)); // + object size
      if (!page) { // no memory available
        if (throwOnError) {
          throw TTransportException(TTransportException::INTERNAL_ERROR);
        }
        return nullptr;
      }
      ++numAllocatedPages_;
    }
  }
  // init page
  page->next_ = nullptr;
  return page;
}

size_t FixedSizeMemoryPageFactory::getPageSize() const {
  return pageSize_;
}

void FixedSizeMemoryPageFactory::returnPage(FixedSizeMemoryPage* page) {
  // lock
  facebook::SpinLockHolder guard(&lock_);
  // put page back to cache if not exceed cache
  if (numCachedPages_ * pageSize_ < cacheMemorySize_) {
    page->next_ = cachedPages_;
    cachedPages_ = page;
    ++numCachedPages_;
  } else {
    free(page);
    --numAllocatedPages_;
  }
}

}}} // apache::thrift::transport
