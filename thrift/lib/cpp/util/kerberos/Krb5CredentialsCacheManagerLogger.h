/*
 * Copyright 2004-present Facebook, Inc.
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

#pragma once

#include <map>

#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>

namespace apache {
namespace thrift {
namespace krb5 {
class Krb5CredentialsCacheManagerLogger : public virtual SecurityLogger {
 public:
  Krb5CredentialsCacheManagerLogger() {}
  virtual ~Krb5CredentialsCacheManagerLogger() {}

  virtual void logCredentialsCache(
      const std::string& /* key */,
      const Krb5Keytab& /* keytab */,
      const Krb5Principal& /* defaultPrincipal */,
      const Krb5Lifetime& /* ccacheLifetime */,
      const std::map<std::string, Krb5Lifetime>& /* serviceLifetimes */,
      const std::map<std::string, Krb5Lifetime>& /* tgtLifetimes */) {}
};
}
}
}
