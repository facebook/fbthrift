/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KRB5_CREDENTIALS_CACHE_MANAGER
#define KRB5_CREDENTIALS_CACHE_MANAGER

#include <condition_variable>
#include <iostream>
#include <krb5.h>
#include <string>
#include <thread>
#include <unordered_map>

#include "folly/stats/BucketedTimeSeries.h"
#include "folly/RWSpinLock.h"
#include "thrift/lib/cpp/util/kerberos/Krb5Util.h"

namespace apache { namespace thrift { namespace krb5 {

/**
 * If you need to work with a credentials cache file, use this class. It will
 * manage the file for you and only expose you to an in-memory cache.
 */
class Krb5CredentialsCacheManager {
 public:

  /**
   * Specify the client to use when doing kInit.
   */
  explicit Krb5CredentialsCacheManager(const std::string& client = "");

  virtual ~Krb5CredentialsCacheManager();

  /**
   * Wait for a credentials cache object to become available.
   */
  std::shared_ptr<Krb5CCache> waitForCache();

  /**
   * Call this whenever a credential is used. This will allow this manager
   * to keep track of credentials usage. Note that this method may
   * potentially hold a lock.
   */
  void incUsedService(const std::string& service);

 protected:
  static const int MANAGE_THREAD_SLEEP_PERIOD;
  static const int SERVICE_HISTOGRAM_NUM_BUCKETS;
  static const int SERVICE_HISTOGRAM_PERIOD;
  static const int ABOUT_TO_EXPIRE_THRESHOLD;
  static const int NUM_ELEMENTS_TO_PERSIST_TO_FILE;

  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> MutexGuard;

  typedef folly::RWSpinLock Lock;
  typedef folly::RWSpinLock::WriteHolder WriteLock;
  typedef folly::RWSpinLock::ReadHolder ReadLock;
  typedef folly::RWSpinLock::UpgradedHolder UpgradeLock;

  /**
   * Returns the pointer to the currently active credentials cache. There
   * could be multiple active caches at any given time.
   */
  std::shared_ptr<Krb5CCache> getCache();

  /**
   * Run kInit on the in-memory cache. All the old credentials will be lost
   * in the new cache returned by getCache().
   */
  std::unique_ptr<Krb5CCache> kInit();

  /**
   * Run kInit on a new cache, and try to renew all the credentials that are in
   * the old cache.
   */
  std::unique_ptr<Krb5CCache> buildRenewedCache();

  /**
   * Read in credentials from the default CC file
   */
  std::unique_ptr<Krb5CCache> readInCache();

  /**
   * Persist the in-memory CC to the default credentials cache file.
   * This will persist the most frequently accessed credentials
   * (up to `limit`).
   */
  void writeOutCache(size_t limit);

  void importMemoryCache(std::shared_ptr<Krb5CCache> cache);

  /**
   * Do tgs request and store the ticket in provided ccache.
   */
  void doTgsReq(krb5_principal server, Krb5CCache& cache);

  void raiseIf(krb5_error_code code, const std::string& what) {
    apache::thrift::krb5::raiseIf(ctx_.get(), code, what);
  }

  void stopThread();

  class ServiceTimeSeries {
   public:
     ServiceTimeSeries();
     void bumpCount();
     uint64_t getCount();
     std::string getName();

   private:
     folly::BucketedTimeSeries<uint64_t> timeSeries_;
     Lock serviceTimeseriesLock_;
  };

  Krb5Context ctx_;
  std::string clientString_;
  Krb5Principal client_;

  // Count struct
  mutable Lock serviceCountLock_;
  std::unordered_map<std::string, ServiceTimeSeries> serviceCountMap_;

  // In-memory cache. Will be shared across different threads. Operations
  // on the CC should be thread-safe within the krb5 library.
  Mutex ccLock_; // A lock for ccMemory_
  std::shared_ptr<Krb5CCache> ccMemory_;
  std::condition_variable ccMemoryCondVar_;

  // Members for controlling the manager thread
  std::thread manageThread_;
  Mutex manageThreadMutex_; // A lock for the two members below
  bool stopManageThread_;
  std::condition_variable manageThreadCondVar_;

};

}}}

#endif
