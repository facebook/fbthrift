/*
 * Copyright 2014-present Facebook, Inc.
 *
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
#ifndef KERBEROS_SASL_HANDSHAKE_SERVER_H
#define KERBEROS_SASL_HANDSHAKE_SERVER_H

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h>

namespace apache { namespace thrift {

/**
 * Kerberos handshake from the server-side.
 */
class KerberosSASLHandshakeServer {
  public:
    explicit KerberosSASLHandshakeServer();

    virtual ~KerberosSASLHandshakeServer();

    /**
     * Init the server credentials, etc. Only call this once.
     */
    void initServer();

    /**
     * Handle a response from the client.
     */
    void handleResponse(const std::string& msg);

    /**
     * Get the token to send. If nullptr then we don't need to send
     * anything else to the client.
     */
    std::unique_ptr<std::string> getTokenToSend();

    /**
     * Check to see if the kerberos context has been successfully established.
     */
    bool isContextEstablished();

    /**
     * Get current phase of the handshake.
     */
    PhaseType getPhase();

    /**
     * Set service principals. Only call these before the handshake
     * started.
     */
    void setRequiredServicePrincipal(const std::string& service);

    /**
     * Get service / client principals. Only call these after the handshake
     * completed.
     */
    const std::string& getEstablishedServicePrincipal() const;
    const std::string& getEstablishedClientPrincipal() const;

    /**
     * Get client principal. It can be called at anytime
     */
    const std::string& getClientPrincipal() const;

    /**
     * Set/get the security mechanism
     */
    void setSecurityMech(const SecurityMech mech);
    SecurityMech getSecurityMech();


    std::unique_ptr<folly::IOBuf> wrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);
    std::unique_ptr<folly::IOBuf> unwrapMessage(
      std::unique_ptr<folly::IOBuf>&& buf);
  private:
    uint32_t securityLayerBitmask_;
    std::unique_ptr<folly::IOBuf> securityLayerBitmaskBuffer_;

    PhaseType phase_;

    std::string servicePrincipal_;

    // Some data about the context after it is established
    std::string establishedClientPrincipal_;
    std::string establishedServicePrincipal_;
    OM_uint32 contextLifetime_;
    OM_uint32 contextSecurityFlags_;

    gss_ctx_id_t context_;
    OM_uint32 contextStatus_;
    gss_name_t serverName_;
    gss_cred_id_t serverCreds_;

    std::unique_ptr<gss_buffer_desc> inputToken_;
    std::vector<unsigned char> inputTokenValue_;

    std::unique_ptr<gss_buffer_desc, GSSBufferDeleter> outputToken_;

    OM_uint32 retFlags_;
    gss_name_t client_;
    gss_OID doid_;

    OM_uint32 minimumRequiredSecContextFlags_;

    SecurityMech securityMech_;

    void acceptSecurityContext();
};

}}  // apache::thrift

#endif
