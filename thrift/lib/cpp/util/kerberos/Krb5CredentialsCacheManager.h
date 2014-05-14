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
#include "thrift/lib/cpp2/security/SecurityLogger.h"
#include "thrift/lib/cpp/util/kerberos/Krb5CCacheStore.h"
#include "thrift/lib/cpp/util/kerberos/Krb5Util.h"

namespace apache { namespace thrift { namespace krb5 {

/**
 * If you need to work with a credentials cache file, use this class. It will
 * manage the file for you and only expose you to an in-memory cache.
 */
class Krb5CredentialsCacheManager {
 public:

  explicit Krb5CredentialsCacheManager(
    const std::shared_ptr<SecurityLogger>& logger =
      std::make_shared<SecurityLogger>());

  virtual ~Krb5CredentialsCacheManager();

  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> MutexGuard;

  /**
   * Wait for a credentials cache object to become available. This will throw
   * runtime_exception if the cache is not available because of an internal
   * error.
   */
  std::shared_ptr<Krb5CCache> waitForCache(const Krb5Principal& service);

 protected:
  static const int MANAGE_THREAD_SLEEP_PERIOD;
  static const int ABOUT_TO_EXPIRE_THRESHOLD;
  static const int NUM_ELEMENTS_TO_PERSIST_TO_FILE;

  /**
   * Read in credentials from the default CC file. Throws if
   * cache is invalid or about to expire.
   */
  std::unique_ptr<Krb5CCache> readInCache();

  /**
   * Persist the in-memory CC to the default credentials cache file.
   * This will persist the most frequently accessed credentials
   * (up to `limit`).
   */
  void writeOutCache(size_t limit);

  void raiseIf(krb5_error_code code, const std::string& what) {
    apache::thrift::krb5::raiseIf(ctx_.get(), code, what);
  }

  void stopThread();

  /**
   * Keytab access helpers.
   */
  std::unique_ptr<Krb5Principal> getFirstPrincipalInKeytab();
  bool isPrincipalInKeytab(const Krb5Principal& princ);

  void initCacheStore();

  bool aboutToExpire(const std::pair<uint64_t, uint64_t>& lifetime);
  bool reachedRenewTime(
    const std::pair<uint64_t, uint64_t>& lifetime, const std::string& client);

  Krb5Context ctx_;

  Krb5CCacheStore store_;

  /**
   * Members for controlling the manager thread
   */
  std::thread manageThread_;
  Mutex manageThreadMutex_; // A lock for the two members below
  bool stopManageThread_;
  std::condition_variable manageThreadCondVar_;
  std::shared_ptr<SecurityLogger> logger_;
};

}}}

#endif
