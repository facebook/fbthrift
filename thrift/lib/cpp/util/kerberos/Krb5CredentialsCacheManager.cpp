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
#include "thrift/lib/cpp2/security/SecurityLogger.h"

#include <glog/logging.h>
#include <memory>
#include <set>
#include <stdio.h>

#include "folly/stats/BucketedTimeSeries-defs.h"
#include "folly/Memory.h"
#include "folly/ScopeGuard.h"
#include "folly/String.h"
#include "folly/ThreadName.h"


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
    const string& client,
    const std::shared_ptr<SecurityLogger>& logger)
  : stopManageThread_(false)
  , logger_(logger) {

  // Client principal choice: first of explicitly specified, principal
  // in current ccache, or first principal in keytab.  It's possible
  // for none to be available at startup.  In that case, we just keep
  // trying until a ccache or keytab appears.  Once a client principal
  // is chosen, we keep using that one and it never changes.

  if (!client.empty()) {
    // If the caller specified a client, just use it.
    client_ = folly::make_unique<Krb5Principal>(ctx_.get(), client);
  }

  manageThread_ = std::thread([=] {
    folly::setThreadName("krb5-cache");
    logger->log("manager_started", client);
    size_t sname_hash = 0;

    if (client_) {
      sname_hash = std::hash<std::string>()(folly::to<string>(*client_));
    }

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
            logger->log("init_via_file");
            VLOG(4) << "Initialized the krb5 credentials cache from file";
          } catch(...) {
            // Failed reading in file cache, probably means it's not there.
            // Just get a new cache.
            mem = kInit();
            logger->log("init_via_kinit");
            VLOG(4) << "Initialized the krb5 credentials cache via kinit";
          }
        }

        // If we got a client from the cache, this will use it.
        // Otherwise, it will use the principal chosen by kInit.
        if (!client_) {
          client_ = folly::make_unique<Krb5Principal>(
            std::move(mem->getClientPrincipal()));
          sname_hash = std::hash<std::string>()(folly::to<string>(*client_));
          logger->log("get_client_from_ccache", folly::to<string>(*client_));
        }

        auto lifetime = mem->getLifetime();
        time_t now;
        time(&now);

        // Set the renew interval to be about 25% of lifetime
        uint64_t quarter_life_time = (lifetime.second - lifetime.first) / 4;
        uint64_t renew_offset = sname_hash % quarter_life_time;
        uint64_t half_life_time =
          (lifetime.first + (lifetime.second - lifetime.first) / 2);

        bool reached_renew_time = (uint64_t) now >
          (half_life_time + renew_offset);
        // about_to_expire is true if the cache will expire in 5 minutes,
        // or has already expired
        bool about_to_expire = ((uint64_t) now +
          Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD) >
            lifetime.second;

        if (about_to_expire || lifetime.first == 0) {
          logger->log("about_to_expire");
          mem = kInit();
          importMemoryCache(mem);
          VLOG(4) << "do kInit because CC is about to expire";
        } else if (reached_renew_time) {
          // If we've reached half-life, but not about to expire, it means
          // the cache we just got is still valid, so we can use it while
          // we update the old one.
          if (getCache() == nullptr) {
            importMemoryCache(mem);
          }
          logger->logStart("build_renewed_cache");
          mem = buildRenewedCache();
          logger->logEnd("build_renewed_cache");
          importMemoryCache(mem);
          VLOG(4) << "renewed CC at half-life";
        }

        // If we still haven't persisted here, persist the cache to memory
        if (getCache() == nullptr) {
          importMemoryCache(mem);
        }

        // Persist cache to file
        writeOutCache(
          Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE);
      } catch (const std::runtime_error& e) {
        // Notify the waitForCache functions that an error happened.
        // We should propagate this error up.
        logger->log("cc_manager_thread_error", e.what());
        notifyOfError(e.what());
        static string oldError = "";
        if (oldError != e.what()) {
          oldError = e.what();
          LOG(ERROR) << "Failure in credential cache thread: " << e.what()
                     << " If the application is authenticating as a user,"
                     << " run \"kinit\" to get new kerberos tickets. If the"
                     << " application is authenticating as a service identity,"
                     << " make sure the keytab is in the right place"
                     << " and is accessible";
        }
      }

      if (!stopManageThread_) {
        int wait_time =
          Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD;
        if (getCache() == nullptr) {
          // Shorten loop time to 1 second if first iteration didn't initialize
          // the cache successfully
          wait_time = 1000;
        }
        manageThreadCondVar_.wait_for(l, std::chrono::milliseconds(wait_time));
      }
    }
  });
}

