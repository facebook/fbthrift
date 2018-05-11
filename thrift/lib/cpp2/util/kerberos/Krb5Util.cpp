/*
 * Copyright 2014-present Facebook, Inc.
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

#include <thrift/lib/cpp2/util/kerberos/Krb5Util.h>

#include <glog/logging.h>
#include <memory>
#include <string>
#include <vector>
#include <folly/ScopeGuard.h>
#include <folly/String.h>
#include <folly/portability/GFlags.h>

#include <thrift/lib/cpp2/util/kerberos/FBKrb5GetCreds.h>

DEFINE_string (thrift_krb5_user_instances, "admin,root,sudo",
       "List of possible instances(second component) of "
       "user kerberos credentials. Separated by ','. ");
static const int kKeytabNameMaxLength = 512;

extern "C" {
krb5_error_code krb5int_init_context_kdc(krb5_context*);
}

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
  auto msg = folly::to<std::string>(err, " while ", what);
  krb5_free_error_message(context, err);

  throw std::runtime_error(msg);
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

Krb5Context::Krb5Context(bool thread_local_ctx)
    : Krb5Context(thread_local_ctx ? ContextType::THREAD_LOCAL
                                   : ContextType::NORMAL) {}

Krb5Context::Krb5Context(ContextType type)
#ifdef KRB5_HAS_INIT_THREAD_LOCAL_CONTEXT
    : threadLocal_(ContextType::THREAD_LOCAL == type)
#endif
{
  krb5_error_code code = 0;
  switch (type) {
  case ContextType::NORMAL:
    code = krb5_init_context(&context_);
    break;
  case ContextType::THREAD_LOCAL:
    code = krb5_init_thread_local_context(&context_);
    break;
  case ContextType::KDC:
    code = krb5int_init_context_kdc(&context_);
    break;
  }

  if (code) {
    std::string msg = folly::to<std::string>(
      "Error while initialiing kerberos context: ",
      error_message(code));
    throw std::runtime_error(msg);
  }
}

Krb5Context::~Krb5Context() {
#ifdef KRB5_HAS_INIT_THREAD_LOCAL_CONTEXT
  if (threadLocal_) {
    // No need to free thread-local contexts. There is only one per thread,
    // and they're updated as needed.
    return;
  }
#endif

  // If thread-local context is not define, we're using regular context,
  // so we should free to avoid leaks.
  krb5_free_context(context_);
}

krb5_context Krb5Context::get() const {
#ifdef KRB5_HAS_INIT_THREAD_LOCAL_CONTEXT
  if (threadLocal_) {
    // Note that we need to re-init the thread-local context every time,
    // since every call to krb5_init_thread_local_context may invalidate
    // the old context pointers.
    krb5_error_code code = krb5_init_thread_local_context(
      (krb5_context*)(&context_));
    if (code) {
      LOG(FATAL) << "Error reinitializing thread-local context: "
                 << error_message(code);
    }
  }
#endif
  return context_;
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

Krb5Principal Krb5Principal::copyPrincipal(krb5_context context,
    krb5_const_principal princ) {
  krb5_principal copied_princ;
  krb5_error_code code = krb5_copy_principal(
    context,
    princ,
    &copied_princ);
  raiseIf(context, code, "krb5_principal copy failed");
  return Krb5Principal(context, std::move(copied_princ));
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

Krb5Principal::Krb5Principal(Krb5Principal&& other) noexcept
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

bool Krb5Principal::isUser() const {
  if (size() == 1) {
    return true;
  }
  if (size() > 2) {
    return false;
  }
  std::vector<std::string> userInstances;
  folly::split(",", FLAGS_thrift_krb5_user_instances, userInstances, true);
  return std::find(userInstances.begin(),
                   userInstances.end(),
                   getComponent(1)) != userInstances.end();
}

std::ostream& operator<<(std::ostream& os, const Krb5Principal& obj) {
  os << folly::to<std::string>(obj);
  return os;
}

Krb5Credentials::Krb5Credentials(krb5_creds&& creds)
  : context_(true)
  , creds_(std::make_unique<krb5_creds>()) {
  // struct assignment.  About 16 words.
  *creds_ = creds;
  // Zero the struct we copied from.  This can be safely passed to
  // krb5_free_cred_contents().
  memset(&creds, 0, sizeof(creds));
}

Krb5Credentials::Krb5Credentials(Krb5Credentials&& other) noexcept
  : context_(true)
  , creds_(std::move(other.creds_)) {}

Krb5Credentials& Krb5Credentials::operator=(Krb5Credentials&& other) {
  if (this != &other) {
    creds_ = std::move(other.creds_);
  }
  return *this;
}

Krb5CCache::Krb5CCache(Krb5CCache&& other) noexcept
  : context_(true)
    // order matters here.  We need to copy the destroy flag before
    // release() clears it.
  , destroy_(other.destroy_)
  , ccache_(other.release()) {}

Krb5CCache Krb5CCache::makeDefault() {
  krb5_ccache ccache;
  Krb5Context context(true);
  krb5_context ctx = context.get();
  krb5_error_code code = krb5_cc_default(ctx, &ccache);
  raiseIf(ctx, code, "getting default ccache");
  return Krb5CCache(ccache);
}

Krb5CCache Krb5CCache::makeResolve(const std::string& name) {
  krb5_ccache ccache;
  Krb5Context context(true);
  krb5_context ctx = context.get();
  krb5_error_code code = krb5_cc_resolve(ctx, name.c_str(), &ccache);
  raiseIf(ctx, code, folly::to<std::string>(
    "failed to resolve ccache: ", name));
  return Krb5CCache(ccache);
}

Krb5CCache Krb5CCache::makeNewUnique(const std::string& type) {
  krb5_ccache ccache;
  Krb5Context context(true);
  krb5_context ctx = context.get();
  krb5_error_code code = krb5_cc_new_unique(
    ctx, type.c_str(), nullptr, &ccache);
  raiseIf(ctx, code, folly::to<std::string>(
    "failed to get new ccache with type: ", type));
  return Krb5CCache(ccache);
}

Krb5CCache::Krb5CCache(krb5_ccache ccache)
    : context_(true)
    , destroy_(false)
    , ccache_(ccache) {
}

Krb5CCache& Krb5CCache::operator=(Krb5CCache&& other) {
  if (this != &other) {
    if (ccache_) {
      if (destroy_) {
        krb5_cc_destroy(context_.get(), ccache_);
      } else {
        krb5_cc_close(context_.get(), ccache_);
      }
    }
    // order matters here.  We need to copy the destroy flag before
    // release() clears it.
    destroy_ = other.destroy_;
    ccache_ = other.release();
  }
  return *this;
}

std::vector<Krb5Principal> Krb5CCache::getServicePrincipalList(
    bool filter_tgt) {
  std::vector<Krb5Principal> ret;
  krb5_context ctx = context_.get();
  for (auto it = begin(); it != end(); ++it) {
    Krb5Principal server(ctx, std::move(it->server));
    if (filter_tgt && server.isTgt()) {
      continue;
    }

    ret.push_back(std::move(server));
  }

  return ret;
}

void Krb5CCache::getCacheTypeAndName(std::string& cacheType,
                                     std::string& cacheName) const {
  krb5_context ctx = context_.get();
  cacheType = std::string(krb5_cc_get_type(ctx, ccache_));
  cacheName = std::string(krb5_cc_get_name(ctx, ccache_));
}

Krb5Lifetime Krb5CCache::getLifetime(krb5_principal principal) const {
  const std::string client_realm = getClientPrincipal().getRealm();
  std::string princ_realm;
  krb5_context ctx = context_.get();
  if (principal) {
    Krb5Principal princ(ctx, std::move(principal));
    princ_realm = princ.getRealm();
    princ.release();
  } else {
    princ_realm = client_realm;
  }

  for (auto& creds : *this) {
    Krb5Principal server(ctx, std::move(creds.server));
    if (server.isTgt() &&
        server.getComponent(1) == princ_realm &&
        server.getRealm() == client_realm) {
      return std::make_pair(creds.times.starttime, creds.times.endtime);
    }
  }

  return std::make_pair(0, 0);
}

Krb5Principal Krb5CCache::getClientPrincipal() const {
  krb5_principal client;
  krb5_context ctx = context_.get();
  krb5_error_code code = krb5_cc_get_principal(ctx, ccache_, &client);
  raiseIf(ctx, code, "getting client from ccache");
  return Krb5Principal(ctx, std::move(client));
}

std::string Krb5CCache::getName() const {
  std::string type;
  std::string name;
  getCacheTypeAndName(type, name);
  return folly::to<std::string>(type, ":", name);
}

void Krb5CCache::initialize(krb5_principal cprinc) {
  krb5_context ctx = context_.get();
  krb5_error_code code = krb5_cc_initialize(ctx, ccache_, cprinc);
  raiseIf(ctx, code, "initializing ccache");
}

void Krb5CCache::storeCred(const krb5_creds& creds) {
  krb5_context ctx = context_.get();
  // The krb5 impl doesn't modify creds.
  krb5_error_code code = krb5_cc_store_cred(
    ctx, ccache_, const_cast<krb5_creds*>(&creds));
  raiseIf(ctx, code, "store cred to ccache");
}

Krb5Credentials Krb5CCache::retrieveCred(
  const krb5_creds& match_creds, krb5_flags match_flags) {

  krb5_creds matched;
  krb5_context ctx = context_.get();
  // The krb5 impl doesn't modify match_creds.
  krb5_error_code code = krb5_cc_retrieve_cred(
    ctx, ccache_, match_flags, const_cast<krb5_creds*>(&match_creds),
    &matched);
  raiseIf(ctx, code, "retrieve cred");

  return Krb5Credentials(std::move(matched));
}

Krb5Credentials Krb5CCache::retrieveCred(krb5_principal sprinc) {
  Krb5Principal cprinc = getClientPrincipal();

  krb5_creds in_creds;
  memset(&in_creds, 0, sizeof(in_creds));
  in_creds.client = cprinc.get();
  in_creds.server = sprinc;

  return retrieveCred(in_creds, 0 /* flags */);
}

