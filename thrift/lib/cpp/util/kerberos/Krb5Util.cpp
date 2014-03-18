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

#include "thrift/lib/cpp/util/kerberos/Krb5Util.h"

#include <glog/logging.h>
#include <memory>
#include <string>
#include <vector>
#include "folly/ScopeGuard.h"

namespace std {

std::ostream& operator<<(std::ostream& os,
                         const std::pair<krb5_context, krb5_principal>& obj) {
  os << folly::to<std::string>(obj);
  return os;
}

}

namespace apache { namespace thrift { namespace krb5 {

void raiseIf(krb5_context context, krb5_error_code code,
             const std::string& what) {
  if (code == 0) {
    return;
  }

  const char* err = krb5_get_error_message(context, code);
  throw std::runtime_error(folly::to<std::string>(err, " while ", what));
}

std::vector<std::string> getHostRealm(krb5_context context,
                                      const std::string& hostName) {
  char **realms;
  // Get host realm into char **realms
  krb5_error_code code = krb5_get_host_realm(context,
                                             hostName.c_str(),
                                             &realms);
  try {
    raiseIf(context, code, "getting host realm in krb5util::getHostRealm");
  } catch (const std::runtime_error& ex) {
    krb5_free_host_realm(context, realms);
    throw ex;
  }

  // Convert char **realms to vector<string>
  std::vector<std::string> vRealms;
  for (int i=0; realms[i] != nullptr; ++i) {
    vRealms.push_back(realms[i]);
  }

  // Free up char **realms
  code = krb5_free_host_realm(context, realms);
  raiseIf(context, code, "freeing up host realm in krb5util::getHostRealm");
  return vRealms;
}

Krb5Context::Krb5Context() {
  krb5_error_code code = krb5_init_context(&context_);
  if (code) {
    LOG(FATAL) << "Error initializing kerberos library: "
               << error_message(code);
  }
}

Krb5Context::~Krb5Context() {
  krb5_free_context(context_);
}

Krb5Principal Krb5Principal::snameToPrincipal(krb5_context context,
    krb5_int32 type, const std::string& hostname,
    const std::string& sname) {
  krb5_principal princ;
  krb5_error_code code = krb5_sname_to_principal(
    context,
    hostname.empty() ? nullptr : hostname.c_str(),
    sname.empty() ? nullptr : sname.c_str(),
    type,
    &princ);
  raiseIf(context, code, folly::to<std::string>(
    "snameToPrincipal error: ", type, " ", hostname, " ", sname));
  return Krb5Principal(context, std::move(princ));
}

Krb5Principal::Krb5Principal(krb5_context context, const std::string& name)
    : context_(context)
    , principal_(nullptr) {
  krb5_error_code code = krb5_parse_name(context, name.c_str(), &principal_);
  raiseIf(context, code, folly::to<std::string>("parsing principal ", name));
}

Krb5Principal::Krb5Principal(krb5_context context, krb5_principal&& principal)
    : context_(context) {
  principal_ = principal;
  principal = nullptr;
}

Krb5Principal::Krb5Principal(Krb5Principal&& other)
  : context_(other.context_)
  , principal_(other.release()) {}

Krb5Principal& Krb5Principal::operator=(Krb5Principal&& other) {
  if (this != &other) {
    if (principal_) {
      krb5_free_principal(context_, principal_);
    }
    context_ = other.context_;
    principal_ = other.release();
    other.context_ = nullptr;
  }
  return *this;
}

Krb5Principal::~Krb5Principal() {
  if (principal_) {
    krb5_free_principal(context_, principal_);
  }
}

krb5_principal Krb5Principal::release() {
  krb5_principal ret = principal_;
  principal_ = nullptr;
  return ret;
}

std::ostream& operator<<(std::ostream& os, const Krb5Principal& obj) {
  os << folly::to<std::string>(obj);
  return os;
}

Krb5CCache::Krb5CCache(Krb5CCache&& other)
  : context_(other.context_)
  , ccache_(other.release()) {}

Krb5CCache Krb5CCache::makeDefault(krb5_context context) {
  krb5_ccache ccache;
  krb5_error_code code = krb5_cc_default(context, &ccache);
  raiseIf(context, code, "getting default ccache");
  return Krb5CCache(context, ccache);
}

Krb5CCache Krb5CCache::makeResolve(krb5_context context,
    const std::string& name) {
  krb5_ccache ccache;
  krb5_error_code code = krb5_cc_resolve(context, name.c_str(), &ccache);
  raiseIf(context, code, folly::to<std::string>(
    "failed to resolve ccache: ", name));
  return Krb5CCache(context, ccache);
}

Krb5CCache Krb5CCache::makeNewUnique(krb5_context context,
    const std::string& type) {
  krb5_ccache ccache;
  krb5_error_code code = krb5_cc_new_unique(context, type.c_str(), nullptr,
    &ccache);
  raiseIf(context, code, folly::to<std::string>(
    "failed to get new ccache with type: ", type));
  return Krb5CCache(context, ccache);
}

Krb5CCache::Krb5CCache(krb5_context context, krb5_ccache ccache)
    : context_(context)
    , ccache_(ccache) {
}

Krb5CCache& Krb5CCache::operator=(Krb5CCache&& other) {
  if (this != &other) {
    if (ccache_) {
      krb5_cc_close(context_, ccache_);
    }
    context_ = other.context_;
    ccache_ = other.release();
    other.context_ = nullptr;
  }
  return *this;
}

std::vector<Krb5Principal> Krb5CCache::getServicePrincipalList(
    bool filter_tgt) {
  std::vector<Krb5Principal> ret;
  for (auto it = begin(); it != end(); ++it) {
    Krb5Principal server(context_, std::move(it->server));
    if (filter_tgt && server.isTgt()) {
      continue;
    }

    ret.push_back(std::move(server));
  }

  return ret;
}

std::pair<uint64_t, uint64_t> Krb5CCache::getLifetime(
    krb5_principal principal) {
  const std::string client_realm = getClientPrincipal().getRealm();
  std::string princ_realm;
  if (principal) {
    Krb5Principal princ(context_, std::move(principal));
    princ_realm = princ.getRealm();
    princ.release();
  } else {
    princ_realm = client_realm;
  }

  for (auto& creds : *this) {
    Krb5Principal server(context_, std::move(creds.server));
    if (server.isTgt() &&
        server.getComponent(1) == princ_realm &&
        server.getRealm() == client_realm) {
      return std::make_pair(creds.times.starttime, creds.times.endtime);
    }
  }

  return std::make_pair(0, 0);
}

Krb5Principal Krb5CCache::getClientPrincipal() {
  krb5_principal client;
  krb5_error_code code = krb5_cc_get_principal(context_, ccache_, &client);
  raiseIf(context_, code, "getting client from ccache");
  return Krb5Principal(context_, std::move(client));
}

Krb5CCache::~Krb5CCache() {
  if (ccache_) {
    krb5_cc_close(context_, ccache_);
  }
}

krb5_ccache Krb5CCache::release() {
  krb5_ccache ret = ccache_;
  ccache_ = nullptr;
  return ret;
}

struct Krb5CCache::Iterator::State {
  State(Krb5CCache* cc, bool include_config_entries)
    : cc_(cc)
    , include_config_entries_(include_config_entries) {
    CHECK(cc);
    krb5_error_code code =
      krb5_cc_start_seq_get(cc_->getContext(), cc_->get(), &cursor_);
    raiseIf(cc_->getContext(), code, "reading credentials cache");
    memset(&creds_, 0, sizeof(creds_));
  }

