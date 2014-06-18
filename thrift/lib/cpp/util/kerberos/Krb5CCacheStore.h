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

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <condition_variable>
#include <iostream>
#include <krb5.h>
#include <string>
#include <thread>
#include <unordered_map>

#include "folly/stats/BucketedTimeSeries.h"
#include "thrift/lib/cpp/util/kerberos/Krb5Tgts.h"
#include "thrift/lib/cpp/util/kerberos/Krb5Util.h"
#include "thrift/lib/cpp2/security/SecurityLogger.h"

namespace apache { namespace thrift { namespace krb5 {

class Krb5CCacheStore {
 public:
  explicit Krb5CCacheStore(const std::shared_ptr<SecurityLogger>& logger)
    : logger_(logger) {}
  virtual ~Krb5CCacheStore() {}

  typedef boost::shared_mutex Lock;
  typedef boost::unique_lock<Lock> WriteLock;
  typedef boost::shared_lock<Lock> ReadLock;

  std::shared_ptr<Krb5CCache> waitForCache(
    const Krb5Principal& service,
    SecurityLogger* logger = nullptr);

  void kInit(const Krb5Principal& client);
  bool isInitialized();
  void notifyOfError(const std::string& error);

  /**
   * Start using the credentials in the provided cache object.
   */
  void importCache(Krb5CCache& cache);
  void renewCreds();
  std::unique_ptr<Krb5CCache> exportCache(size_t limit);

  Krb5Principal getClientPrincipal();
  /**
   * Get lifetime of the currently loaded creds.
   */
  std::pair<uint64_t, uint64_t> getLifetime();

 protected:
  static const int SERVICE_HISTOGRAM_NUM_BUCKETS;
  static const int SERVICE_HISTOGRAM_PERIOD;

  /**
   * Stores data about a service: how often it's accessed,
   * the associated credentials cache.
   */
  class ServiceData {
   public:
     ServiceData();
     void bumpCount();
     uint64_t getCount();

     Lock lock;
     folly::BucketedTimeSeries<uint64_t> timeSeries;
     // Credentials cache for the service
     std::shared_ptr<Krb5CCache> cache;
  };

  /**
   * Given a service, fill an empty cache with krbtgts, making it possible
   * to later fetch the service cred from the kdc.
   */
  std::unique_ptr<Krb5CCache> initCacheForService(
    const Krb5Principal& service,
    const krb5_creds* creds = nullptr,
    SecurityLogger* logger = nullptr);

  std::shared_ptr<ServiceData> getServiceDataPtr(const Krb5Principal& service);
  std::vector<Krb5Principal> getServicePrincipalList();

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
  Lock serviceDataMapLock_;
  DataMapType serviceDataMap_;

  /**
   * Storage for krbtgt credentials
   */
  Krb5Tgts tgts_;
  std::shared_ptr<SecurityLogger> logger_;
};

}}}

#endif
