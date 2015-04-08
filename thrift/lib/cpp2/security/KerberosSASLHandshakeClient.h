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

#ifndef KERBEROS_SASL_HANDSHAKE_CLIENT_H
#define KERBEROS_SASL_HANDSHAKE_CLIENT_H

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h>
#include <thrift/lib/cpp2/security/KerberosSASLThreadManager.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>
#include <thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h>
#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>
#include <thrift/lib/cpp/util/kerberos/Krb5OlderVersionStubs.h>

namespace apache { namespace thrift {

/**
 * Kerberos handshake from the client-side.
 */
class KerberosSASLHandshakeClient {
  public:

    explicit KerberosSASLHandshakeClient(
      const std::shared_ptr<SecurityLogger>& logger =
        std::make_shared<SecurityLogger>());

    virtual ~KerberosSASLHandshakeClient();

    /**
     * Start the client-server kerberos handshake. Can only call this
     * function once.
     */
    virtual void startClientHandshake();

    /**
     * Get the token to send. If nullptr then we don't need to send
     * anything else to the server.
     */
    std::unique_ptr<std::string> getTokenToSend();

    /**
     * Handle the response that comes back from the server.
     */
    void handleResponse(const std::string& msg);

    /**
     * Check to see if the kerberos context has been successfully established.
     */
    bool isContextEstablished();

    /**
     * Get current phase of the handshake.
     */
    PhaseType getPhase();

    /**
     * Set client / service principals. Only call these before the handshake
     * started.
     */
    void setRequiredServicePrincipal(const std::string& service);
    void setRequiredClientPrincipal(const std::string& client);

    /**
     * Set the security mechanism
     */
    void setSecurityMech(const SecurityMech mech);

    /**
     * Set the the function that grabs the required service principal.
     */
    void setRequiredServicePrincipalFetcher(
      std::function<std::pair<std::string, std::string>()>&& function);

    /**
     * Get service / client principals. Only call these after the handshake
     * completed.
     */
    const std::string& getEstablishedServicePrincipal() const;
    const std::string& getEstablishedClientPrincipal() const;

    std::unique_ptr<folly::IOBuf> wrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);
    std::unique_ptr<folly::IOBuf> unwrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);

    static void cleanUpState(
      gss_ctx_id_t context,
      gss_name_t target_name,
      gss_cred_id_t client_creds,
      const std::shared_ptr<SecurityLogger>& logger);

    virtual void setSaslThreadManager(
        const std::shared_ptr<SaslThreadManager>& thread_manager) {
      saslThreadManager_ = thread_manager;
    }

    virtual void setCredentialsCacheManager(
        const std::shared_ptr<krb5::Krb5CredentialsCacheManager>& cc_manager) {
      credentialsCacheManager_ = cc_manager;
    }

  private:
    uint32_t securityLayerBitmask_;
    std::unique_ptr<folly::IOBuf> securityLayerBitmaskBuffer_;

    PhaseType phase_;

    std::string clientPrincipal_;
    std::string servicePrincipal_;
    std::function<std::pair<std::string, std::string>()>
      getRequiredServicePrincipal_;

    // Some data about the context after it is established
    std::string establishedClientPrincipal_;
    std::string establishedServicePrincipal_;
    OM_uint32 contextLifetime_;
    OM_uint32 contextSecurityFlags_;

    gss_name_t targetName_;
    gss_ctx_id_t context_;
    OM_uint32 contextStatus_;

    std::unique_ptr<gss_buffer_desc> inputToken_;
    std::vector<unsigned char> inputTokenValue_;

    std::unique_ptr<gss_buffer_desc, GSSBufferDeleter> outputToken_;

    OM_uint32 retFlags_;

    gss_cred_id_t clientCreds_;

    OM_uint32 requiredFlags_;

    void initSecurityContext();
    void throwKrb5Exception(
      const std::string& str,
      krb5_context ctx,
      krb5_error_code code);

    std::shared_ptr<krb5::Krb5CCache> cc_;
    std::shared_ptr<SaslThreadManager> saslThreadManager_;
    std::shared_ptr<krb5::Krb5CredentialsCacheManager>
      credentialsCacheManager_;
    std::shared_ptr<SecurityLogger> logger_;

    SecurityMech securityMech_;
};

}}  // apache::thrift

#endif
