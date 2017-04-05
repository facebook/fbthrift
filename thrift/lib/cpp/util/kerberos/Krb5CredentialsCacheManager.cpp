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
#include <sys/stat.h>

#include <folly/stats/BucketedTimeSeries-defs.h>
#include <folly/Memory.h>
#include <folly/String.h>
#include <folly/ThreadName.h>
#include <folly/portability/GFlags.h>

// DO NOT modify this flag from your application
DEFINE_string(
  thrift_cc_manager_kill_switch_file,
  "/var/thrift_security/disable_cc_manager",
  "A file, which when present, acts as a kill switch for and disables the cc "
  " manager thread running on the host.");
DEFINE_bool(thrift_cc_manager_renew_user_creds,
            false,
            "If true will try to renew *@REALM and */admin@REALM creds");

// Time in seconds after which cc manager kill switch expires
static const time_t kCcManagerKillSwitchExpired = 86400;

// Don't log more than once about cc manager kill switch in this time (seconds)
static const time_t kCcManagerKillSwitchLoggingTimeout = 300;

static const std::string kCcThreadName = "Krb5CcManager";

namespace apache { namespace thrift { namespace krb5 {
using namespace std;

const int Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD = 10*60*1000;
const int Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD = 600;
const int Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE = 10000;
const int Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_LOG = 10;

Krb5CredentialsCacheManager::Krb5CredentialsCacheManager(
    const std::shared_ptr<Krb5CredentialsCacheManagerLogger>& logger,
    int maxCacheSize)
    : stopManageThread_(false),
      logger_(logger),
      ccacheTypeIsMemory_(false),
      updateFileCacheEnabled_(true) {
  try {
    // These calls can throw if the context cannot be initialized for some
    // reason, e.g. bad config format, etc.
    ctx_ = std::make_unique<Krb5Context>();
    store_ = std::make_unique<Krb5CCacheStore>(logger, maxCacheSize);
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
    folly::setThreadName(kCcThreadName);
    logger->log("manager_started");
    logger->log("max_cache_size", folly::to<string>(maxCacheSize));

    string oldError = "";
    while (true) {
      MutexGuard l(manageThreadMutex_);
      if (stopManageThread_) {
        break;
      }

      // If ccache manager kill switch is enabled, then sleep for a second
      // and continue
      if (fetchIsKillSwitchEnabled()) {
        // Log only once in kCcManagerKillSwitchLoggingTimeout seconds if the
        // kill switch is active
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - lastLoggedKillSwitch_);
        if (elapsed.count() >= kCcManagerKillSwitchLoggingTimeout) {
          logger->log("manager_kill_switch_enabled");
          lastLoggedKillSwitch_ = now;
        }
        manageThreadCondVar_.wait_for(l, std::chrono::seconds(1));
        continue;
      }

      // Catch all the exceptions. This thread should never die.
      try {
        // If the context or store are not initialized, try to initialize them.
        if (!ctx_) {
          ctx_ = std::make_unique<Krb5Context>();
        }
        if (!store_) {
          store_ = std::make_unique<Krb5CCacheStore>(
            logger, maxCacheSize);
        }

        // Reinit or init the cache store if it expired or has never been
        // initialized
        if (!store_->isInitialized()) {
          logger->logStart("init_cache_store");
          initCacheStore();
          logger->logEnd("init_cache_store");
          logTopCredentials(logger, "init_cache_store");
        } else if (aboutToExpire(store_->getLifetime())) {
          logger->logStart("init_cache_store", "expired");
          initCacheStore();
          logger->logEnd("init_cache_store");
          logTopCredentials(logger, "init_expired_cache_store");
        }

        // If not a user credential and the cache store needs to be renewed,
        // renew it
        Krb5Principal clientPrinc = store_->getClientPrincipal();
        if (!clientPrinc.isUser() || FLAGS_thrift_cc_manager_renew_user_creds) {
          auto lifetime = store_->getLifetime();
          bool reached_renew_time = reachedRenewTime(
            lifetime, folly::to<string>(clientPrinc));
          if (reached_renew_time) {
            logger->logStart("build_renewed_cache");
            uint64_t renewCount = store_->renewCreds();
            logger->logEnd(
              "build_renewed_cache", folly::to<std::string>(renewCount));
            logTopCredentials(logger, "build_renewed_cache");
          }

          if (updateFileCacheEnabled_) {
            // Persist cache store to a file
            logger->logStart("persist_ccache");
            int outSize = writeOutCache(
              Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_PERSIST_TO_FILE);
            logger->logEnd(
              "persist_ccache", folly::to<std::string>(outSize));
          }
        }
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
        if (oldError != e.what()) {
          oldError = e.what();
          LOG(ERROR) << "Failure in credential cache thread: " << e.what()
                     << credentialCacheErrorMessage;
        }
      }

      auto waitTime =
        std::chrono::milliseconds(
            Krb5CredentialsCacheManager::MANAGE_THREAD_SLEEP_PERIOD);
      if (store_ && !store_->isInitialized()) {
        // Shorten loop time to 1 second if first iteration didn't initialize
        // the client successfully
        waitTime = std::chrono::milliseconds(1000);
      }
      manageThreadCondVar_.wait_for(l, waitTime);
    }
  });
}

Krb5CredentialsCacheManager::~Krb5CredentialsCacheManager() {
  MutexGuard l(manageThreadMutex_);
  stopManageThread_ = true;
  l.unlock();
  manageThreadCondVar_.notify_one();
  manageThread_.join();
  logger_->log("manager_destroyed");
}

bool Krb5CredentialsCacheManager::fetchIsKillSwitchEnabled() {
  struct stat info;
  return (stat(FLAGS_thrift_cc_manager_kill_switch_file.c_str(), &info) == 0 &&
          time(nullptr) - info.st_mtime < kCcManagerKillSwitchExpired);
}

bool Krb5CredentialsCacheManager::waitUntilCacheStoreInitialized(
    std::chrono::milliseconds timeoutMS) {
  bool initialized = false;
  Krb5CCacheStore* store = nullptr;
  auto start = std::chrono::high_resolution_clock::now();
  while (!initialized) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - start);
    if (elapsed > timeoutMS) {
      return false;
    }
    if (!store) {
      MutexGuard l(manageThreadMutex_);
      store = store_.get();
    }
    initialized = store && store->isInitialized();
    sched_yield();
  }
  return true;
}

