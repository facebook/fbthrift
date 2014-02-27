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
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeClient.h"

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>
#include <stdlib.h>
#include "folly/io/IOBuf.h"
#include "folly/io/Cursor.h"
#include "folly/Memory.h"
#include "thrift/lib/cpp/concurrency/Mutex.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"

extern "C" {
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
}

using namespace std;
using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::concurrency;

namespace {
static const ReadWriteMutex ccMutex;
}

/**
 * Client functions.
 */
KerberosSASLHandshakeClient::KerberosSASLHandshakeClient() : phase_(INIT) {
  // Override the location of the conf file if it doesn't already exist.
  setenv("KRB5_CONFIG", "/etc/krb5-thrift.conf", 0);

  // Set required security properties, we can define setters for these if
  // they need to be modified later.
  requiredFlags_ =
    GSS_C_MUTUAL_FLAG |
    GSS_C_REPLAY_FLAG |
    GSS_C_SEQUENCE_FLAG |
    GSS_C_INTEG_FLAG |
    GSS_C_CONF_FLAG;

  context_ = GSS_C_NO_CONTEXT;
  targetName_ = GSS_C_NO_NAME;
  clientCreds_ = GSS_C_NO_CREDENTIAL;
  contextStatus_ = GSS_S_NO_CONTEXT;

  // Bitmask specifying a requirement for all security layers and max
  // buffer length from the protocol. If we ever allow different security layer
  // properties, this would need to become more dynamic.
  // Confidentiality=04, Integrity=02, None=01.
  // Select only one of them (server can support several, client chooses one)
  securityLayerBitmask_ = 0x04ffffff;
  securityLayerBitmaskBuffer_ = IOBuf::create(sizeof(securityLayerBitmask_));
  io::Appender b(securityLayerBitmaskBuffer_.get(), 0);
  b.writeBE(securityLayerBitmask_);
}

KerberosSASLHandshakeClient::~KerberosSASLHandshakeClient() {
  OM_uint32 min_stat;
  if (context_ != GSS_C_NO_CONTEXT) {
    gss_delete_sec_context(&min_stat, &context_, GSS_C_NO_BUFFER);
  }
  if (targetName_ != GSS_C_NO_NAME) {
    gss_release_name(&min_stat, &targetName_);
  }
  if (clientCreds_ != GSS_C_NO_CREDENTIAL) {
    gss_release_cred(&min_stat, &clientCreds_);
  }
}

void KerberosSASLHandshakeClient::throwKrb5Exception(
    const std::string& custom,
    krb5_context ctx,
    krb5_error_code code) {
  const char* err = krb5_get_error_message(ctx, code);
  string err_str(err);
  throw TKerberosException(custom + " " + err_str);
}

int KerberosSASLHandshakeClient::getTgtLifetime() {
  RWGuard guard(ccMutex);

  struct Krb5Data k5 = getKrb5Data();
  krb5_error_code code = 0;
  krb5_cc_cursor cur;
  krb5_creds creds;
  krb5_flags flags;
  int lifetime = 0;

  if ((code = krb5_cc_default(k5.ctx, &k5.cc))) {
    return 0;
  }

  if ((code = krb5_cc_start_seq_get(k5.ctx, k5.cc, &cur))) {
    // If can't acquire cursor, likely that the cc doesn't exist, so just
    // return 0
    return 0;
  }

  while (!(code = krb5_cc_next_cred(k5.ctx, k5.cc, &cur, &creds))) {
    if (creds.server->length == 2 &&
        k5_data_eq(creds.server->realm, k5.me->realm) &&
        k5_data_eq_string(creds.server->data[0], "krbtgt") &&
        k5_data_eq(creds.server->data[1], k5.me->realm)) {
      lifetime = creds.times.endtime - creds.times.starttime;
      krb5_free_cred_contents(k5.ctx, &creds);
      break;
    }
    krb5_free_cred_contents(k5.ctx, &creds);
  }

  if (lifetime || code == KRB5_CC_END) {
    if ((code = krb5_cc_end_seq_get(k5.ctx, k5.cc, &cur))) {
      throwKrb5Exception("Failed finishing ticket retrieval", k5.ctx, code);
    }
  } else {
    throwKrb5Exception("Failed going through cc", k5.ctx, code);
  }

  return lifetime;
}

