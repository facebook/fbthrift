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

#include <thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h>

#include <glog/logging.h>
#include <memory>
#include <set>
#include <stdio.h>

#include <folly/stats/BucketedTimeSeries-defs.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>
#include <folly/String.h>

namespace apache { namespace thrift { namespace krb5 {
using namespace std;

const int Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD = 60*1000;
const int Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD = 600;
const int Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE = 10000;

Krb5CredentialsCacheManager::Krb5CredentialsCacheManager(
  const std::shared_ptr<SecurityLogger>& logger)
    : stopManageThread_(false)
    , logger_(logger)
    , ccacheTypeIsMemory_(false) {

  try {
    // These calls can throw if the context cannot be initialized for some
    // reason, e.g. bad config format, etc.
    ctx_ = folly::make_unique<Krb5Context>();
    store_ = folly::make_unique<Krb5CCacheStore>(logger);
  } catch (const std::runtime_error& e) {
    // Caught exception while trying to initialize the context / store.
    // The ccache manager thread will detect this and attempt to initialize them
    // again. Don't do anything now.
  }

  // Client principal choice: principal
  // in current ccache, or first principal in keytab.  It's possible
  // for none to be available at startup.  In that case, we just keep
  // trying until a ccache or keytab appears.  Once a client principal
  // is chosen, we keep using that one and it never changes.

  manageThread_ = std::thread([=] {
    logger->log("manager_started");
    while(true) {
      MutexGuard l(manageThreadMutex_);
      if (stopManageThread_) {
        break;
      }

      // Catch all the exceptions. This thread should never die.
      try {
        // If the context or store are not initialized, try to initialize them.
        if (!ctx_) {
          ctx_ = folly::make_unique<Krb5Context>();
        }
        if (!store_) {
          store_ = folly::make_unique<Krb5CCacheStore>(logger);
        }

        // Reinit or init the cache store if it expired or has never been
        // initialized
        if (!store_->isInitialized()) {
          logger->logStart("init_cache_store");
          initCacheStore();
          logger->logEnd("init_cache_store");
        } else if (aboutToExpire(store_->getLifetime())) {
          logger->logStart("init_cache_store_after_expiration");
          initCacheStore();
          logger->logEnd("init_cache_store_after_expiration");
        }

        // If the cache store needs to be renewed, renew it
        auto lifetime = store_->getLifetime();
        bool reached_renew_time = reachedRenewTime(
          lifetime, folly::to<string>(store_->getClientPrincipal()));
        if (reached_renew_time) {
          logger->logStart("build_renewed_cache");
          store_->renewCreds();
          logger->logEnd("build_renewed_cache");
        }

        // Persist cache store to a file
        logger->logStart("persist_ccache");
        writeOutCache(
          Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE);
        logger->logEnd("persist_ccache");
      } catch (const std::runtime_error& e) {
        // Notify the waitForCache functions that an error happened.
        // We should propagate this error up.
        logger->log("cc_manager_thread_error", e.what());
        const string credentialCacheErrorMessage =
                     ". If the application is authenticating as a user," \
                     " run \"kinit\" to get new kerberos tickets. If the" \
                     " application is authenticating as a service identity," \
                     " make sure the keytab is in the right place"
                     " and is accessible. Check for bad format in kerberos" \
                     " config too.";
        if (store_) {
          store_->notifyOfError(e.what() + credentialCacheErrorMessage);
        }
        static string oldError = "";
        if (oldError != e.what()) {
          oldError = e.what();
          LOG(ERROR) << "Failure in credential cache thread: " << e.what()
                     << credentialCacheErrorMessage;
        }
      }

      if (!stopManageThread_) {
        int wait_time =
          Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD;
        if (store_ && !store_->isInitialized()) {
          // Shorten loop time to 1 second if first iteration didn't initialize
          // the client successfully
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

std::unique_ptr<Krb5CCache> Krb5CredentialsCacheManager::readInCache() {
  Krb5CCache file_cache = Krb5CCache::makeDefault();
  Krb5Principal client = file_cache.getClientPrincipal();

  // If a client principal has already been set, make sure the ccache
  // matches.  If it doesn't, bail out. This is so we make sure that
  // we don't accidentally read in a ccache for a different client.
  if (store_->isInitialized() && store_->getClientPrincipal() != client) {
    throw std::runtime_error(
      folly::to<string>("ccache contains client principal ", client,
                        " but ", store_->getClientPrincipal(),
                        " was required"));
  }

  auto mem = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique("MEMORY"));
  mem->setDestroyOnClose();
  mem->initialize(client.get());
  // Copy the file cache into memory
  krb5_error_code code = krb5_cc_copy_creds(
    ctx_->get(), file_cache.get(), mem->get());
  raiseIf(code, "copying to memory cache");

  return mem;
}

void Krb5CredentialsCacheManager::writeOutCache(size_t limit) {
  if (ccacheTypeIsMemory_) {
    // Don't write to file if type is MEMORY
    return;
  }

  Krb5CCache default_cache = Krb5CCache::makeDefault();

  // Get the default name
  folly::StringPiece default_type, default_name;
  string default_cache_name = default_cache.getName();
  folly::split<false>(":", default_cache_name, default_type, default_name);
  if (default_type == "MEMORY") {
    LOG(INFO) << "Default cache is of type MEMORY and will not be persisted";
    ccacheTypeIsMemory_ = true;
    return;
  }
  if (default_type != "FILE") {
    LOG(ERROR) << "Default cache is not of type FILE, the type is: " +
      default_type.str();
    logger_->log("persist_ccache_fail_default_name_invalid");
    return;
  }
  Krb5Principal client_principal = store_->getClientPrincipal();

  // Check if client matches.
  try {
    Krb5Principal def_princ = default_cache.getClientPrincipal();
    // We still want to overwrite caches that are about to expire.
    bool about_to_expire = aboutToExpire(default_cache.getLifetime());
    if (!about_to_expire && client_principal != def_princ) {
      VLOG(4) << "File cache principal does not match client, not overwriting";
      logger_->log("persist_ccache_fail_client_mismatch");
      return;
    }
  } catch (...) {
    VLOG(4) << "Error happend reading the default credentials cache";
  }

  // If the manager can't renew credentials, it should not be modifying the
  // file cache.
  bool can_renew = false;
  try {
    can_renew = isPrincipalInKeytab(client_principal);
  } catch (...) {
    VLOG(4) << "Error reading from keytab";
  }
  if (!can_renew) {
    VLOG(4) << "CC manager can't renew creds, won't overwrite ccache";
    logger_->log("persist_ccache_fail_keytab_mismatch");
    return;
  }

  // Create a temporary file in the same directory as the default cache
  std::vector<folly::StringPiece> path_tokens;
  folly::split("/", default_name, path_tokens);
  std::string tmp_template;
  folly::join(
    "/",
    path_tokens.begin(),
    path_tokens.begin() + path_tokens.size() - 1,
    tmp_template);
  tmp_template += "/tmpcache_XXXXXX";

  char str_buf[4096];
  int ret = snprintf(str_buf, 4096, "%s", tmp_template.c_str());
  if (ret < 0 || ret >= 4096) {
    LOG(ERROR) << "Temp file template name too long: " << tmp_template;
    logger_->log("persist_ccache_fail_tmp_file_too_long", tmp_template);
    return;
  }

  int fd = mkstemp(str_buf);
  if (fd == -1) {
    LOG(ERROR) << "Could not open a temporary cache file with template: "
               << tmp_template << " error: " << strerror(errno);
    logger_->log("persist_ccache_fail_tmp_file_create_fail", tmp_template);
    return;
  }
  ret = close(fd);
  if (ret == -1) {
    LOG(ERROR) << "Failed to close file: "
               << str_buf << " error: " << strerror(errno);
    // Don't return. Not closing the file is still OK. At worst, it's a
    // fd leak.
  }

  std::unique_ptr<Krb5CCache> temp_cache = store_->exportCache(limit);
  temp_cache->setDestroyOnClose();

  // Move the in-memory temp_cache to a temporary file
  auto file_cache = Krb5CCache::makeResolve(str_buf);
  file_cache.initialize(client_principal.get());
  krb5_error_code code = krb5_cc_copy_creds(
    ctx_->get(), temp_cache->get(), file_cache.get());
  raiseIf(code, "copying to file cache");

  folly::StringPiece tmp_type, tmp_name;
  string file_cache_name = file_cache.getName();
  folly::split<false>(":", file_cache_name, tmp_type, tmp_name);
  if (tmp_type != "FILE") {
    LOG(ERROR) << "Tmp cache is not of type FILE, the type is: " +
      tmp_type.str();
    logger_->log("persist_ccache_fail_file_not_of_type_file");
    return;
  }
  int result = rename(tmp_name.str().c_str(), default_name.str().c_str());
  if (result != 0) {
    logger_->log("persist_ccache_fail_rename", strerror(errno));
    LOG(ERROR) << "Failed modifying the default credentials cache: "
               << strerror(errno);
    // Attempt to delete tmp file. We shouldn't leave it around. If delete
    // fails for some reason, not much we can do.
    ret = unlink(tmp_name.str().c_str());
    if (ret == -1) {
      LOG(ERROR) << "Error deleting temp file: " << tmp_name.str().c_str()
                 << " because of error: " << strerror(errno);
    }
    return;
  }
  logger_->log("persist_ccache_success");
}

std::shared_ptr<Krb5CCache> Krb5CredentialsCacheManager::waitForCache(
    const Krb5Principal& service,
    SecurityLogger* logger) {
  if (!store_) {
    throw std::runtime_error("Kerberos ccache store could not be initialized");
  }
  return store_->waitForCache(service, logger);
}

void Krb5CredentialsCacheManager::initCacheStore() {
  // Read the file cache. Note that file caches for the incorrect
  // client will not be read in.
  std::unique_ptr<Krb5CCache> file_cache;
  string err_string;
  logger_->logStart("read_in_cache_attempt");
  try {
    file_cache = readInCache();
  } catch (const std::runtime_error& e) {
    VLOG(4) << "Failed to read file cache: " << e.what();
    err_string += (string(e.what()) + ". ");
  }
  logger_->logEnd("read_in_cache_attempt");

  // If the file cache is usable (ie. not expired), then just import it
  if (file_cache && !aboutToExpire(file_cache->getLifetime())) {
    logger_->logStart("import_ccache");
    store_->importCache(*file_cache);
    logger_->logEnd("import_ccache");
    return;
  } else if (file_cache) {
    err_string += "File cache about to expire";
  }

  // If file cache is not present or expired, we want to do a kinit
  auto first_principal = getFirstPrincipalInKeytab();
  if (first_principal) {
    logger_->logStart("kinit_ccache");
    store_->kInit(*first_principal);
    logger_->logEnd("kinit_ccache");
  } else {
    throw std::runtime_error(
      "The credentials cache and keytab are both unavailable: "
      + err_string);
  }

  // If file cache is present but expired, let's just renew the
  // old creds.
  if (file_cache) {
    vector<Krb5Principal> princ_list = file_cache->getServicePrincipalList();
    logger_->logStart("renew_expired_princ");
    for (const auto& princ : princ_list) {
      store_->waitForCache(princ);
    }
    logger_->logEnd("renew_expired_princ");
  }
}

unique_ptr<Krb5Principal>
    Krb5CredentialsCacheManager::getFirstPrincipalInKeytab() {
  Krb5Keytab keytab(ctx_->get());
  // No client has been chosen.  Try the first principal in the
  // keytab.
  for (auto& ktentry : keytab) {
    return folly::make_unique<Krb5Principal>(
      ctx_->get(), std::move(ktentry.principal));
  }
  return nullptr;
}

bool Krb5CredentialsCacheManager::isPrincipalInKeytab(
    const Krb5Principal& princ) {
  Krb5Keytab keytab(ctx_->get());
  for (auto& ktentry : keytab) {
    if (princ == Krb5Principal(
        ctx_->get(), std::move(ktentry.principal))) {
      return true;
    }
  }
  return false;
}

bool Krb5CredentialsCacheManager::aboutToExpire(
    const std::pair<uint64_t, uint64_t>& lifetime) {
  time_t now;
  time(&now);
  return ((uint64_t) now +
    Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD) >
      lifetime.second;
}

bool Krb5CredentialsCacheManager::reachedRenewTime(
    const std::pair<uint64_t, uint64_t>& lifetime, const std::string& client) {
  time_t now;
  time(&now);
  size_t sname_hash = std::hash<std::string>()(client);

  uint64_t start = lifetime.first;
  uint64_t end = lifetime.second;
  // Set the renew interval to be about 25% of lifetime
  uint64_t quarter_life_time = (end - start) / 4;
  uint64_t renew_offset = sname_hash % quarter_life_time;
  uint64_t half_life_time = (start + (end - start) / 2);

  return (uint64_t) now > (half_life_time + renew_offset);
}

}}}