Krb5CredentialsCacheManager::~Krb5CredentialsCacheManager() {
  stopThread();
  logger_->log("manager_destroyed");
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
  // Get default keytab
  Krb5Keytab keytab(ctx_.get());

  unique_ptr<Krb5Principal> first_service;
  Krb5Principal* client = nullptr;

  if (client_) {
    // If a client has already been set, confirm we have a matching
    // keytab entry.  This avoids changing clients, and querying the
    // KDC unnecessarily if the client isn't in the keytab.
    bool client_ok = false;
    for (auto& ktentry : keytab) {
      if (*client_ == Krb5Principal(ctx_.get(), std::move(ktentry.principal))) {
        client_ok = true;
        break;
      }
    }
    if (!client_ok) {
      throw std::runtime_error(
        folly::to<string>("client principal ", *client_,
                          " is not available in keytab ", keytab.getName()));
    }
    client = client_.get();
  } else {
    // No client has been chosen.  Try the first principal in the
    // keytab.
    for (auto& ktentry : keytab) {
      first_service = folly::make_unique<Krb5Principal>(
        ctx_.get(), std::move(ktentry.principal));
      break;
    }
    client = first_service.get();
    if (!client) {
      throw std::runtime_error(
        folly::to<string>("Keytab ", keytab.getName(), "has no services"));
    }
  }

  // Make a new memory cache.
  auto mem = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique(ctx_.get(), "MEMORY"));
  // Initialize the new CC
  krb5_error_code code = krb5_cc_initialize(
    ctx_.get(), mem->get(), client->get());
  raiseIf(code, "initializing memory ccache");

  // Get default init options
  Krb5InitCredsOpt options(ctx_.get());

  logger_->logStart("kinit_get_tgt", folly::to<string>(*client));
  // Grab the tgt
  krb5_creds creds;
  code = krb5_get_init_creds_keytab(
    ctx_.get(),
    &creds,
    client->get(),
    keytab.get(),
    0,
    nullptr,
    options.get());
  raiseIf(code, "Getting new tgt ticket using keytab " + keytab.getName());
  logger_->logEnd("kinit_get_tgt");

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
  in_creds.client = client_->get();
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
      logger_->logStart("do_tgs_request", folly::to<string>(server));
      doTgsReq(server.get(), *mem);
      logger_->logEnd("do_tgs_request");
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

  // If a client principal has already been set, make sure the ccache
  // matches.  If it doesn't, bail out.
  if (client_ && *client_ != client) {
    throw std::runtime_error(
      folly::to<string>("ccache contains client principal ", client,
                        " but ", *client_, " was required"));
  }

  // Copy the cache into memory
  krb5_error_code code = krb5_cc_initialize(
    ctx_.get(), mem->get(), client.get());
  raiseIf(code, "initializing memory ccache");
  code = krb5_cc_copy_creds(ctx_.get(), file_cache.get(), mem->get());
  raiseIf(code, "copying to memory cache");

  auto service_list = mem->getServicePrincipalList(false /* filter tgt */);
  for (auto& service : service_list)  {
    logger_->log("read_in_principal", folly::to<string>(service));
    if (!service.isTgt()) {
      incUsedService(folly::to<string>(service));
    }
  }

  return mem;
}

void Krb5CredentialsCacheManager::writeOutCache(size_t limit) {
  auto cc_mem = getCache();
  if (cc_mem == nullptr) {
    throw std::runtime_error("Trying to persist an empty cache");
  }

  // Make a new file cache.
  auto file_cache = Krb5CCache::makeNewUnique(ctx_.get(), "FILE");
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
      element.first, element.second->getCount()));
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
    logger_->log("write_out_principal", folly::to<string>(server));
    // Store the cred into a file
    code = krb5_cc_store_cred(ctx_.get(), file_cache.get(), &(*it));
    // Erase from top_services struct so we don't persist the same
    // principal more than once.
    top_services.erase(princ_string);
    raiseIf(code, "Failed storing a credential into a file");
  }

  // Now let's rename the tmp file.
  Krb5CCache default_cache = Krb5CCache::makeDefault(ctx_.get());
  folly::StringPiece default_type, default_name;
  string default_cache_name = default_cache.getName();
  folly::split<false>(":", default_cache_name, default_type, default_name);
  if (default_type != "FILE") {
    LOG(ERROR) << "Default cache is not of type FILE, the type is: " +
      default_type.str();
    return;
  }
  folly::StringPiece tmp_type, tmp_name;
  string file_cache_name = file_cache.getName();
  folly::split<false>(":", file_cache_name, tmp_type, tmp_name);
  if (tmp_type != "FILE") {
    LOG(ERROR) << "Tmp cache is not of type FILE, the type is: " +
      tmp_type.str();
    return;
  }
  int result = rename(tmp_name.str().c_str(), default_name.str().c_str());
  if (result != 0) {
    LOG(ERROR) << "Failed modifying the default credentials cache";
  }
}

void Krb5CredentialsCacheManager::importMemoryCache(
    std::shared_ptr<Krb5CCache> cache) {
  MutexGuard guard(ccLock_);
  ccMemory_ = cache;
  ccMemoryCondVar_.notify_all();
}

void Krb5CredentialsCacheManager::notifyOfError(const std::string& error) {
  MutexGuard guard(ccLock_);
  ccMemoryFetchError_ = error;
  ccMemoryCondVar_.notify_all();
}

std::shared_ptr<Krb5CCache> Krb5CredentialsCacheManager::waitForCache() {
  MutexGuard guard(ccLock_);
  // If the manager still hasn't fully initialized, we force this to block
  while (ccMemory_ == nullptr && ccMemoryFetchError_.empty()) {
    ccMemoryCondVar_.wait(guard);
  }
  // If the thread manager has been through the initialization thread loop
  // at least once, and ccMemory_ is still null, then throw an error. It means
  // there was some sort of failure and we need to report it up.
  if (ccMemory_ == nullptr) {
    throw std::runtime_error(ccMemoryFetchError_);
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
    found->second->bumpCount();
  } else {
    // Here let's upgrade the lock to a write lock and insert a new element
    // into the map.
    WriteLock writeLock(std::move(readLock));
    serviceCountMap_[service] = folly::make_unique<ServiceTimeSeries>();
    serviceCountMap_[service]->bumpCount();
  }
}

}}}