std::unique_ptr<Krb5CCache> Krb5CredentialsCacheManager::readInCache() {
  auto default_cache =
      std::make_unique<Krb5CCache>(Krb5CCache::makeDefault());
  std::unique_ptr<Krb5CCache> file_cache;
  std::unique_ptr<Krb5Principal> client;

  std::string default_type, default_path;
  default_cache->getCacheTypeAndName(default_type, default_path);

  if (default_type != "FILE" && default_type != "DIR") {
    throw std::runtime_error(
        folly::to<string>("ccache is if a type ", default_type,
                          " but expect DIR or FILE"));

  }
  if (store_->isInitialized()) {
    client = std::make_unique<Krb5Principal>(store_->getClientPrincipal());
  }

  if (default_type == "FILE") {
    // If a client principal has already been set, make sure the ccache
    // matches.  If it doesn't, bail out. This is so we make sure that
    // we don't accidentally read in a ccache for a different client.
    if (client && default_cache->getClientPrincipal() != *client) {
      throw std::runtime_error(
        folly::to<string>("ccache contains client principal ",
                          default_cache->getClientPrincipal(),
                          " but ", *client, " was required"));
    }
    file_cache.swap(default_cache);
    client =
        std::make_unique<Krb5Principal>(file_cache->getClientPrincipal());
  }
  else if (default_type == "DIR") {
    if(default_path[0] == ':') {
      default_path.erase(0, 1);
    }
    else { // Should not ever happen
      throw std::runtime_error(
          folly::to<string>("Wrong DIR ccache path: ", default_path));
    }
    if (!client) {
      // If client is not set, use first principal from keytab
      client = getFirstPrincipalInKeytab();
    }

    // Get default ccache dir.
    std::string default_dir;
    std::vector<folly::StringPiece> path_tokens;
    folly::split("/", default_path, path_tokens);
    folly::join(
      "/",
      path_tokens.begin(),
      path_tokens.begin() + path_tokens.size() - 1,
      default_dir);
    std::string client_princ_str = folly::to<string>(*client);
    client_princ_str.erase(std::remove(client_princ_str.begin(),
                                       client_princ_str.end(),
                                       '/'),
                           client_princ_str.end());
    const std::string ccache_path =
        folly::to<string>(default_dir, "/tkt_", client_princ_str);

    file_cache = std::make_unique<Krb5CCache>(Krb5CCache::makeResolve(
          folly::to<string>("DIR::", ccache_path)));
    if (*client != file_cache->getClientPrincipal()) {
      throw std::runtime_error(
        folly::to<string>("ccache contains client principal ",
                          file_cache->getClientPrincipal(),
                          " but ", *client, " was required"));
    }
  }
  auto mem = std::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique("MEMORY"));
  mem->setDestroyOnClose();
  mem->initialize(client->get());
  // Copy the file cache into memory
  const krb5_error_code code = krb5_cc_copy_creds(
    ctx_->get(), file_cache->get(), mem->get());
  raiseIf(code, "copying to memory cache");

  return mem;
}

