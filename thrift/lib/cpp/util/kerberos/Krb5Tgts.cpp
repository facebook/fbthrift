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

#include "thrift/lib/cpp/util/kerberos/Krb5Tgts.h"

#include <glog/logging.h>
#include <memory>
#include <set>
#include <stdio.h>

#include "folly/Memory.h"
#include "folly/ScopeGuard.h"
#include "folly/String.h"

// In older version of krb, this constant is not defined. Define it
#ifndef KRB5_GC_NO_STORE
#define KRB5_GC_NO_STORE     8  /**< Do not store in credential cache */
#endif

namespace apache { namespace thrift { namespace krb5 {
using namespace std;

Krb5Tgts& Krb5Tgts::operator=(Krb5Tgts&& tgts) {
  if (this == &tgts) {
    return *this;
  }

  string orig_error = tgts.initError_;
  // The thing we're moving shouldn't be waiting on anything
  tgts.notifyOfError("Moving Krb5Tgt object, error!");

  WriteLock lock(lock_);
  initError_ = orig_error;
  client_ = std::move(tgts.client_);
  tgt_ = std::move(tgts.tgt_);
  realmTgtsMap_ = std::move(tgts.realmTgtsMap_);
  if (tgt_) {
    notifyOfSuccess();
  } else {
    notifyOfError(initError_);
  }
  return *this;
}

void Krb5Tgts::setTgt(std::unique_ptr<Krb5Credentials> tgt) {
  if (tgt == nullptr) {
    throw std::runtime_error("Can't set tgt to null");
  }
  WriteLock lock(lock_);
  tgt_ = std::move(tgt);
  notifyOfSuccess();
}

void Krb5Tgts::setTgtForRealm(const std::string& realm,
  std::unique_ptr<Krb5Credentials> tgt) {
  if (tgt == nullptr) {
    throw std::runtime_error("Can't set tgt to null");
  }
  WriteLock lock(lock_);
  realmTgtsMap_[realm] = std::move(tgt);
}

void Krb5Tgts::setClientPrincipal(const Krb5Principal& client) {
  WriteLock lock(lock_);
  client_ = folly::make_unique<Krb5Principal>(
    Krb5Principal::copyPrincipal(ctx_.get(), client.get()));
}

void Krb5Tgts::kInit(const Krb5Principal& client) {
  Krb5Keytab keytab(ctx_.get());
  // Verify that client is valid
  bool client_ok = isPrincipalInKeytab(client);
  if (!client_ok) {
    throw std::runtime_error(
      folly::to<string>("client principal ", client,
                        " is not available in keytab ", keytab.getName()));
  }
  Krb5Credentials new_creds = keytab.getInitCreds(client.get());
  WriteLock lock(lock_);
  client_ = folly::make_unique<Krb5Principal>(
    Krb5Principal::copyPrincipal(ctx_.get(), client.get()));
  // Other realm credentials become invalid, clear them
  realmTgtsMap_.clear();
  tgt_ = std::make_shared<Krb5Credentials>(
    std::move(new_creds));
  notifyOfSuccess();
}

void Krb5Tgts::notifyOfError(const std::string& error) {
  MutexGuard guard(initLock_);
  initError_ = error;
  initCondVar_.notify_all();
}

void Krb5Tgts::notifyOfSuccess() {
  MutexGuard guard(initLock_);
  initError_.clear();
  initCondVar_.notify_all();
}

std::shared_ptr<const Krb5Credentials> Krb5Tgts::getTgt() {
  waitForInit();
  ReadLock lock(lock_);
  return tgt_;
}

std::shared_ptr<const Krb5Credentials> Krb5Tgts::getTgtForRealm(
    const std::string& realm) {
  waitForInit();

  {
    ReadLock lock(lock_);
    // We're going cross-realm, we also need the tgt for the other realm.
    std::shared_ptr<Krb5Credentials> tgt = getForRealm(realm);
    if (tgt != nullptr) {
      return tgt;
    }
  }

  WriteLock write_lock(lock_);
  // Try again
  std::shared_ptr<Krb5Credentials> tgt = getForRealm(realm);
  if (tgt != nullptr) {
    return tgt;
  }

  Krb5Principal princ = Krb5Principal::copyPrincipal(
    ctx_.get(), tgt_->get().server);

  // Get the tgt name for the realm
  Krb5Principal realm_princ(
    ctx_.get(), folly::to<string>("krbtgt/", realm));

  // Make a new memory cache.
  auto mem = Krb5CCache::makeNewUnique("MEMORY");
  // Initialize the new CC
  mem.setDestroyOnClose();
  mem.initialize(client_->get());
  mem.storeCred(tgt_->get());

  auto realm_cred = mem.getCredentials(realm_princ.get(), KRB5_GC_NO_STORE);

  setForRealm(realm, std::move(realm_cred));
  return getForRealm(realm);
}

vector<string> Krb5Tgts::getValidRealms() {
  waitForInit();
  ReadLock lock(lock_);
  vector<string> ret;
  for (const auto& entry : realmTgtsMap_) {
    ret.push_back(entry.first);
  }
  return ret;
}

std::pair<uint64_t, uint64_t> Krb5Tgts::getLifetime() {
  waitForInit();
  // Get the lifetime of the main cred
  ReadLock lock(lock_);
  return std::make_pair(
    tgt_->get().times.starttime,
    tgt_->get().times.endtime);
}


void Krb5Tgts::waitForInit() {
  MutexGuard guard(initLock_);
  while (tgt_ == nullptr && initError_.empty()) {
    initCondVar_.wait(guard);
  }
  if (tgt_ == nullptr) {
    throw std::runtime_error(initError_);
  }
}

bool Krb5Tgts::isInitialized() {
  ReadLock lock(lock_);
  return (tgt_ != nullptr);
}

bool Krb5Tgts::isPrincipalInKeytab(
    const Krb5Principal& princ) {
  Krb5Keytab keytab(ctx_.get());
  for (auto& ktentry : keytab) {
    if (princ == Krb5Principal(
        ctx_.get(), std::move(ktentry.principal))) {
      return true;
    }
  }
  return false;
}

std::shared_ptr<Krb5Credentials> Krb5Tgts::getForRealm(
      const std::string& realm) {
  auto found = realmTgtsMap_.find(realm);
  if (found != realmTgtsMap_.end()) {
    return found->second;
  }
  return nullptr;
}

void Krb5Tgts::setForRealm(const std::string& realm, Krb5Credentials&& creds) {
  realmTgtsMap_[realm] = std::make_shared<Krb5Credentials>(std::move(creds));
}

Krb5Principal Krb5Tgts::getClientPrincipal() {
  waitForInit();
  ReadLock lock(lock_);
  return Krb5Principal::copyPrincipal(ctx_.get(), client_->get());
}

}}}