  ~State() {
    krb5_free_cred_contents(cc_->getContext(), &creds_);
    krb5_error_code code =
      krb5_cc_end_seq_get(cc_->getContext(), cc_->get(), &cursor_);
    raiseIf(cc_->getContext(), code, "ending read of credentials cache");
  }

  bool next_any() {
    krb5_free_cred_contents(cc_->getContext(), &creds_);
    memset(&creds_, 0, sizeof(creds_));
    krb5_error_code code =
      krb5_cc_next_cred(cc_->getContext(), cc_->get(), &cursor_, &creds_);
    if (code == KRB5_CC_END) {
      return false;
    } else {
      raiseIf(cc_->getContext(), code, "reading next credential");
    }
    return true;
  }

  bool next() {
    bool valid = next_any();
    if (include_config_entries_) {
      return valid;
    }
    while (valid &&
           krb5_is_config_principal(cc_->getContext(), creds_.server)) {
      valid = next_any();
    }
    return valid;
  }

  Krb5CCache* cc_;
  bool include_config_entries_;
  krb5_cc_cursor cursor_;
  krb5_creds creds_;
};

Krb5CCache::Iterator::Iterator(Krb5CCache* cc, bool include_config_entries) {
  if (!cc) {
    return;
  }
  state_.reset(new State(cc, include_config_entries));
  next();
}

void Krb5CCache::Iterator::next() {
  try {
    if (!state_->next()) {
      state_.reset();
    }
  } catch (...) {
    state_.reset();
    throw;
  }
}

bool Krb5CCache::Iterator::isConfigEntry() {
  return state_ && krb5_is_config_principal(
    state_->cc_->getContext(), state_->creds_.server);
}

Krb5CCache::Iterator& Krb5CCache::Iterator::operator++() {  // prefix
  next();
  return *this;
}

Krb5CCache::Iterator::reference Krb5CCache::Iterator::operator*() {
  return state_->creds_;
}

Krb5Keytab::Krb5Keytab(krb5_context context, const std::string& name)
    : context_(context)
    , keytab_(nullptr) {
  if (name.empty()) {
    krb5_error_code code = krb5_kt_default(context, &keytab_);
    raiseIf(context, code, "getting default keytab");
  } else {
    krb5_error_code code = krb5_kt_resolve(context, name.c_str(), &keytab_);
    raiseIf(context, code, folly::to<std::string>(
      "failed to open keytab: ", name));
  }
}

Krb5Keytab::~Krb5Keytab() {
  if (keytab_) {
    krb5_kt_close(context_, keytab_);
  }
}

krb5_keytab Krb5Keytab::release() {
  krb5_keytab ret = keytab_;
  keytab_ = nullptr;
  return ret;
}

std::string Krb5Keytab::getName() const {
  char name[256];
  krb5_error_code code = krb5_kt_get_name(context_,
                                          keytab_,
                                          name,
                                          sizeof(name));
  raiseIf(context_, code, "getting keytab name");
  return name;
}

struct Krb5Keytab::Iterator::State {
  State(Krb5Keytab* kt)
    : kt_(kt) {
    CHECK(kt);
    krb5_error_code code =
      krb5_kt_start_seq_get(kt_->getContext(), kt_->get(), &cursor_);
    raiseIf(kt_->getContext(), code, "reading keytab");
    memset(&ktentry_, 0, sizeof(ktentry_));
  }

