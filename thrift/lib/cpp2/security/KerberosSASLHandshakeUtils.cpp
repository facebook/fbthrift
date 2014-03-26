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

#include "thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h"

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

#include "folly/io/IOBuf.h"
#include "folly/io/Cursor.h"

using namespace std;
using namespace apache::thrift;
using namespace folly;

/**
 * Some utility functions.
 */
unique_ptr<folly::IOBuf> KerberosSASLHandshakeUtils::wrapMessage(
  gss_ctx_id_t context,
  unique_ptr<folly::IOBuf>&& buf) {

  OM_uint32 maj_stat, min_stat;
  gss_buffer_desc in_buf;
  int state;
  in_buf.value = (void *)buf->data();
  in_buf.length = buf->length();

  unique_ptr<gss_buffer_desc, GSSBufferDeleter> out_buf(new gss_buffer_desc);
  *out_buf = GSS_C_EMPTY_BUFFER;

  maj_stat = gss_wrap(
    &min_stat,
    context,
    1, // conf and integrity requested
    GSS_C_QOP_DEFAULT,
    &in_buf,
    &state,
    out_buf.get()
  );
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error wrapping message", maj_stat, min_stat);
  }

  unique_ptr<folly::IOBuf> wrapped = folly::IOBuf::copyBuffer(
    out_buf->value,
    out_buf->length
  );

  return wrapped;
}

unique_ptr<folly::IOBuf> KerberosSASLHandshakeUtils::unwrapMessage(
  gss_ctx_id_t context,
  unique_ptr<folly::IOBuf>&& buf) {

  OM_uint32 maj_stat, min_stat;
  gss_buffer_desc in_buf;
  int state;
  in_buf.value = (void *)buf->data();
  in_buf.length = buf->length();

  unique_ptr<gss_buffer_desc, GSSBufferDeleter> out_buf(new gss_buffer_desc);
  *out_buf = GSS_C_EMPTY_BUFFER;

  maj_stat = gss_unwrap(
    &min_stat,
    context,
    &in_buf,
    out_buf.get(),
    &state,
    (gss_qop_t *) nullptr // quality of protection output... we don't need it
  );
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error unwrapping message", maj_stat, min_stat);

  }
  unique_ptr<folly::IOBuf> unwrapped = folly::IOBuf::copyBuffer(
    out_buf->value,
    out_buf->length
  );

  return unwrapped;
}

string KerberosSASLHandshakeUtils::getStatusHelper(
  OM_uint32 code,
  int type) {

  OM_uint32 min_stat;
  OM_uint32 msg_ctx = 0;
  string output;

  unique_ptr<gss_buffer_desc, GSSBufferDeleter> out_buf(new gss_buffer_desc);
  *out_buf = GSS_C_EMPTY_BUFFER;

  while (true) {
    gss_display_status(
      &min_stat,
      code,
      type,
      (gss_OID) gss_mech_krb5, // mech type, default to krb 5
      &msg_ctx,
      out_buf.get()
    );
    output += " " + string((char *)out_buf->value);
    (void) gss_release_buffer(&min_stat, out_buf.get());
    if (!msg_ctx) {
      break;
    }
  }
  return output;
}

string KerberosSASLHandshakeUtils::getStatus(
  OM_uint32 maj_stat,
  OM_uint32 min_stat) {

  string output;
  output += getStatusHelper(maj_stat, GSS_C_GSS_CODE);
  output += ";" + getStatusHelper(min_stat, GSS_C_MECH_CODE);
  return output;
}

string KerberosSASLHandshakeUtils::throwGSSException(
  const string& msg,
  OM_uint32 maj_stat,
  OM_uint32 min_stat) {

  throw TKerberosException(msg + getStatus(maj_stat, min_stat));
}

void KerberosSASLHandshakeUtils::getContextData(
  gss_ctx_id_t context,
  OM_uint32& context_lifetime,
  OM_uint32& context_security_flags,
  string& client_principal,
  string& service_principal) {

  OM_uint32 maj_stat, min_stat;

  // Acquire name buffers
  unique_ptr<gss_name_t, GSSNameDeleter> client_name(new gss_name_t);
  unique_ptr<gss_name_t, GSSNameDeleter> service_name(new gss_name_t);

  maj_stat = gss_inquire_context(
    &min_stat,
    context,
    client_name.get(),
    service_name.get(),
    &context_lifetime,
    nullptr, // mechanism
    &context_security_flags,
    nullptr, // is local
    nullptr // is open
  );
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error inquiring context", maj_stat, min_stat);
  }
  std::unique_ptr<gss_buffer_desc, GSSBufferDeleter> client_name_buf(
    new gss_buffer_desc);
  std::unique_ptr<gss_buffer_desc, GSSBufferDeleter> service_name_buf(
    new gss_buffer_desc);
  *client_name_buf = GSS_C_EMPTY_BUFFER;
  *service_name_buf = GSS_C_EMPTY_BUFFER;

  maj_stat = gss_display_name(
    &min_stat,
    *client_name.get(),
    client_name_buf.get(),
    (gss_OID *) nullptr);
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error getting client name", maj_stat, min_stat);
  }

  maj_stat = gss_display_name(
    &min_stat,
    *service_name.get(),
    service_name_buf.get(),
    (gss_OID *) nullptr);
  if (maj_stat != GSS_S_COMPLETE) {
    KerberosSASLHandshakeUtils::throwGSSException(
      "Error getting service name", maj_stat, min_stat);
  }

  client_principal = string(
    (char *)client_name_buf->value,
    client_name_buf->length);
  service_principal = string(
    (char *)service_name_buf->value,
    service_name_buf->length);
}
