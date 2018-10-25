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

#pragma once

// As Kerberos is not used at runtime, but still piped through a lot of
// the code, for Windows support we just stub out literally everything
// with std::terminate(). Don't try and call kerberos or you're going
// to have a bad time.

#ifndef _WIN32
extern "C" {
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5/krb5.h>
}
#else
#include <cstdint>
#include <exception>

#define GSS_S_COMPLETE 0
#define GSS_S_CONTINUE_NEEDED 0
#define GSS_S_NO_CRED 0
#define GSS_S_NO_CONTEXT 0

#define GSS_C_MUTUAL_FLAG 0
#define GSS_C_REPLAY_FLAG 0
#define GSS_C_SEQUENCE_FLAG 0
#define GSS_C_INTEG_FLAG 0
#define GSS_C_CONF_FLAG 0

#define GSS_C_ACCEPT 0
#define GSS_C_EMPTY_BUFFER 0
#define GSS_C_GSS_CODE 0
#define GSS_C_INDEFINITE 0
#define GSS_C_MECH_CODE 0
#define GSS_C_NO_BUFFER 0
#define GSS_C_NO_CHANNEL_BINDINGS 0
#define GSS_C_NO_CONTEXT 0
#define GSS_C_NO_CREDENTIAL 0
#define GSS_C_NO_NAME 0
#define GSS_C_NO_OID_SET 0

#define KRB5_NT_UNKNOWN 0
#define KRB5_CC_END 0
#define KRB5_KT_END 0

using OM_uint32 = uint32_t;

using gss_OID = uint32_t;
#define gss_mech_krb5 0
#define gss_nt_krb5_name 0

using krb5_boolean = bool;
using krb5_int32 = int32_t;

using krb5_flags = uint32_t;
using krb5_error_code = uint32_t;

inline const char* error_message(krb5_error_code) {
  std::terminate();
}

inline krb5_error_code krb5_init_context(void*) {
  std::terminate();
}

inline char* krb5_get_error_message(void*, krb5_error_code) {
  std::terminate();
}

inline krb5_error_code krb5_free_error_message(void*, const void*) {
  std::terminate();
}

inline krb5_error_code krb5_get_host_realm(void*, const void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_free_host_realm(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_free_context(void*) {
  std::terminate();
}

inline void krb5_free_principal(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_parse_name(void*, const void*, void*) {
  std::terminate();
}

inline krb5_error_code
krb5_sname_to_principal(void*, const void*, const void*, krb5_int32, void*) {
  std::terminate();
}

inline krb5_error_code krb5_copy_principal(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_default(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_copy_creds(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_resolve(void*, const void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_new_unique(void*, const void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_get_principal(void*, void*, void*) {
  std::terminate();
}

inline char* krb5_cc_get_type(void*, void*) {
  std::terminate();
}

inline char* krb5_cc_get_name(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_initialize(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_store_cred(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code
krb5_cc_retrieve_cred(void*, void*, krb5_flags, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_next_cred(void*, void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_start_seq_get(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_cc_end_seq_get(void*, void*, void*) {
  std::terminate();
}

inline void krb5_cc_close(void*, void*) {
  std::terminate();
}

inline void krb5_cc_destroy(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_next_entry(void*, void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_start_seq_get(void*, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_end_seq_get(void*, void*, void*) {
  std::terminate();
}

inline void krb5_free_keytab_entry_contents(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_get_init_creds_opt_alloc(void*, void*) {
  std::terminate();
}

inline void krb5_get_init_creds_opt_free(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_get_name(void*, void*, void*, size_t) {
  std::terminate();
}

inline krb5_error_code krb5_kt_default_name(void*, void*, size_t) {
  std::terminate();
}

inline krb5_error_code
krb5_get_init_creds_keytab(void*, void*, void*, void*, int, void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_default(void*, void*) {
  std::terminate();
}

inline krb5_error_code krb5_kt_resolve(void*, const void*, void*) {
  std::terminate();
}

inline void krb5_kt_close(void*, void*) {
  std::terminate();
}

inline krb5_error_code
krb5_get_credentials(void*, krb5_flags, void*, void*, void*) {
  std::terminate();
}

inline void krb5_free_creds(void*, void*) {
  std::terminate();
}

using krb5_principal = void*;
using krb5_const_principal = const krb5_principal;

using krb5_ccache = void*;
using krb5_cc_cursor = void*;
using krb5_kt_cursor = void*;
using krb5_context = void*;
struct krb5_creds {
  krb5_principal client;
  krb5_principal server;
  struct {
    uint32_t starttime;
    uint32_t endtime;
  } times;
};
struct krb5_data {
  char* data;
  uint32_t length;
};
using krb5_keytab = void*;
struct krb5_keytab_entry {
  krb5_principal principal;
};
using krb5_get_init_creds_opt = void*;

inline krb5_error_code krb5_unparse_name(void*, void*, void*) {
  std::terminate();
}

inline void krb5_free_cred_contents(void*, void*) {
  std::terminate();
}

inline uint32_t krb5_princ_size(void*, void*) {
  std::terminate();
}

inline krb5_data* krb5_princ_realm(void*, void*) {
  std::terminate();
}

inline krb5_data* krb5_princ_component(void*, void*, uint32_t) {
  std::terminate();
}

inline bool krb5_principal_compare(void*, void*, void*) {
  std::terminate();
}

using gss_ctx_id_t = uint32_t;
using gss_cred_id_t = uint32_t;
using gss_name_t = uint32_t;

struct gss_buffer_desc {
  void* value;
  OM_uint32 length;

  gss_buffer_desc& operator=(int) {
    std::terminate();
  }
};

inline void gss_release_buffer(void*, void*) {
  std::terminate();
}

inline void gss_release_name(void*, void*) {
  std::terminate();
}

inline void gss_release_cred(void*, void*) {
  std::terminate();
}

inline void gss_delete_sec_context(void*, void*, int) {
  std::terminate();
}

inline OM_uint32 gss_display_name(void*, gss_name_t, void*, void*) {
  std::terminate();
}

inline OM_uint32 gss_import_name(void*, void*, int, void*) {
  std::terminate();
}

inline OM_uint32 gss_init_sec_context(
    void*,
    int,
    void*,
    gss_name_t,
    int,
    int,
    int,
    void*,
    void*,
    void*,
    void*,
    void*,
    void*) {
  std::terminate();
}

inline OM_uint32
gss_acquire_cred(void*, gss_name_t, int, int, int, void*, void*, void*) {
  std::terminate();
}

inline OM_uint32 gss_accept_sec_context(
    void*,
    void*,
    gss_cred_id_t,
    void*,
    int,
    void*,
    void*,
    void*,
    void*,
    void*,
    void*) {
  std::terminate();
}

inline OM_uint32 gss_inquire_context(
    void*,
    gss_ctx_id_t,
    void*,
    void*,
    void*,
    void*,
    void*,
    void*,
    void*) {
  std::terminate();
}

inline void gss_display_status(void*, int, int, int, void*, void*) {
  std::terminate();
}

#endif