Krb5Credentials Krb5CCache::getCredentials(
  const krb5_creds& in_creds, krb5_flags options) {

  krb5_creds* out_creds;
  krb5_context ctx = context_.get();
  // The krb5 impl doesn't modify in_creds.
  krb5_error_code code = krb5_get_credentials(
    ctx, options, ccache_, const_cast<krb5_creds*>(&in_creds),
    &out_creds);
  raiseIf(ctx, code, "get credentials");
  SCOPE_EXIT { krb5_free_creds(ctx, out_creds); };

  return Krb5Credentials(std::move(*out_creds));
}

Krb5Credentials Krb5CCache::getCredentials(
  krb5_principal sprinc, krb5_flags options) {

  Krb5Principal cprinc = getClientPrincipal();

  krb5_creds in_creds;
  memset(&in_creds, 0, sizeof(in_creds));
  in_creds.client = cprinc.get();
  in_creds.server = sprinc;

  return getCredentials(in_creds, options);
}


Krb5CCache::~Krb5CCache() {
  if (ccache_) {
    if (destroy_) {
      krb5_cc_destroy(context_.get(), ccache_);
    } else {
      krb5_cc_close(context_.get(), ccache_);
    }
  }
}

krb5_ccache Krb5CCache::release() {
  krb5_ccache ret = ccache_;
  ccache_ = nullptr;
  destroy_ = false;
  return ret;
}