struct Krb5Data KerberosSASLHandshakeClient::getKrb5Data() {
  struct Krb5Data k5;
  krb5_error_code code = 0;
  code = krb5_init_context(&k5.ctx);
  if (code) {
    throwKrb5Exception("Failed initializing krb5 context", k5.ctx, code);
  }

  // If the client principal is specified, use it, otherwise
  // kinit the default one.
  if (clientPrincipal_.size() > 0) {
    code = krb5_parse_name(
      k5.ctx,
      clientPrincipal_.c_str(),
      &k5.me
    );
    if (code) {
      throwKrb5Exception("Failed parsing client principal", k5.ctx, code);
    }
  } else {
    code = krb5_sname_to_principal(
      k5.ctx,
      nullptr,
      nullptr,
      KRB5_NT_UNKNOWN,
      &k5.me
    );
    if (code) {
      throwKrb5Exception("Failed getting default principal", k5.ctx, code);
    }
  }
  return k5;
}

void KerberosSASLHandshakeClient::kInit() {
  RWGuard guard(ccMutex, true);

  struct Krb5Data k5 = getKrb5Data();
  krb5_error_code code = 0;

  code = krb5_cc_default(k5.ctx, &k5.cc);
  if (code) {
    throwKrb5Exception("Failed initializing default cache", k5.ctx, code);
  }

  code = krb5_get_init_creds_opt_alloc(k5.ctx, &k5.options);
  if (code) {
    throwKrb5Exception("Failed allocating options structure", k5.ctx, code);
  }

  // The krb5 library provides a mechanism to override this in the
  // environment.  If we want something more fb-specific, we can call
  // krb5_kt_resolve here.  However, we should always use
  // krb5_kt_default if nothing else is specified, so we don't lose
  // the library's idea of the default.

  code = krb5_kt_default(k5.ctx, &k5.keytab);
  if (code) {
    throwKrb5Exception("Failed opening default keytab file", k5.ctx, code);
  }

  krb5_creds my_creds;
  code = krb5_get_init_creds_keytab(
    k5.ctx,
    &my_creds,
    k5.me,
    k5.keytab,
    0,
    nullptr,
    k5.options);
  if (code) {
    throwKrb5Exception("Failed getting credentials from keytab", k5.ctx, code);
  }

  code = krb5_cc_initialize(k5.ctx, k5.cc, k5.me);
  if (code) {
    krb5_free_cred_contents(k5.ctx, &my_creds);
    throwKrb5Exception("Failed initializing credentials cache", k5.ctx, code);
  }

  code = krb5_cc_store_cred(k5.ctx, k5.cc, &my_creds);
  if (code) {
    krb5_free_cred_contents(k5.ctx, &my_creds);
    throwKrb5Exception("Failed storing into credentials cache", k5.ctx, code);
  }

  krb5_free_cred_contents(k5.ctx, &my_creds);

  return;
}

// copy-pasted from common/network/NetworkUtil to avoid dependency cycle
// between thrift and common/network
static string getHostByAddr(const string& ip) {
  struct addrinfo hints, *res, *res0;
  char hostname[NI_MAXHOST];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_flags = AI_NUMERICHOST;
  if (getaddrinfo(ip.c_str(), nullptr, &hints, &res0)) {
    return string();
  }

  for (res = res0; res; res = res->ai_next) {
    if (getnameinfo(res->ai_addr, res->ai_addrlen,
                    hostname, NI_MAXHOST, nullptr, 0, NI_NAMEREQD) < 0) {
      continue;
    }
    freeaddrinfo(res0);
    return string(hostname);
  }
  freeaddrinfo(res0);
  return string();
}

