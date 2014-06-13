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

#include "thrift/lib/cpp/util/kerberos/Krb5CCacheStore.h"

#include <glog/logging.h>
#include <memory>
#include <set>
#include <stdio.h>

#include "folly/stats/BucketedTimeSeries-defs.h"
#include "folly/Memory.h"
#include "folly/ScopeGuard.h"
#include "folly/String.h"

// In older version of krb, this constant is not defined. Define it
#ifndef KRB5_GC_NO_STORE
#define KRB5_GC_NO_STORE     8  /**< Do not store in credential cache */
#endif

namespace apache { namespace thrift { namespace krb5 {
using namespace std;

const int Krb5CCacheStore::SERVICE_HISTOGRAM_NUM_BUCKETS = 10;
const int Krb5CCacheStore::SERVICE_HISTOGRAM_PERIOD = 600;

bool serviceCountCompare (
    const pair<string, uint64_t>& i,
    const pair<string, uint64_t>& j) {
  return (i.second > j.second);
}

Krb5CCacheStore::ServiceData::ServiceData() :
  timeSeries(
    Krb5CCacheStore::SERVICE_HISTOGRAM_NUM_BUCKETS,
    std::chrono::seconds(
      Krb5CCacheStore::SERVICE_HISTOGRAM_PERIOD)) {
}

void Krb5CCacheStore::ServiceData::bumpCount() {
  WriteLock guard(lock);
  time_t now = time(nullptr);
  timeSeries.addValue(std::chrono::seconds(now), 1);
}

uint64_t Krb5CCacheStore::ServiceData::getCount() {
  ReadLock guard(lock);
  // Note that we don't have a need to call timeSeries_.update(<time>)
  // here because we don't care to have the exact count at the current
  // time. We're ok with grabbing the count at the last update.
  return timeSeries.count();
}

std::shared_ptr<Krb5CCache> Krb5CCacheStore::waitForCache(
    const Krb5Principal& service,
    SecurityLogger* logger) {
  std::shared_ptr<ServiceData> dataPtr = getServiceDataPtr(service);

  // Bump that we've used the principal
  dataPtr->bumpCount();

  if (logger) {
    logger->logStart("get_prepared_cache", folly::to<string>(service));
  }

  {
    ReadLock readLock(dataPtr->lock);
    // If there is a cache, just return it. Try with a read lock first
    // for performance reasons.
    if (dataPtr->cache) {
      if (logger) {
        logger->logEnd("get_prepared_cache");
      }
      return dataPtr->cache;
    }
  }

  // If we didn't find a cache, upgrade to a write lock, and initialize
  // the cache.
  WriteLock writeLock(dataPtr->lock);
  if (dataPtr->cache) {
    if (logger) {
      logger->logEnd("get_prepared_cache");
    }
    return dataPtr->cache;
  }

  if (logger) {
    logger->logStart("init_cache_for_service", folly::to<string>(service));
  }
  dataPtr->cache = initCacheForService(service, nullptr, logger);
  if (logger) {
    logger->logEnd("init_cache_for_service");
  }
  return dataPtr->cache;
}

std::shared_ptr<Krb5CCacheStore::ServiceData>
    Krb5CCacheStore::getServiceDataPtr(const Krb5Principal& service) {
  string service_name = folly::to<string>(service);

  {
    ReadLock readLock(serviceDataMapLock_);
    auto found = serviceDataMap_.find(service_name);
    if (found != serviceDataMap_.end()) {
      // Get the element from the map if it exists
      return found->second;
    }
  }

  // Not found, we need to create it
  WriteLock writeLock(serviceDataMapLock_);
  if (!serviceDataMap_.count(service_name)) {
    serviceDataMap_[service_name] = std::make_shared<ServiceData>();
  }
  return serviceDataMap_[service_name];
}

std::unique_ptr<Krb5CCache> Krb5CCacheStore::initCacheForService(
    const Krb5Principal& service,
    const krb5_creds* creds,
    SecurityLogger* logger) {

  if (logger) {
    logger->logStart("wait_for_tgt");
  }
  std::shared_ptr<const Krb5Credentials> tgt;
  auto client_princ = tgts_.getClientPrincipal();
  // Get a cross-realm tgt if we need
  string srealm = service.getRealm();
  if (srealm != tgts_.getClientPrincipal().getRealm()) {
    tgt = tgts_.getTgtForRealm(srealm);
  } else {
    tgt = tgts_.getTgt();
  }
  if (logger) {
    logger->logEnd("wait_for_tgt");
  }

  if (logger) {
    logger->logStart("init_barebones_ccache");
  }
  // Make a new memory cache.
  auto mem = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique("MEMORY"));
  // Initialize the new CC
  mem->initialize(client_princ.get());
  mem->storeCred(tgt->get());
  if (logger) {
    logger->logEnd("init_barebones_ccache");
  }

