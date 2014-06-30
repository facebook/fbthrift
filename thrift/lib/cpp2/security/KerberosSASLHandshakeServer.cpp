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
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h>

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>
#include <stdlib.h>
#include <folly/io/IOBuf.h>
#include <folly/io/Cursor.h>

using namespace std;
using namespace apache::thrift;
using namespace folly;

/**
 * Server functions.
 */
KerberosSASLHandshakeServer::KerberosSASLHandshakeServer() : phase_(INIT) {
  // Disable replay caching since we're doing mutual auth. Enabling this will
  // significantly degrade perf. Force this to overwrite existing env
  // variables to avoid performance regressions.
  setenv("KRB5RCACHETYPE", "none", 1);

  // Override the location of the conf file if it doesn't already exist.
  setenv("KRB5_CONFIG", "/etc/krb5-thrift.conf", 0);

  // Set required security properties, we can define setters for these if
  // they need to be modified later.
  minimumRequiredSecContextFlags_ =
    GSS_C_MUTUAL_FLAG |
    GSS_C_REPLAY_FLAG |
    GSS_C_SEQUENCE_FLAG |
    GSS_C_INTEG_FLAG |
    GSS_C_CONF_FLAG;

  serverName_ = GSS_C_NO_NAME;
  serverCreds_ = GSS_C_NO_CREDENTIAL;
  client_ = GSS_C_NO_NAME;
  context_ = GSS_C_NO_CONTEXT;
  contextStatus_ = GSS_S_NO_CONTEXT;

  // Bitmask specifying a requirement for all security layers and max
  // buffer length from the protocol. If we ever allow different security layer
  // properties, this would need to become more dynamic.
  securityLayerBitmask_ = 0x07ffffff;
  securityLayerBitmaskBuffer_ = IOBuf::create(sizeof(securityLayerBitmask_));
  io::Appender b(securityLayerBitmaskBuffer_.get(), 0);
  b.writeBE(securityLayerBitmask_);
}

KerberosSASLHandshakeServer::~KerberosSASLHandshakeServer() {
  OM_uint32 min_stat;
  if (context_ != GSS_C_NO_CONTEXT) {
    gss_delete_sec_context(&min_stat, &context_, GSS_C_NO_BUFFER);
  }
  if (serverName_ != GSS_C_NO_NAME) {
    gss_release_name(&min_stat, &serverName_);
  }
  if (serverCreds_ != GSS_C_NO_CREDENTIAL) {
    gss_release_cred(&min_stat, &serverCreds_);
  }
  if (client_ != GSS_C_NO_NAME) {
    gss_release_name(&min_stat, &client_);
  }
}

void KerberosSASLHandshakeServer::initServer() {
  assert(phase_ == INIT);

  OM_uint32 maj_stat, min_stat;
  context_ = GSS_C_NO_CONTEXT;

  if (servicePrincipal_.size() > 0) {
    gss_buffer_desc service_principal_token;
    service_principal_token.value = (void *)servicePrincipal_.c_str();
    service_principal_token.length = servicePrincipal_.size() + 1;

    maj_stat = gss_import_name(
      &min_stat,
      &service_principal_token,
      (gss_OID) gss_nt_krb5_name,
      &serverName_);
    if (maj_stat != GSS_S_COMPLETE) {
      KerberosSASLHandshakeUtils::throwGSSException(
        "Error initiating server credentials", maj_stat, min_stat);
    }

    maj_stat = gss_acquire_cred(
      &min_stat,
      serverName_,
      GSS_C_INDEFINITE,
      GSS_C_NO_OID_SET,
      GSS_C_ACCEPT,
      &serverCreds_,
      nullptr,
      nullptr
    );
    if (maj_stat != GSS_S_COMPLETE) {
      KerberosSASLHandshakeUtils::throwGSSException(
        "Error establishing server credentials", maj_stat, min_stat);
    }
  }

  phase_ = ESTABLISH_CONTEXT;
}