void KerberosSASLHandshakeClient::startClientHandshake() {
  assert(phase_ == INIT);

  auto guard = folly::make_unique<RWGuard>(ccMutex);

  OM_uint32 maj_stat, min_stat;
  context_ = GSS_C_NO_CONTEXT;

  // Convert ip to hostname if applicable. Also make sure the service
  // principal is in a valid format. <service>@<host> where <host> is non-empty
  // An empty <host> part may trigger a large buffer overflow and segfault
  // in the glibc codebase. :(
  string service, addr;
  size_t at = servicePrincipal_.find("@");
  if (at == string::npos) {
    throw TKerberosException(
      "Service principal invalid: " + servicePrincipal_);
  }

  addr = servicePrincipal_.substr(at + 1);
  service = servicePrincipal_.substr(0, at);

  if (addr.empty()) {
    throw TKerberosException(
      "Service principal invalid: " + servicePrincipal_);
  }

  // If a valid IPAddr, convert it to a hostname first.
  try {
    apache::thrift::transport::TSocketAddress ipaddr(addr, 0);
    if (ipaddr.getFamily() == AF_INET || ipaddr.getFamily() == AF_INET6) {
      string hostname = getHostByAddr(addr);
      if (!hostname.empty()) {
        addr = hostname;
        servicePrincipal_ = service + "@" + addr;
      }
    }
  } catch (...) {
    // If invalid ip address, don't do anything and swallow this exception.
  }

  // Initialize krb5 context
  krb5_error_code code = 0;
  code = krb5_init_context(&servicePrincipalKrbStruct_.ctx);
  if (code) {
    throwKrb5Exception("Failed initializing krb5 context",
      servicePrincipalKrbStruct_.ctx, code);
  }

  code = krb5_sname_to_principal(
    servicePrincipalKrbStruct_.ctx,
    addr.c_str(),
    service.c_str(),
    KRB5_NT_UNKNOWN,
    &servicePrincipalKrbStruct_.me
  );

  if (code) {
    throwKrb5Exception("Failed getting default principal",
      servicePrincipalKrbStruct_.ctx, code);
  }

  gss_buffer_desc service_principal_token;
  // We need to keep servicePrincipalKrbStruct_ alive as a member, while
  // the handshake is happening. This is because the gss_name
  // references into it.
  service_principal_token.value = (void *)&servicePrincipalKrbStruct_.me;
  service_principal_token.length = sizeof(krb5_principal);

  maj_stat = gss_import_name(
    &min_stat,
    &service_principal_token,
    (gss_OID) gss_nt_krb5_principal,
    &targetName_);
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error parsing server name on client", maj_stat, min_stat);
  }

  unique_ptr<gss_name_t, GSSNameDeleter> client_name(new gss_name_t);
  *client_name = GSS_C_NO_NAME;

  if (clientPrincipal_.size() > 0) {
    // If a client principal was explicitly specified, then establish
    // credentials using that principal, otherwise use the default.
    gss_buffer_desc client_name_tok;
    // It's ok to grab a c_str() pointer here since client_name_tok only
    // needs to be valid for a couple lines, in which the clientPrincipal_
    // is not modified.
    client_name_tok.value = (void *)clientPrincipal_.c_str();
    client_name_tok.length = clientPrincipal_.size() + 1;

    maj_stat = gss_import_name(
      &min_stat,
      &client_name_tok,
      (gss_OID) gss_nt_krb5_name,
      client_name.get());
    if (maj_stat != GSS_S_COMPLETE) {
      KerberosSASLHandshakeUtils::throwGSSException(
        "Error parsing client name on client", maj_stat, min_stat);
    }
  }

  // Attempt to acquire client credentials. On failure also try to kinit.
  bool kinit_attempted = false;
  do {
    maj_stat = gss_acquire_cred(
      &min_stat,
      *client_name.get(),
      GSS_C_INDEFINITE, // Max lifetime, will be controlled by connection
                        // lifetime. Limited by lifetime indicated in
                        // krb5.conf file.
      GSS_C_NO_OID_SET,
      GSS_C_INITIATE,
      &clientCreds_,
      nullptr,
      nullptr
    );

    if (maj_stat != GSS_S_COMPLETE) {
      if (!kinit_attempted) {
        LOG(INFO) << "Acquire credentials fail, attempting kinit: " <<
          KerberosSASLHandshakeUtils::getStatus(maj_stat, min_stat);
        guard->release();
        kInit();
        guard = folly::make_unique<RWGuard>(ccMutex);
        kinit_attempted = true;
        continue;
      }
      KerberosSASLHandshakeUtils::throwGSSException(
        "Error establishing client credentials", maj_stat, min_stat);
    }
  } while (maj_stat != GSS_S_COMPLETE);

  // Init phase complete, start establishing security context
  phase_ = ESTABLISH_CONTEXT;
  guard->release();
  initSecurityContext();
}