int Krb5CredentialsCacheManager::writeOutCache(size_t limit) {
  if (ccacheTypeIsMemory_) {
    // Don't write to file if type is MEMORY
    return -1;
  }

  auto default_cache =
      std::make_unique<Krb5CCache>(Krb5CCache::makeDefault());
  std::string default_type, default_path;
  default_cache->getCacheTypeAndName(default_type, default_path);
  if (default_type == "MEMORY") {
    LOG(INFO) << "Default cache is of type MEMORY and will not be persisted";
    ccacheTypeIsMemory_ = true;
    logger_->log("persist_ccache_fail_memory_type");
    return -1;
  }
  if (default_type != "FILE" && default_type != "DIR") {
    LOG(ERROR) << "Default cache is not of type FILE or DIR, the type is: "
               << default_type;
    logger_->log("persist_ccache_fail_default_name_invalid");
    return -1;
  }

  if (default_type == "DIR") {
    if (default_path[0] == ':') {
      default_path.erase(0, 1);
    }
    else { // Should not ever happen
      throw std::runtime_error(
          folly::to<string>("Wrong DIR ccache path: ", default_path));
    }
  }

  std::unique_ptr<Krb5CCache> file_cache;
  std::string file_path;
  // Get default ccache dir.
  std::string default_dir;
  std::vector<folly::StringPiece> path_tokens;
  folly::split("/", default_path, path_tokens);
  folly::join(
    "/",
    path_tokens.begin(),
    path_tokens.begin() + path_tokens.size() - 1,
    default_dir);
  Krb5Principal client = store_->getClientPrincipal();
  if (default_type == "FILE") {
    file_path = default_path;
    file_cache.swap(default_cache);
  }
  else if (default_type == "DIR") {
    std::string client_princ_str = folly::to<string>(client);
    client_princ_str.erase(std::remove(client_princ_str.begin(),
                                       client_princ_str.end(),
                                       '/'),
                           client_princ_str.end());
    file_path =
        folly::to<string>(default_dir, "/tkt_", client_princ_str);
    file_cache = std::make_unique<Krb5CCache>(Krb5CCache::makeResolve(
        folly::to<string>("DIR::", file_path)));
  }
  // Check if client matches.
  try {
    Krb5Principal file_princ = file_cache->getClientPrincipal();
    // We still want to overwrite caches that are about to expire.
    const bool about_to_expire = aboutToExpire(file_cache->getLifetime());
    if (!about_to_expire && client != file_princ) {
      VLOG(4) << "File cache principal does not match client, not overwriting";
      logger_->log("persist_ccache_fail_client_mismatch");
      return -1;
    }
  } catch (...) {
    VLOG(4) << "Error happend reading the default credentials cache";
  }

  // If the manager can't renew credentials, it should not be modifying the
  // file cache.
  bool can_renew = false;
  try {
    can_renew = isPrincipalInKeytab(client);
  } catch (...) {
    VLOG(4) << "Error reading from keytab";
  }
  if (!can_renew) {
    VLOG(4) << "CC manager can't renew creds, won't overwrite ccache";
    logger_->log("persist_ccache_fail_keytab_mismatch");
    return -1;
  }

  // Create a temporary file in the same directory as the default cache
  const std::string tmp_template =
      folly::to<string>(default_dir, "/tmpcache_XXXXXX");

  char str_buf[4096];
  int ret = snprintf(str_buf, 4096, "%s", tmp_template.c_str());
  if (ret < 0 || ret >= 4096) {
    LOG(ERROR) << "Temp file template name too long: " << tmp_template;
    logger_->log("persist_ccache_fail_tmp_file_too_long", tmp_template);
    return -1;
  }

  const int fd = mkstemp(str_buf);
  if (fd == -1) {
    LOG(ERROR) << "Could not open a temporary cache file with template: "
               << tmp_template << " error: " << strerror(errno);
    logger_->log(
        "persist_ccache_fail_tmp_file_create_fail",
        {tmp_template, strerror(errno)},
        Krb5CredentialsCacheManagerLogger::TracingOptions::NONE);
    return -1;
  }
  ret = close(fd);
  if (ret == -1) {
    LOG(ERROR) << "Failed to close file: "
               << str_buf << " error: " << strerror(errno);
    // Don't return. Not closing the file is still OK. At worst, it's a
    // fd leak.
  }

  std::unique_ptr<Krb5CCache> temp_cache = store_->exportCache(limit);
  // Count the elements:
  uint64_t persistCount = 0;
  for (auto it = temp_cache->begin(); it != temp_cache->end(); ++it) {
    persistCount++;
  }
  temp_cache->setDestroyOnClose();

  // Move the in-memory temp_cache to a temporary file
  auto tmp_file_cache = std::make_unique<Krb5CCache>(Krb5CCache::makeResolve(
      folly::to<string>("FILE:", str_buf)));
  tmp_file_cache->initialize(client.get());
  krb5_error_code code = krb5_cc_copy_creds(
    ctx_->get(), temp_cache->get(), tmp_file_cache->get());
  raiseIf(code, "copying to file cache");

  std::string tmp_type, tmp_path;
  tmp_file_cache->getCacheTypeAndName(tmp_type, tmp_path);
  if (tmp_path[0] == ':') {
    tmp_path.erase(0, 1);
  }
  if (tmp_type != "FILE" && tmp_type != "DIR") {
    LOG(ERROR) << "Tmp cache is not of type FILE or DIR, the type is: "
               << tmp_type;
    logger_->log("persist_ccache_fail_file_not_of_type_file");
    return -1;
  }
  const int result = rename(tmp_path.c_str(), file_path.c_str());
  if (result != 0) {
    logger_->log("persist_ccache_fail_rename", strerror(errno));
    LOG(ERROR) << "Failed modifying the default credentials cache: "
               << strerror(errno);
    // Attempt to delete tmp file. We shouldn't leave it around. If delete
    // fails for some reason, not much we can do.
    ret = unlink(tmp_path.c_str());
    if (ret == -1) {
      LOG(ERROR) << "Error deleting temp file: " << tmp_path.c_str()
                 << " because of error: " << strerror(errno);
    }
    return -1;
  }
  return persistCount;
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

  if (file_cache && !isPrincipalInKeytab(file_cache->getClientPrincipal())) {
    logger_->log("file cache principal is not in the key tab, disable renewal");
    updateFileCacheEnabled_ = false;
  }

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
    logger_->logStart(
      "kinit_ccache", folly::to<std::string>(*first_principal));
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
    uint64_t renewCount = 0;
    for (const auto& princ : princ_list) {
      store_->waitForCache(princ);
      renewCount++;
    }
    logger_->logEnd(
      "renew_expired_princ", folly::to<std::string>(renewCount));
  }

  if(!store_->isInitialized()) {
    throw std::runtime_error("Ccache store initialization failed: " +
        err_string);
  }
}