void KerberosSASLHandshakeServer::acceptSecurityContext() {
  assert(phase_ == ESTABLISH_CONTEXT);

  OM_uint32 min_stat;

  outputToken_.reset(new gss_buffer_desc);
  *outputToken_ = GSS_C_EMPTY_BUFFER;

  if (client_ != GSS_C_NO_NAME) {
    gss_release_name(&min_stat, &client_);
  }

  contextStatus_ = gss_accept_sec_context(
    &min_stat,
    &context_,
    serverCreds_,
    inputToken_.get(),
    GSS_C_NO_CHANNEL_BINDINGS,
    &client_,
    &doid_,
    outputToken_.get(),
    &retFlags_,
    nullptr,  // time_rec
    nullptr // del_cred_handle
  );

  if (contextStatus_ != GSS_S_COMPLETE &&
      contextStatus_ != GSS_S_CONTINUE_NEEDED) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error initiating server context", contextStatus_, min_stat);
  }

  if (contextStatus_ == GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::getContextData(
      context_,
      contextLifetime_,
      contextSecurityFlags_,
      establishedClientPrincipal_,
      establishedServicePrincipal_);

    if ((minimumRequiredSecContextFlags_ & contextSecurityFlags_) !=
          minimumRequiredSecContextFlags_) {
      throw TKerberosException("Not all security properties established");
    }
    phase_ = CONTEXT_NEGOTIATION_COMPLETE;
  }
}

std::unique_ptr<std::string> KerberosSASLHandshakeServer::getTokenToSend() {
  switch(phase_) {
    case INIT:
      // Should not call this function if in INIT state
      assert(false);
    case ESTABLISH_CONTEXT:
    case CONTEXT_NEGOTIATION_COMPLETE:
      // If the server is establishing a security context or has completed
      // it, it may still need to send a token back so the client can
      // finish establishing it's context.
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
    case COMPLETE:
      // Send empty token back
      return unique_ptr<string>(new string(""));
      break;
    default:
      break;
  }
  return nullptr;
}

void KerberosSASLHandshakeServer::handleResponse(const string& msg) {
  unique_ptr<IOBuf> unwrapped_security_layer_msg;
  switch(phase_) {
    case INIT:
      initServer();
      phase_ = ESTABLISH_CONTEXT;
      // fall through to phase 1
    case ESTABLISH_CONTEXT:
      if (inputToken_ == nullptr) {
        inputToken_.reset(new gss_buffer_desc);
      }
      inputToken_->length = msg.length();
      inputTokenValue_ = vector<unsigned char>(msg.begin(), msg.end());
      inputToken_->value = &inputTokenValue_[0];
      acceptSecurityContext();
      break;
    case CONTEXT_NEGOTIATION_COMPLETE:
      // This state will send out a security layer bitmask,
      // max buffer size, and authorization identity.
      phase_ = SELECT_SECURITY_LAYER;
      break;
    case SELECT_SECURITY_LAYER:
    {
      // Here we unwrap the message and make sure that it has the correct
      // security properties.
      unique_ptr<IOBuf> unwrapped_security_layer_msg = unwrapMessage(std::move(
        IOBuf::wrapBuffer(msg.c_str(), msg.length())));
      io::Cursor c = io::Cursor(unwrapped_security_layer_msg.get());
      uint32_t security_layers = c.readBE<uint32_t>();
      if ((security_layers & securityLayerBitmask_) >> 24 == 0 ||
          (security_layers & 0x00ffffff) != 0x00ffffff) {
        // the top 8 bits contain:
        // in security_layers (received from client):
        //    selected layer
        // in securityLayerBitmask_ (local):
        //    a bitmask of the available layers
        // bottom 3 bytes contain the max buffer size
        throw TKerberosException("Security layer negotiation failed");
      }
      phase_ = COMPLETE;
      break;
    }
    default:
      break;
  }
}

bool KerberosSASLHandshakeServer::isContextEstablished() {
  return phase_ == COMPLETE;
}

PhaseType KerberosSASLHandshakeServer::getPhase() {
  return phase_;
}

void KerberosSASLHandshakeServer::setRequiredServicePrincipal(
  const std::string& service) {

  assert(phase_ == INIT);
  servicePrincipal_ = service;
}

const string& KerberosSASLHandshakeServer::getEstablishedServicePrincipal()
  const {

  assert(phase_ == COMPLETE);
  return establishedServicePrincipal_;
}

const string& KerberosSASLHandshakeServer::getEstablishedClientPrincipal()
  const {

  assert(phase_ == COMPLETE);
  return establishedClientPrincipal_;
}

unique_ptr<folly::IOBuf> KerberosSASLHandshakeServer::wrapMessage(
    unique_ptr<folly::IOBuf>&& buf) {
  assert(contextStatus_ == GSS_S_COMPLETE);
  return KerberosSASLHandshakeUtils::wrapMessage(
    context_,
    std::move(buf)
  );
}

unique_ptr<folly::IOBuf> KerberosSASLHandshakeServer::unwrapMessage(
    unique_ptr<folly::IOBuf>&& buf) {
  assert(contextStatus_ == GSS_S_COMPLETE);
  return KerberosSASLHandshakeUtils::unwrapMessage(
    context_,
    std::move(buf)
  );
}
