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

#include "thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h"

#include <memory>
#include <set>

#include "folly/stats/BucketedTimeSeries-defs.h"
#include "folly/Memory.h"
#include "folly/ScopeGuard.h"

namespace apache { namespace thrift { namespace krb5 {
using namespace std;

const int Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD = 60*1000;
const int Krb5CredentialsCacheManager::SERVICE_HISTOGRAM_NUM_BUCKETS = 10;
const int Krb5CredentialsCacheManager::SERVICE_HISTOGRAM_PERIOD = 600;
const int Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD = 300;
const int Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE = 10000;

bool serviceCountCompare (
    const pair<string, uint64_t>& i,
    const pair<string, uint64_t>& j) {
  return (i.second > j.second);
}

Krb5CredentialsCacheManager::ServiceTimeSeries::ServiceTimeSeries() :
  timeSeries_(
    Krb5CredentialsCacheManager::SERVICE_HISTOGRAM_NUM_BUCKETS,
    std::chrono::seconds(
      Krb5CredentialsCacheManager::SERVICE_HISTOGRAM_PERIOD)) {
}

void Krb5CredentialsCacheManager::ServiceTimeSeries::bumpCount() {
  WriteLock guard(&serviceTimeseriesLock_);
  time_t now = time(nullptr);
  timeSeries_.addValue(std::chrono::seconds(now), 1);
}

uint64_t Krb5CredentialsCacheManager::ServiceTimeSeries::getCount() {
  ReadLock guard(serviceTimeseriesLock_);
  // Note that we don't have a need to call timeSeries_.update(<time>)
  // here because we don't care to have the exact count at the current
  // time. We're ok with grabbing the count at the last update.
  return timeSeries_.count();
}

Krb5CredentialsCacheManager::Krb5CredentialsCacheManager(
    const string& client)
  : clientString_(client)
  , client_(ctx_.get(), client)
  , stopManageThread_(false) {

  if (client.empty()) {
    client_ = Krb5Principal::snameToPrincipal(ctx_.get(), KRB5_NT_UNKNOWN);
  }

  manageThread_ = std::thread([=] {
    while(true) {
      MutexGuard l(manageThreadMutex_);
      if (stopManageThread_) {
        break;
      }

      // Catch all the exceptions. This thread should never die.
      try {
        auto mem = getCache();
        if (mem == nullptr) {
          try {
            mem = readInCache();
            LOG(INFO) << "Initialized the krb5 credentials cache from file";
          } catch(...) {
            // Failed reading in file cache, probably means it's not there.
            // Just get a new cache.
            mem = kInit();
            LOG(INFO) << "Initialized the krb5 credentials cache via kinit";
          }
        }

        auto lifetime = mem->getLifetime();
        time_t now;
        time(&now);
        bool reached_half_life = (uint64_t) now >
          (lifetime.first + (lifetime.second - lifetime.first) / 2);
        // about_to_expire is true if the cache will expire in 5 minutes,
        // or has already expired
        bool about_to_expire = ((uint64_t) now +
          Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD) >
            lifetime.second;

        if (about_to_expire || lifetime.first == 0) {
          mem = kInit();
          importMemoryCache(mem);
          LOG(INFO) << "do kInit because CC is about to expire";
        } else if (reached_half_life) {
          // If we've reached half-life, but not about to expire, it means
          // the cache we just got is still valid, so we can use it while
          // we update the old one.
          if (getCache() == nullptr) {
            importMemoryCache(mem);
          }
          mem = buildRenewedCache();
          importMemoryCache(mem);
          LOG(INFO) << "renewed CC at half-life";
        }

        // If we still haven't persisted here, persist the cache to memory
        if (getCache() == nullptr) {
          importMemoryCache(mem);
        }

        // Persist cache to file
        writeOutCache(
          Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE);
      } catch (const std::runtime_error& e) {
        LOG(ERROR) << "Failure in credential cache thread: " << e.what();
      }

      if (!stopManageThread_) {
        manageThreadCondVar_.wait_for(l, std::chrono::milliseconds(
          Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD));
      }
    }
  });
}

Krb5CredentialsCacheManager::~Krb5CredentialsCacheManager() {
  stopThread();
}

void Krb5CredentialsCacheManager::stopThread() {
  // Kill the manage thread
  MutexGuard l(manageThreadMutex_);
  if (!stopManageThread_) {
    stopManageThread_ = true;
    manageThreadCondVar_.notify_one();
    l.unlock();
    manageThread_.join();
  }
}

std::unique_ptr<Krb5CCache> Krb5CredentialsCacheManager::kInit() {
  // Make a new memory cache.
  auto mem = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique(ctx_.get(), "MEMORY"));
  // Initialize the new CC
  krb5_error_code code = krb5_cc_initialize(
    ctx_.get(), mem->get(), client_.get());
  raiseIf(code, "initializing memory ccache");

  // Get default options and keytab objects
  Krb5InitCredsOpt options(ctx_.get());
  Krb5Keytab keytab(ctx_.get());

  // Grab the tgt
  krb5_creds creds;
  code = krb5_get_init_creds_keytab(
    ctx_.get(),
    &creds,
    client_.get(),
    keytab.get(),
    0,
    nullptr,
    options.get());
  raiseIf(code, "Getting new tgt ticket");
  SCOPE_EXIT { krb5_free_cred_contents(ctx_.get(), &creds); };

  code = krb5_cc_store_cred(ctx_.get(), mem->get(), &creds);
  raiseIf(code, "failed storing a credential into the new memory cache");