  if (creds != nullptr) {
    mem->storeCred(*creds);
  } else {
    if (logger) {
      logger->logStart("get_credential_from_kdc");
    }
    mem->getCredentials(service.get());
    if (logger) {
      logger->logEnd("get_credential_from_kdc");
    }
  }

  return mem;
}

void Krb5CCacheStore::importCache(
    Krb5CCache& file_cache) {
  logger_->logStart("import_tgts");
  // Split out tgts and service principals
  std::vector<Krb5Credentials> tgts;
  std::vector<Krb5Credentials> services;
  for (auto& tmp_cred : file_cache) {
    Krb5Credentials cred(std::move(tmp_cred));
    Krb5Principal server = Krb5Principal::copyPrincipal(
      ctx_.get(), cred.get().server);
    if (server.isTgt()) {
      tgts.push_back(std::move(cred));
    } else {
      services.push_back(std::move(cred));
    }
  }

  // Import TGTs
  Krb5Tgts tgts_obj;
  Krb5Principal client_principal = file_cache.getClientPrincipal();
  tgts_obj.setClientPrincipal(client_principal);
  for (auto& tgt : tgts) {
    Krb5Principal server_principal = Krb5Principal::copyPrincipal(
      ctx_.get(), tgt.get().server);
    if (server_principal.getComponent(1) == client_principal.getRealm()) {
      tgts_obj.setTgt(folly::make_unique<Krb5Credentials>(std::move(tgt)));
    } else {
      tgts_obj.setTgtForRealm(
        server_principal.getComponent(1),
        folly::make_unique<Krb5Credentials>(std::move(tgt)));
    }
  }
  tgts_ = std::move(tgts_obj);
  logger_->logEnd("import_tgts");

  // Import service creds
  DataMapType new_data_map;
  logger_->logStart("import_service_creds");
  for (auto& service : services) {
    Krb5Principal princ = Krb5Principal::copyPrincipal(
      ctx_.get(), service.get().server);
    auto mem = initCacheForService(princ, &service.get());

    auto data = std::make_shared<ServiceData>();
    new_data_map[folly::to<string>(princ)] = data;
    data->cache = std::move(mem);
    data->bumpCount();
  }
  WriteLock service_data_lock(serviceDataMapLock_);
  serviceDataMap_ = std::move(new_data_map);
  logger_->logEnd("import_service_creds");
}

std::vector<Krb5Principal> Krb5CCacheStore::getServicePrincipalList() {
  std::vector<Krb5Principal> services;
  ReadLock lock(serviceDataMapLock_);
  for (auto& data : serviceDataMap_) {
    ReadLock lock(data.second->lock);
    if (!data.second->cache) {
      continue;
    }
    auto princ_list = data.second->cache->getServicePrincipalList();
    if (princ_list.size() < 1) {
      throw std::runtime_error("Principal list too small in ccache");
    }
    auto princ = std::move(princ_list[0]);
    services.push_back(std::move(princ));
  }
  return services;
}

void Krb5CCacheStore::renewCreds() {
  auto curTgt = tgts_.getTgt();
  auto client_princ = tgts_.getClientPrincipal();
  auto realms = tgts_.getValidRealms();

  // Renew TGTs
  Krb5Tgts tgts;
  tgts.kInit(client_princ);
  for (const auto& realm : realms) {
    tgts.getTgtForRealm(realm);
  }
  tgts_ = std::move(tgts);

  // Renew service creds, and store the renewed creds in a temporary map
  std::unordered_map<string, std::unique_ptr<Krb5CCache>> renewed_map;
  for (auto& service : getServicePrincipalList()) {
    try {
      auto mem = initCacheForService(service);
      renewed_map[folly::to<string>(service)] = std::move(mem);
    } catch (const std::runtime_error& e) {
      VLOG(4) << "Failed to renew cred for service: "
              << folly::to<string>(service) << " "
              << e.what();
    }
  }

  // Update the main service data map with the renewed creds.
  // If the creds failed to be renewed and the old ones are stale, then
  // just delete the old ones.
  WriteLock service_data_lock(serviceDataMapLock_);
  for (auto& entry : serviceDataMap_) {
    auto renewed_entry = renewed_map.find(entry.first);
    WriteLock lock(entry.second->lock);
    if (renewed_entry != renewed_map.end()) {
      entry.second->cache = std::move(renewed_entry->second);
    } else if (entry.second->cache) {
      // Not found, see if it's a new cred or old one
      auto lifetime = entry.second->cache->getLifetime();
      auto tgt_lifetime = tgts_.getLifetime();
      if (lifetime.second < tgt_lifetime.second) {
        // Delete old credential
        entry.second->cache.reset();
      }
    } // else
    // It's possible this thread wins the race to hit a service in the
    // data map that hasn't yet obtained a ccache. In this case, just leave
    // it alone. It will get the correct ccache.
  }
}

std::unique_ptr<Krb5CCache> Krb5CCacheStore::exportCache(size_t limit) {
  Krb5Principal client_principal = tgts_.getClientPrincipal();

  // Make a new memory cache.
  auto temp_cache = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique("MEMORY"));
  // Initialize the new CC
  temp_cache->initialize(client_principal.get());

