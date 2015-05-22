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
#ifndef KERBEROS_SASL_HANDSHAKE_UTILS_H
#define KERBEROS_SASL_HANDSHAKE_UTILS_H

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

namespace apache { namespace thrift {

/**
 * GSS deleters.
 */
struct GSSNameDeleter {
  void operator()(gss_name_t* ptr) const {
    OM_uint32 min_stat;
    gss_release_name(&min_stat, ptr);
    delete ptr;
  }
};

struct GSSBufferDeleter {
  void operator()(gss_buffer_desc* ptr) const {
    OM_uint32 min_stat;
    if (ptr->length) {
      gss_release_buffer(&min_stat, ptr);
    }
    delete ptr;
  }
};

/**
 * Kerberos Exception.
 */
class TKerberosException: public
  ::apache::thrift::transport::TTransportException {
 public:
  explicit TKerberosException(const std::string& message):
    TTransportException(TTransportException::INTERNAL_ERROR, message) {}

  const char* what() const throw() override {
    if (message_.empty()) {
      return "TKerberosException";
    } else {
      return message_.c_str();
    }
  }
};

enum PhaseType {
  INIT,
  ESTABLISH_CONTEXT,
  CONTEXT_NEGOTIATION_COMPLETE,
  SELECT_SECURITY_LAYER,
  COMPLETE
};

enum class SecurityMech {
  KRB5_SASL,
  KRB5_GSS,
  KRB5_GSS_NO_MUTUAL,
};

/**
 * Utility methods for the SASL protocol
 */
class KerberosSASLHandshakeUtils {
  public:
    static std::unique_ptr<folly::IOBuf> wrapMessage(
      gss_ctx_id_t context,
      std::unique_ptr<folly::IOBuf>&& buf);
    static std::unique_ptr<folly::IOBuf> unwrapMessage(
      gss_ctx_id_t context,
      std::unique_ptr<folly::IOBuf>&& buf);
    static std::string getStatusHelper(
      OM_uint32 code,
      int type);
    static std::string getStatus(
      OM_uint32 maj_stat,
      OM_uint32 min_stat);
    static std::string throwGSSException(
      const std::string& message,
      OM_uint32 maj_stat,
      OM_uint32 min_stat);
    static void getContextData(
      gss_ctx_id_t context,
      OM_uint32& context_lifetime,
      OM_uint32& context_security_flags,
      std::string& service_principal,
      std::string& client_principal);
    /**
     * Helper function to be used with folly::IOBuf::takeOwnership.
     */
    static void GSSBufferFreeFunction(void *buf, void *arg);

};

}}  // apache::thrift

#endif
