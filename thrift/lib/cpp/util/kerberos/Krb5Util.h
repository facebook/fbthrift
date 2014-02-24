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

#ifndef KRB5_UTIL
#define KRB5_UTIL

/**
 * This module defines helper methods and classes to use the MIT krb5
 * library from the context of a C++11 application.  This includes
 * error handling, memory management, etc.
 */

#include <iostream>
#include <memory>
#include <string>

#include <krb5.h>

#include "folly/Conv.h"

namespace std {

// For converting krb5_principal to a string: this specialization
// enables the use of folly::to<string> on krb5_principal obtained
// from krb5 API calls.  It takes a pair with a krb5_context so the
// library methods can be called.  This has to be in the std namespace
// (same as std::pair) so ADL can find it.

template <class Tgt>
typename std::enable_if<folly::IsSomeString<Tgt>::value>::type
toAppend(const std::pair<krb5_context, krb5_principal>& value, Tgt * result) {
  char *name = nullptr;
  krb5_error_code code = krb5_unparse_name(value.first, value.second, &name);
  if (code == 0) {
    result->append(name);
    free(name);
  } else {
    // From code inspection, this only happens if the inputs are
    // invalid, krb5.conf has no default realm, or malloc fails.
    result->append("<unparse error>");
  }
}

/// For logging principal names.

std::ostream& operator<<(std::ostream& os,
                         const std::pair<krb5_context, krb5_principal>& obj);

}

namespace apache { namespace thrift { namespace krb5 {

/**
 * This is a convenience method which will raise a std::runtime_error
 * exception with a useful description if code != 0.
 */
void raiseIf(krb5_context context, krb5_error_code code,
             const std::string& what);

/**
 * A wrapper over krb5_get_host_realm function. Calls raiseIf() function if
 * krb5_get_host_realm does not succeed.
 * Input: krb5_context context, const string& hostName
 * Output: vector<string>
 */
std::vector<std::string> getHostRealm(krb5_context context,
                                      const std::string& hostName);

/**
 * RAII for krb5_context
 */
class Krb5Context {
public:
  Krb5Context();
  ~Krb5Context();

  krb5_context get() { return context_; }

private:
  krb5_context context_;
};

/**
 * For converting string to krb5_principal: this is a container for a
 * context and principal, with some convenience methods for common
 * cases.  It is movable but not copyable, so it has the same
 * ownership properties as a krb5_principal, but with RAII and better
 * enforcement.
 */

class Krb5Principal {
public:
  explicit Krb5Principal(krb5_context context, const std::string& name);
  static Krb5Principal snameToPrincipal(krb5_context context,
    krb5_int32 type, const std::string& hostname = "",
    const std::string& sname = "");

  // Take ownership of principal
  Krb5Principal(krb5_context context, krb5_principal&& principal);
  Krb5Principal(Krb5Principal&& other);
  Krb5Principal& operator=(Krb5Principal&& other);

  ~Krb5Principal();

  krb5_principal release();

  krb5_principal get() const { return principal_; }

  uint size() { return krb5_princ_size(context_, principal_); }
  std::string getRealm() {
    krb5_data* d = krb5_princ_realm(context_, principal_);
    return std::string(d->data, d->length);
  }
  std::string getComponent(uint nth) {
    krb5_data* d = krb5_princ_component(context_, principal_, nth);
    if (!d) {
      return "";
    }
    return std::string(d->data, d->length);
  }

  bool isTgt() {
    return size() == 2 && getComponent(0) == "krbtgt";
  }

  krb5_context get_context() const { return context_; }

private:
  krb5_context context_;
  krb5_principal principal_;
};

/**
 * For converting Krb5Principal to a string: this specialization
 * enables the use of folly::to<string> directly.
 */

template <class Tgt>
typename std::enable_if<folly::IsSomeString<Tgt>::value>::type
toAppend(const Krb5Principal& value, Tgt * result) {
  char *name = nullptr;
  krb5_error_code code =
    krb5_unparse_name(value.get_context(), value.get(), &name);
  if (code == 0) {
    result->append(name);
    free(name);
  } else {
    // From code inspection, this only happens if the inputs are
    // invalid, krb5.conf has no default realm, or malloc fails.
    result->append("<unparse error>");
  }
}

/// For logging Krb5Principal objects.

std::ostream& operator<<(std::ostream& os, const Krb5Principal& obj);

class Krb5CCache {
public:
  class Iterator : public std::iterator<std::input_iterator_tag, krb5_creds> {
  private:
    friend class Krb5CCache;

    explicit Iterator(Krb5CCache* cc, bool include_config_entries);

    void next_any();
    void next();

  public:
    ~Iterator() {}

    Iterator& operator++();  // prefix
    reference operator*();
    bool isConfigEntry();

    Iterator operator++(int) {  // postfix
      Iterator inc(*this);
      ++inc;
      return inc;
    }

    bool operator==(const Iterator& other) {
      return state_ == other.state_;
    }

    bool operator!=(const Iterator& other) {
      return !(*this == other);
    }

    pointer operator->() { return &(*(*this)); }

  private:
    struct State;
    std::shared_ptr<State> state_;
  };

  static Krb5CCache makeDefault(krb5_context context);
  static Krb5CCache makeResolve(krb5_context context,const std::string& name);
  static Krb5CCache makeNewUnique(krb5_context context,
    const std::string& type);

  // Disable copy
  Krb5CCache(const Krb5CCache& that) = delete;
  Krb5CCache(Krb5CCache&& other);
  ~Krb5CCache();
  Krb5CCache& operator=(Krb5CCache&& other);

  krb5_ccache release();
  krb5_ccache get() const { return ccache_; }
  krb5_context getContext() const { return context_; }

  Iterator begin(bool include_config_entries=false) {
    return Iterator(this, include_config_entries);
  }
  Iterator end() { return Iterator(nullptr, false); }

  /**
   * Gets a list of service credential principals.
   */
  std::vector<Krb5Principal> getServicePrincipalList(bool filter_tgt = true);
  /**
   * Gets the start and end times for a TGT associated with the realm
   * of 'principal'.  If the principal is nullptr, use the ccache's
   * client principal.
   */
  std::pair<uint64_t, uint64_t> getLifetime(krb5_principal principal = nullptr);
  Krb5Principal getClientPrincipal();

 private:
  Krb5CCache(krb5_context context, krb5_ccache ccache);

  krb5_context context_;
  krb5_ccache ccache_;
};

class Krb5Keytab {
 public:
  /**
   * Get a new keytab. If name is not specified uses the default keytab,
   * otherwise uses a keytab identified by name.
   */
  explicit Krb5Keytab(krb5_context context, const std::string& name = "");

  // Disable copy
  Krb5Keytab(const Krb5Keytab& that) = delete;
  ~Krb5Keytab();

  krb5_keytab release();
  krb5_keytab get() const { return keytab_; }
  krb5_context getContext() const { return context_; }

 private:
  krb5_context context_;
  krb5_keytab keytab_;
};

class Krb5InitCredsOpt {
 public:
  explicit Krb5InitCredsOpt(krb5_context context);

  // Disable copy
  Krb5InitCredsOpt(const Krb5InitCredsOpt& that) = delete;
  ~Krb5InitCredsOpt();

  krb5_get_init_creds_opt* release();
  krb5_get_init_creds_opt* get() const { return options_; }
  krb5_context getContext() const { return context_; }

 private:
  krb5_context context_;
  krb5_get_init_creds_opt* options_;
};

}}}

#endif
