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
#include <chrono>

#include <folly/stats/BucketedTimeSeries.h>
#include <folly/RWSpinLock.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>
#include <thrift/lib/cpp/util/kerberos/Krb5CCacheStore.h>
#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>

namespace apache { namespace thrift { namespace krb5 {

/**
 * If you need to work with a credentials cache file, use this class. It will
 * manage the file for you and only expose you to an in-memory cache.
 */
class Krb5CredentialsCacheManager {
 public:

  /**
   * The logger object here will log internal events happening in the class.
   */
  explicit Krb5CredentialsCacheManager(
    const std::shared_ptr<SecurityLogger>& logger =
      std::make_shared<SecurityLogger>(),
    int maxCacheSize = -1);

  virtual ~Krb5CredentialsCacheManager();

  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> MutexGuard;

  /**
   * Wait for a credentials cache object to become available. This will throw
   * runtime_exception if the cache is not available because of an internal
   * error.
   *
   * Note the logger object is optional here and is different from the logger
   * object in the class constructor. The logger object passed in here is
   * designed specifically to log events from waitForCache call (since it
   * occurs much more frequently than other internal events from the class).
   */
  std::shared_ptr<Krb5CCache> waitForCache(
    const Krb5Principal& service,
    SecurityLogger* logger = nullptr);

  bool fetchIsKillSwitchEnabled();

  /**
   * Test-only method. Does a busy-wait. If you need something like this in
   * production, you should consider a better implementation.
   */
  bool waitUntilCacheStoreInitialized(std::chrono::milliseconds timeoutMS
    = std::chrono::milliseconds(500));

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
  int writeOutCache(size_t limit);

  void raiseIf(krb5_error_code code, const std::string& what) {
    apache::thrift::krb5::raiseIf(ctx_->get(), code, what);
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

  std::unique_ptr<Krb5Context> ctx_;

  std::unique_ptr<Krb5CCacheStore> store_;

  /**
   * Members for controlling the manager thread
   */
  std::thread manageThread_;
  Mutex manageThreadMutex_; // A lock for the two members below
  bool stopManageThread_;
  std::condition_variable manageThreadCondVar_;
  std::shared_ptr<SecurityLogger> logger_;

  bool ccacheTypeIsMemory_;
  bool updateFileCacheEnabled_;

  // Rate limit kill switch logging. Since we have only one thread in
  // credentials cache manager, we don't need to protect access to this using
  // a mutex.
  std::chrono::time_point<std::chrono::system_clock> lastLoggedKillSwitch_;
};

}}}

#endif