struct Krb5CCache::Iterator::State {
  State(const Krb5CCache* cc, bool include_config_entries)
    : cc_(cc)
    , include_config_entries_(include_config_entries)
    , context_(true) {
    CHECK(cc);
    krb5_context ctx = context_.get();
    krb5_error_code code =
      krb5_cc_start_seq_get(ctx, cc_->get(), &cursor_);
    raiseIf(ctx, code, "reading credentials cache");
    memset(&creds_, 0, sizeof(creds_));
  }

  ~State() {
    krb5_context ctx = context_.get();
    krb5_free_cred_contents(ctx, &creds_);
    krb5_error_code code = krb5_cc_end_seq_get(ctx, cc_->get(), &cursor_);
    raiseIf(ctx, code, "ending read of credentials cache");
  }

  bool next_any() {
    krb5_context ctx = context_.get();
    krb5_free_cred_contents(ctx, &creds_);
    memset(&creds_, 0, sizeof(creds_));
    krb5_error_code code =
      krb5_cc_next_cred(ctx, cc_->get(), &cursor_, &creds_);
    if (code == KRB5_CC_END) {
      return false;
    } else {
      raiseIf(ctx, code, "reading next credential");
    }
    return true;
  }

  bool next() {
    bool valid = next_any();
    if (include_config_entries_) {
      return valid;
    }
    krb5_context ctx = context_.get();
    while (valid &&
           krb5_is_config_principal(ctx, creds_.server)) {
      valid = next_any();
    }
    return valid;
  }

