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

#ifndef KRB5_CCACHE_STORE
#define KRB5_CCACHE_STORE

#include <krb5.h>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>

#include <folly/stats/BucketedTimeSeries.h>
#include <thrift/lib/cpp/util/kerberos/Krb5Tgts.h>
#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>

#include <folly/SharedMutex.h>

namespace apache { namespace thrift { namespace krb5 {

class Krb5CCacheStore {
 public:
  /**
   * maxCacheSize specifies the max size of the in-memory cache. If
   * it is 0, then no cache is used, if it is < 0, then there is no limit.
   */
  explicit Krb5CCacheStore(
      const std::shared_ptr<SecurityLogger>& logger,
      int maxCacheSize)
    : maxCacheSize_(maxCacheSize)
    , logger_(logger) {}
  virtual ~Krb5CCacheStore() {}

  std::shared_ptr<Krb5CCache> waitForCache(
      const Krb5Principal& service,
      SecurityLogger* logger = nullptr,
      bool* didInitCacheForService = nullptr);

  void kInit(const Krb5Principal& client);
  bool isInitialized();
  void notifyOfError(const std::string& error);

  /**
   * Start using the credentials in the provided cache object.
   */
  void importCache(Krb5CCache& cache);
  uint64_t renewCreds();
  std::unique_ptr<Krb5CCache> exportCache(size_t limit);

  Krb5Principal getClientPrincipal();
  /**
   * Get lifetime of the currently loaded creds.
   */
  Krb5Lifetime getLifetime();

  std::map<std::string, Krb5Lifetime> getServicePrincipalLifetimes(
      size_t limit);
  std::pair<std::string, Krb5Lifetime> getLifetimeOfFirstServicePrincipal(
      const std::shared_ptr<Krb5CCache>& cache);
  std::map<std::string, Krb5Lifetime> getTgtLifetimes();

 protected:
  static const int SERVICE_HISTOGRAM_NUM_BUCKETS;
  static const int SERVICE_HISTOGRAM_PERIOD;
  static const uint32_t EXPIRATION_THRESHOLD_SEC;

  /**
   * Stores data about a service: how often it's accessed,
   * the associated credentials cache.
   */
  class ServiceData {
   public:
     ServiceData();
     void bumpCount();
     uint64_t getCount();

     folly::SharedMutex lockTimeSeries;
     folly::BucketedTimeSeries<uint64_t> timeSeries;
     folly::SharedMutex lockCache;
     // Credentials cache for the service
     std::shared_ptr<Krb5CCache> cache;
     // Indicates when we want this cache to expire.
     uint64_t expires;
  };

  /**
   * Given a service, fill an empty cache with krbtgts, making it possible
   * to later fetch the service cred from the kdc.
   */
  std::unique_ptr<Krb5CCache> initCacheForService(
    const Krb5Principal& service,
    const krb5_creds* creds,
    SecurityLogger* logger,
    uint64_t& expires);

  std::shared_ptr<ServiceData> getServiceDataPtr(const Krb5Principal& service);
  std::vector<Krb5Principal> getServicePrincipalList();
  std::set<std::string> getTopServices(size_t limit);

  void raiseIf(krb5_error_code code, const std::string& what) {
    apache::thrift::krb5::raiseIf(ctx_.get(), code, what);
  }

  Krb5Context ctx_;

  /**
   * Map from service principal to data about it. For each service we will
   * store the number of times it's used as well a credential cache
   * associated with it.
   */
  typedef std::unordered_map<std::string, std::shared_ptr<ServiceData>>
    DataMapType;
  std::queue<std::string> cacheItemQueue_;
  int maxCacheSize_;

  folly::SharedMutex serviceDataMapLock_;
  DataMapType serviceDataMap_;

  /**
   * Storage for krbtgt credentials
   */
  Krb5Tgts tgts_;
  std::shared_ptr<SecurityLogger> logger_;
};

}}}

#endif
