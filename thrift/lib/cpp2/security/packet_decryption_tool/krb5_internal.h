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
#pragma once

#include <krb5.h>

// The following functions and #define's are pulled from krb5 library
// because they are not available from krb5.h.

#define KG_USAGE_ACCEPTOR_SEAL 22
#define KG_USAGE_INITIATOR_SEAL 24
#define KG_TOK_CTX_AP_REQ 0x0100
#define KG_TOK_CTX_AP_REP 0x0200

#define TREAD_STR(ptr, str, len)                \
   (str) = (ptr);                               \
   (ptr) += (len);

extern "C" {
  void KRB5_CALLCONV krb5_free_ap_rep(krb5_context, krb5_ap_rep *);
  gss_int32 gssint_g_verify_token_header (const gss_OID_desc * mech,
                                          unsigned int *body_size,
                                          unsigned char **buf,
                                          int tok_type,
                                          unsigned int toksize_in,
                                          int flags);
  krb5_error_code decode_krb5_ap_req(const krb5_data *code,
                                     krb5_ap_req **request);
  krb5_error_code decode_krb5_ap_rep(const krb5_data *code,
                                     krb5_ap_rep **reply);
  krb5_error_code decode_krb5_authenticator(const krb5_data *code,
                                            krb5_authenticator **auth);
  krb5_error_code decode_krb5_ap_rep_enc_part(const krb5_data *code,
                                              krb5_ap_rep_enc_part **enc);
  int gss_krb5int_rotate_left(void *ptr, size_t bufsiz, size_t rc);

  static inline unsigned short load_16_be(const void *cvp) {
    const unsigned char *p = (const unsigned char *) cvp;
    return (p[1] | (p[0] << 8));
  }
}