void KerberosSASLHandshakeClient::initSecurityContext() {
  assert(phase_ == ESTABLISH_CONTEXT);

  auto guard = folly::make_unique<RWGuard>(ccMutex);

  OM_uint32 ret_flags;
  OM_uint32 maj_stat, min_stat;

  outputToken_.reset(new gss_buffer_desc);
  *outputToken_ = GSS_C_EMPTY_BUFFER;

  // For expired tickets, credentials can successfully be acquired, but we
  // may still need to do a kinit to renew them. This is the reasoning behind
  // this loop.
  bool kinit_attempted = false;
  do {
    OM_uint32 time_rec = 0;
    contextStatus_ = gss_init_sec_context(
      &min_stat, // minor status
      clientCreds_,
      &context_, // context
      targetName_, // what we're connecting to
      (gss_OID) gss_mech_krb5, // mech type, default to krb 5
      requiredFlags_, // flags
      GSS_C_INDEFINITE, // Max lifetime, will be controlled by connection
                        // lifetime. Limited by lifetime indicated in
                        // krb5.conf file.
      nullptr, // channel bindings
      &inputToken_ != nullptr ? inputToken_.get() : GSS_C_NO_BUFFER,
      nullptr, // mech type
      outputToken_.get(), // output token
      &retFlags_, // return flags
      &time_rec // time_rec
    );

    // Get the lifetime of the tgt ticket
    int lifetime = getTgtLifetime();

    // Check if we should attempt kinit. We should attempt it if the ticket
    // had served half its lifetime
    bool should_renew =
      lifetime &&
      (time_rec < lifetime / 2) && // half the lifetime
      (contextStatus_ == GSS_S_COMPLETE);
    if (should_renew && !kinit_attempted) {
      LOG(INFO) << "Half-life reached, attempting kInit "
                << time_rec << " "
                << lifetime;
      try {
        guard->release();
        kInit();
        guard = folly::make_unique<RWGuard>(ccMutex);
      } catch (const std::exception& e) {
        LOG(ERROR) << "Half-life kinit failure" << e.what();
      } catch (...) {
        // Catch anything else that we don't know how to log generically.
        LOG(ERROR) << "Half-life kinit failure";
      }
      kinit_attempted = true;
      continue;
    } else if (should_renew && kinit_attempted) {
      LOG(ERROR) << "Half-life kinit failed to renew ticket";
    }

    if (contextStatus_ != GSS_S_COMPLETE &&
        contextStatus_ != GSS_S_CONTINUE_NEEDED) {
      if (!kinit_attempted) {
        LOG(INFO) << "Init sec context fail, attempting kinit: " <<
          KerberosSASLHandshakeUtils::getStatus(contextStatus_, min_stat);
        guard->release();
        kInit();
        guard = folly::make_unique<RWGuard>(ccMutex);
        kinit_attempted = true;
        continue;
      }

      KerberosSASLHandshakeUtils::throwGSSException(
        "Error initiating client context",
        contextStatus_,
        min_stat);
    }
  } while (contextStatus_ != GSS_S_COMPLETE &&
           contextStatus_ != GSS_S_CONTINUE_NEEDED);

  if (contextStatus_ == GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::getContextData(
      context_,
      contextLifetime_,
      contextSecurityFlags_,
      establishedClientPrincipal_,
      establishedServicePrincipal_);

    if ((requiredFlags_ & contextSecurityFlags_) != requiredFlags_) {
      throw TKerberosException("Not all security properties established");
    }

    phase_ = CONTEXT_NEGOTIATION_COMPLETE;
  }
}