unique_ptr<Krb5Principal>
    Krb5CredentialsCacheManager::getFirstPrincipalInKeytab() {
  Krb5Keytab keytab(ctx_->get());
  // No client has been chosen.  Try the first principal in the
  // keytab.
  return keytab.getFirstPrincipalInKeytab();
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

bool Krb5CredentialsCacheManager::aboutToExpire(const Krb5Lifetime& lifetime) {
  time_t now;
  time(&now);
  return ((uint64_t) now +
    Krb5CredentialsCacheManager::ABOUT_TO_EXPIRE_THRESHOLD) >
      lifetime.second;
}

bool Krb5CredentialsCacheManager::reachedRenewTime(
    const Krb5Lifetime& lifetime,
    const std::string& client) {
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

void Krb5CredentialsCacheManager::logTopCredentials(
    const std::shared_ptr<Krb5CredentialsCacheManagerLogger>& logger,
    const std::string& key) {
  Krb5Keytab keytab(ctx_->get());
  logger->logCredentialsCache(
      key,
      keytab,
      store_->getClientPrincipal(),
      store_->getLifetime(),
      store_->getServicePrincipalLifetimes(
          Krb5CredentialsCacheManager::NUM_ELEMENTS_TO_LOG),
      store_->getTgtLifetimes());
}
}}}