  const Krb5CCache* cc_;
  bool include_config_entries_;
  krb5_cc_cursor cursor_;
  krb5_creds creds_;
  Krb5Context context_;
};

Krb5CCache::Iterator::Iterator(
    const Krb5CCache* cc, bool include_config_entries) {
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
    state_->context_.get(), state_->creds_.server);
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
  char name[kKeytabNameMaxLength];
  krb5_error_code code = krb5_kt_get_name(context_,
                                          keytab_,
                                          name,
                                          sizeof(name));
  raiseIf(context_, code, "getting keytab name");
  return name;
}

std::string Krb5Keytab::getDefaultKeytabName(krb5_context context) {
  char name[kKeytabNameMaxLength];
  krb5_error_code code = krb5_kt_default_name(context,
                                              name,
                                              sizeof(name));
  raiseIf(context, code, "getting default keytab name");
  return name;
}

Krb5Credentials Krb5Keytab::getInitCreds(
  krb5_principal princ, krb5_get_init_creds_opt* opts) {

  krb5_creds creds;
  krb5_error_code code = krb5_get_init_creds_keytab(
    context_, &creds, princ, keytab_, 0 /* starttime */,
    nullptr /* initial sname */, opts);
  raiseIf(context_, code, "getting credentials from keytab");
  return Krb5Credentials(std::move(creds));
}

std::unique_ptr<Krb5Principal> Krb5Keytab::getFirstPrincipalInKeytab() {
  auto it = begin();
  if (it != end()) {
    return std::make_unique<Krb5Principal>(context_, std::move(it->principal));
  }
  return nullptr;
}

struct Krb5Keytab::Iterator::State {
  State(Krb5Keytab* kt)
    : kt_(kt) {
    CHECK(kt);
    krb5_error_code code =
      krb5_kt_start_seq_get(kt_->getContext(), kt_->get(), &cursor_);
    raiseIf(kt_->getContext(), code, "reading keytab " + kt_->getName());
    memset(&ktentry_, 0, sizeof(ktentry_));
  }

  ~State() {
    krb5_free_keytab_entry_contents(kt_->getContext(), &ktentry_);
    krb5_error_code code =
      krb5_kt_end_seq_get(kt_->getContext(), kt_->get(), &cursor_);
    raiseIf(kt_->getContext(), code, "ending read of keytab " + kt_->getName());
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