std::unique_ptr<std::string> KerberosSASLHandshakeClient::getTokenToSend() {
  switch(phase_) {
    case INIT:
      // Should not call this function if in INIT state
      assert(false);
    case ESTABLISH_CONTEXT:
    case CONTEXT_NEGOTIATION_COMPLETE:
      return unique_ptr<string>(
        new string((const char*) outputToken_->value, outputToken_->length));
      break;
    case SELECT_SECURITY_LAYER:
    {
      unique_ptr<IOBuf> wrapped_sec_layer_message = wrapMessage(
        std::move(securityLayerBitmaskBuffer_));
      return unique_ptr<string>(new string(
        (char *)wrapped_sec_layer_message->data(),
        wrapped_sec_layer_message->length()
      ));
      break;
    }
    default:
      break;
  }
  return nullptr;
}

void KerberosSASLHandshakeClient::handleResponse(const string& msg) {
  switch(phase_) {
    case INIT:
      // Should not call this function if in INIT state
      assert(false);
    case ESTABLISH_CONTEXT:
      assert(contextStatus_ == GSS_S_CONTINUE_NEEDED);
      if (inputToken_ == nullptr) {
        inputToken_.reset(new gss_buffer_desc);
      }
      inputToken_->length = msg.length();
      inputTokenValue_ = vector<unsigned char>(msg.begin(), msg.end());
      inputToken_->value = &inputTokenValue_[0];
      initSecurityContext();
      break;
    case CONTEXT_NEGOTIATION_COMPLETE:
    {
      unique_ptr<IOBuf> unwrapped_security_layer_msg = unwrapMessage(std::move(
        IOBuf::wrapBuffer(msg.c_str(), msg.length())));
      io::Cursor c = io::Cursor(unwrapped_security_layer_msg.get());
      uint32_t security_layers = c.readBE<uint32_t>();
      if ((security_layers & securityLayerBitmask_) >> 24 == 0 ||
          (security_layers & 0x00ffffff) != 0x00ffffff) {
        // the top 8 bits contain:
        // in security_layers (received from server):
        //    a bitmask of the available layers
        // in securityLayerBitmask_ (local):
        //    selected layer
        // bottom 3 bytes contain the max buffer size
        throw TKerberosException("Security layer negotiation failed");
      }
      phase_ = SELECT_SECURITY_LAYER;
      break;
    }
    case SELECT_SECURITY_LAYER:
      // If we are in select security layer state and we get any message
      // from the server, it means that the server is successful, so complete
      // the handshake
      phase_ = COMPLETE;
      break;
    default:
      break;
  }
}

bool KerberosSASLHandshakeClient::isContextEstablished() {
  return phase_ == COMPLETE;
}

PhaseType KerberosSASLHandshakeClient::getPhase() {
  return phase_;
}

void KerberosSASLHandshakeClient::setRequiredServicePrincipal(
  const std::string& service) {

  assert(phase_ == INIT);
  servicePrincipal_ = service;
}

void KerberosSASLHandshakeClient::setRequiredClientPrincipal(
  const std::string& client) {

  assert(phase_ == INIT);
  clientPrincipal_ = client;
}

const string& KerberosSASLHandshakeClient::getEstablishedServicePrincipal()
  const {

  assert(phase_ == COMPLETE);
  return establishedServicePrincipal_;
}

const string& KerberosSASLHandshakeClient::getEstablishedClientPrincipal()
  const {

  assert(phase_ == COMPLETE);
  return establishedClientPrincipal_;
}

unique_ptr<folly::IOBuf> KerberosSASLHandshakeClient::wrapMessage(
    unique_ptr<folly::IOBuf>&& buf) {
  assert(contextStatus_ == GSS_S_COMPLETE);
  return KerberosSASLHandshakeUtils::wrapMessage(
    context_,
    std::move(buf)
  );
}

unique_ptr<folly::IOBuf> KerberosSASLHandshakeClient::unwrapMessage(
    unique_ptr<folly::IOBuf>&& buf) {
  assert(contextStatus_ == GSS_S_COMPLETE);
  return KerberosSASLHandshakeUtils::unwrapMessage(
    context_,
    std::move(buf)
  );
}