  {
    // Put 'limit' number of most frequently used credentials into the
    // top_services set.
    ReadLock readLock(serviceDataMapLock_);
    vector<pair<string, uint64_t>> count_vector;
    for (auto& element : serviceDataMap_) {
      count_vector.emplace_back(element.first, element.second->getCount());
    }

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

    for (auto& data : serviceDataMap_) {
      ReadLock lock(data.second->lock);
      if (!data.second->cache) {
        continue;
      }
      auto princ_list = data.second->cache->getServicePrincipalList();
      if (princ_list.size() < 1) {
        throw std::runtime_error("Principal list too small in ccache");
      }
      auto princ = std::move(princ_list[0]);
      if (top_services.count(folly::to<string>(princ)) == 0) {
        continue;
      }
      Krb5Credentials service_cred = data.second->cache->retrieveCred(
        princ.get());
      temp_cache->storeCred(service_cred.get());
    }
  }

  // Store the TGTs in the cache.
  temp_cache->storeCred(tgts_.getTgt()->get());
  auto realms = tgts_.getValidRealms();
  for (const auto& realm : realms) {
    auto cred = tgts_.getTgtForRealm(realm);
    temp_cache->storeCred(cred->get());
  }
  return temp_cache;
}

bool Krb5CCacheStore::isInitialized() {
  return tgts_.isInitialized();
}

std::pair<uint64_t, uint64_t> Krb5CCacheStore::getLifetime() {
  return tgts_.getLifetime();
}

Krb5Principal Krb5CCacheStore::getClientPrincipal() {
  return tgts_.getClientPrincipal();
}

void Krb5CCacheStore::kInit(const Krb5Principal& client) {
  tgts_.kInit(client);
  WriteLock service_data_lock(serviceDataMapLock_);
  serviceDataMap_.clear();
}

void Krb5CCacheStore::notifyOfError(const std::string& error) {
  tgts_.notifyOfError(error);
}

}}}