  ~State() {
    krb5_free_keytab_entry_contents(kt_->getContext(), &ktentry_);
    krb5_error_code code =
      krb5_kt_end_seq_get(kt_->getContext(), kt_->get(), &cursor_);
    raiseIf(kt_->getContext(), code, "ending read of keytab");
  }

  bool next() {
    krb5_free_keytab_entry_contents(kt_->getContext(), &ktentry_);
    memset(&ktentry_, 0, sizeof(ktentry_));
    krb5_error_code code =
      krb5_kt_next_entry(kt_->getContext(), kt_->get(), &ktentry_, &cursor_);
    if (code == KRB5_KT_END) {
      return false;
    } else {
      raiseIf(kt_->getContext(), code, "reading next credential");
    }
    return true;
  }

  Krb5Keytab* kt_;
  krb5_kt_cursor cursor_;
  krb5_keytab_entry ktentry_;
};

Krb5Keytab::Iterator::Iterator(Krb5Keytab* kt) {
  if (!kt) {
    return;
  }
  state_.reset(new State(kt));
  next();
}

void Krb5Keytab::Iterator::next() {
  try {
    if (!state_->next()) {
      state_.reset();
    }
  } catch (...) {
    state_.reset();
    throw;
  }
}

Krb5Keytab::Iterator& Krb5Keytab::Iterator::operator++() {  // prefix
  next();
  return *this;
}

Krb5Keytab::Iterator::reference Krb5Keytab::Iterator::operator*() {
  return state_->ktentry_;
}

Krb5InitCredsOpt::Krb5InitCredsOpt(krb5_context context)
    : context_(context)
    , options_(nullptr) {
  krb5_error_code code = krb5_get_init_creds_opt_alloc(context, &options_);
  raiseIf(context, code, "getting default options");
}

Krb5InitCredsOpt::~Krb5InitCredsOpt() {
  if (options_) {
    krb5_get_init_creds_opt_free(context_, options_);
  }
}

krb5_get_init_creds_opt* Krb5InitCredsOpt::release() {
  krb5_get_init_creds_opt* ret = options_;
  options_ = nullptr;
  return ret;
}

}}}
