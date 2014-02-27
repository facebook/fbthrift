/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
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

#include "folly/io/IOBuf.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h"

namespace apache { namespace thrift {

/**
 * Struct for kinit data.
 */
struct Krb5Data {
  krb5_context ctx;
  krb5_ccache cc;
  krb5_principal me;
  krb5_get_init_creds_opt *options;
  krb5_keytab keytab;

  Krb5Data() :
    ctx(0),
    cc(0),
    me(0),
    options(NULL),
    keytab(0) {}

  ~Krb5Data() {
    if (me)
        krb5_free_principal(ctx, me);
    if (cc)
        krb5_cc_close(ctx, cc);
    if (options)
        krb5_get_init_creds_opt_free(ctx, options);
    if (keytab)
        krb5_kt_close(ctx, keytab);
    if (ctx)
        krb5_free_context(ctx);
  }
};

static inline int
k5_data_eq(krb5_data d1, krb5_data d2) {
  return (d1.length == d2.length && !memcmp(d1.data, d2.data, d1.length));
}

static inline int
k5_data_eq_string (krb5_data d, const char *s) {
  return (d.length == strlen(s) && !memcmp(d.data, s, d.length));
}


/**
 * Kerberos handshake from the client-side.
 */
class KerberosSASLHandshakeClient {
  public:

    KerberosSASLHandshakeClient();

    virtual ~KerberosSASLHandshakeClient();

    /**
     * Start the client-server kerberos handshake. Can only call this
     * function once.
     */
    void startClientHandshake();

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
     * Get service / client principals. Only call these after the handshake
     * completed.
     */
    const std::string& getEstablishedServicePrincipal() const;
    const std::string& getEstablishedClientPrincipal() const;

    std::unique_ptr<folly::IOBuf> wrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);
    std::unique_ptr<folly::IOBuf> unwrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);

  private:
    uint32_t securityLayerBitmask_;
    std::unique_ptr<folly::IOBuf> securityLayerBitmaskBuffer_;

    PhaseType phase_;

    std::string clientPrincipal_;
    std::string servicePrincipal_;
    struct Krb5Data servicePrincipalKrbStruct_;

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
    void kInit();
    struct Krb5Data getKrb5Data();
    int getTgtLifetime();
    void throwKrb5Exception(
      const std::string& str,
      krb5_context ctx,
      krb5_error_code code);
};

}}  // apache::thrift

#endif