  return mem;
}

void Krb5CredentialsCacheManager::doTgsReq(
    krb5_principal server, Krb5CCache& cache) {
  // Make input credentials
  krb5_creds in_creds;
  memset(&in_creds, 0, sizeof(in_creds));
  in_creds.client = client_.get();
  in_creds.server = server;

  // Make options
  krb5_flags options;
  memset(&options, 0, sizeof(options));

  // To TGS request
  krb5_creds* out_creds;
  krb5_error_code code = krb5_get_credentials(ctx_.get(), options, cache.get(),
                                              &in_creds, &out_creds);
  raiseIf(code, "tgs request");
  // Drop the out credentials, we don't actually use these.
  krb5_free_cred_contents(ctx_.get(), out_creds);
}

std::unique_ptr<Krb5CCache> Krb5CredentialsCacheManager::buildRenewedCache() {

  // Read in the pointer to the current cache.
  auto cc_cur = getCache();
  if (cc_cur == nullptr) {
    throw std::runtime_error("Trying to refresh an empty cache");
  }

  // Get a new memory cache and initialize it.
  auto mem = kInit();

  for (auto& creds : *cc_cur) {
    Krb5Principal server(ctx_.get(), std::move(creds.server));
    if (!server.isTgt()) {
      doTgsReq(server.get(), *mem);
    }
  }

  return mem;
}

std::unique_ptr<Krb5CCache> Krb5CredentialsCacheManager::readInCache() {
  auto mem = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique(ctx_.get(), "MEMORY"));
  // Get the default local cache
  Krb5CCache file_cache = Krb5CCache::makeDefault(ctx_.get());
  Krb5Principal client = file_cache.getClientPrincipal();

  // Copy the cache into memory
  krb5_error_code code = krb5_cc_initialize(
    ctx_.get(), mem->get(), client.get());
  raiseIf(code, "initializing memory ccache");
  code = krb5_cc_copy_creds(ctx_.get(), file_cache.get(), mem->get());
  raiseIf(code, "copying to memory cache");

  auto service_list = mem->getServicePrincipalList();
  for (auto& service : service_list)  {
    incUsedService(folly::to<string>(service));
  }

  return mem;
}

void Krb5CredentialsCacheManager::writeOutCache(size_t limit) {
  auto cc_mem = getCache();
  if (cc_mem == nullptr) {
    throw std::runtime_error("Trying to persist an empty cache");
  }

  // Get a default file cache and empty it out
  Krb5CCache file_cache = Krb5CCache::makeDefault(ctx_.get());
  Krb5Principal client = cc_mem->getClientPrincipal();
  krb5_error_code code = krb5_cc_initialize(
    ctx_.get(), file_cache.get(), client.get());
  raiseIf(code, "Failed initializing file credentials cache");

  // Put 'limit' number of most frequently used credentials into the
  // top_services set.
  vector<pair<string, uint64_t>> count_vector;
  ReadLock readLock(&serviceCountLock_);
  for (auto& element : serviceCountMap_) {
    count_vector.push_back(pair<string, uint64_t>(
      element.first, element.second.getCount()));
  }
  readLock.reset();
  sort(count_vector.begin(), count_vector.end(), serviceCountCompare);

  std::set<string> top_services;
  int count = 0;
  for (auto& element : count_vector) {
    if (count >= limit) {
      break;
    }
    top_services.insert(element.first);
    count++;
  }

  // Iterate through the cc
  for (auto it = cc_mem->begin(true); it != cc_mem->end(); ++it) {
    krb5_principal borrowed_server = it->server;
    Krb5Principal server(ctx_.get(), std::move(borrowed_server));
    const string princ_string = folly::to<string>(server);
    SCOPE_EXIT { server.release(); };  // give back borrowed_server
    // Always persist config and tgt principals. And only persist
    // top 'limit' services.
    if (!it.isConfigEntry() && !server.isTgt() &&
        top_services.count(princ_string) == 0) {
      continue;
    }

    // Store the cred into a file
    code = krb5_cc_store_cred(ctx_.get(), file_cache.get(), &(*it));
    // Erase from top_services struct so we don't persist the same
    // principal more than once.
    top_services.erase(princ_string);
    raiseIf(code, "Failed storing a credential into a file");
  }
}

void Krb5CredentialsCacheManager::importMemoryCache(
    std::shared_ptr<Krb5CCache> cache) {
  MutexGuard guard(ccLock_);
  ccMemory_ = cache;
  ccMemoryCondVar_.notify_all();
}

std::shared_ptr<Krb5CCache> Krb5CredentialsCacheManager::waitForCache() {
  MutexGuard guard(ccLock_);
  while (ccMemory_ == nullptr) {
    ccMemoryCondVar_.wait(guard);
  }
  return ccMemory_;
}

std::shared_ptr<Krb5CCache> Krb5CredentialsCacheManager::getCache() {
  MutexGuard guard(ccLock_);
  return ccMemory_;
}

void Krb5CredentialsCacheManager::incUsedService(
    const string& service) {
  UpgradeLock readLock(&serviceCountLock_);
  auto found = serviceCountMap_.find(service);
  if (found != serviceCountMap_.end()) {
    // Here we already have the element, so we can just bump the count.
    found->second.bumpCount();
  } else {
    // Here let's upgrade the lock to a write lock and insert a new element
    // into the map.
    WriteLock writeLock(std::move(readLock));
    serviceCountMap_[service].bumpCount();
  }
}

}}}
