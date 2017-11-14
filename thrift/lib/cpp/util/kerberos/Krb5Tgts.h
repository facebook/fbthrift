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

#ifndef KRB5_TGTS
#define KRB5_TGTS

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <condition_variable>
#include <krb5.h>
#include <string>
#include <unordered_map>

#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>

namespace apache { namespace thrift { namespace krb5 {


class Krb5Tgts {
 public:
  Krb5Tgts() {}
  // This is a partial move, the locks in the original object are not used.
  // This is also not a thread-safe method. Make sure no setters are called
  // on tgts while this is called.
  Krb5Tgts& operator=(Krb5Tgts&& tgts);
  virtual ~Krb5Tgts() {}

  void setTgt(std::unique_ptr<Krb5Credentials> tgt);
  void setTgtForRealm(const std::string& realm,
    std::unique_ptr<Krb5Credentials> tgt);
  void kInit(const Krb5Principal& client);
  void notifyOfError(const std::string& error);
  void notifyOfSuccess();
  void setClientPrincipal(const Krb5Principal& client);

  /**
   * The getter methods below will block until the Krb5Tgts object is
   * initialized. ie. kInit is called or the tgt is explicitly set.
   */
  std::shared_ptr<const Krb5Credentials> getTgt();
  std::shared_ptr<const Krb5Credentials> getTgtForRealm(
    const std::string& realm);
  std::vector<std::string> getValidRealms();
  Krb5Principal getClientPrincipal();
  /**
   * Get lifetime of the currently loaded creds.
   */
  Krb5Lifetime getLifetime();
  std::map<std::string, Krb5Lifetime> getLifetimes();

  bool isInitialized();

  typedef boost::shared_mutex Lock;
  typedef boost::unique_lock<Lock> WriteLock;
  typedef boost::shared_lock<Lock> ReadLock;

 protected:
  static const uint32_t EXPIRATION_THRESHOLD_SEC;

  void waitForInit();
  bool isPrincipalInKeytab(const Krb5Principal& princ);
  std::shared_ptr<Krb5Credentials> getForRealm(const std::string& realm);
  void setForRealm(const std::string& realm, Krb5Credentials&& creds);

  Krb5Context ctx_;

  std::mutex initLock_;
  std::condition_variable initCondVar_;
  std::string initError_;

  std::unique_ptr<Krb5Principal> client_;

  Lock lock_;
  // Main tgt
  std::shared_ptr<Krb5Credentials> tgt_;
  // Tgt for other realms
  std::unordered_map<std::string, std::shared_ptr<Krb5Credentials>>
    realmTgtsMap_;
};

}}}

#endif
