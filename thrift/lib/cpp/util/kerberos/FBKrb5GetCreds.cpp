/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp/util/kerberos/FBKrb5GetCreds.h>
#include <glog/logging.h>

#ifdef KRB5_GC_NO_STORE
#include <memory>
#include <string>
#include <vector>

using apache::thrift::krb5::g_KdcTransportPlugin;

/*
 * Declaration of the internal method in krb5 library
 * which synchronously communicates with KDC.
 */
extern "C" {
krb5_error_code
krb5_sendto_kdc(krb5_context context, const krb5_data *message,
                const krb5_data *realm, krb5_data *reply, int *use_master,
                int tcp_only);
}


namespace {

static inline void *
k5calloc(size_t nmemb, size_t size, krb5_error_code *code)
{
    void *ptr;

    /* Allocate at least one byte since zero-byte allocs may return NULL. */
    ptr = calloc(nmemb ? nmemb : 1, size ? size : 1);
    *code = (ptr == nullptr) ? ENOMEM : 0;
    return ptr;
}

/* Allocate zeroed memory; set *code to 0 on success or ENOMEM on failure. */
static inline void *
k5alloc(size_t size, krb5_error_code *code)
{
    return k5calloc(1, size, code);
}

static inline krb5_data
make_data(void *data, unsigned int len)
{
    krb5_data d;

    d.magic = KV5M_DATA;
    d.data = (char *) data;
    d.length = len;
    return d;
}

static inline krb5_data
empty_data()
{
    return make_data(nullptr, 0);
}

krb5_error_code fb_krb5_tkt_creds_get(krb5_context context,
                                      krb5_tkt_creds_context ctx) {
    krb5_error_code code;
    krb5_data request = empty_data(), reply = empty_data();
    void* replyStorage = nullptr;
    krb5_data realm = empty_data();
    unsigned int flags = 0;
    int tcp_only = 1, use_master;

    for (;;) {
        /* Get the next request and realm.  Turn on TCP if necessary. */
        code = krb5_tkt_creds_step(context, ctx, &reply, &request, &realm,
                                   &flags);
        if (code == KRB5KRB_ERR_RESPONSE_TOO_BIG && !tcp_only) {
            tcp_only = 1;
        } else if (code != 0 || !(flags & KRB5_TKT_CREDS_STEP_FLAG_CONTINUE)) {
          break;
        }

        if (replyStorage) {
          g_KdcTransportPlugin.fnDtorStorage(&replyStorage);
          reply.data = nullptr;
          reply.length = 0;
        } else {
          krb5_free_data_contents(context, &reply);
        }

        /* Send it to a KDC for the appropriate realm. */
        use_master = 0;

        if (!g_KdcTransportPlugin.isAvailable()) {
          code = krb5_sendto_kdc(context, &request, &realm, &reply, &use_master,
                                 tcp_only);
        } else {
          if (!g_KdcTransportPlugin.fnSendAndRecv(&request, &realm, &reply,
                                                  &replyStorage)) {
            code = KRB5_KDC_UNREACH;
          }
        }

        if (code != 0) {
          break;
        }

        krb5_free_data_contents(context, &request);
        krb5_free_data_contents(context, &realm);
    }

    krb5_free_data_contents(context, &request);
    if (replyStorage) {
      g_KdcTransportPlugin.fnDtorStorage(&replyStorage);
      reply.data = nullptr;
      reply.length = 0;
    } else {
      krb5_free_data_contents(context, &reply);
    }
    krb5_free_data_contents(context, &realm);
    return code;
}

}

namespace apache { namespace thrift { namespace krb5 {

KdcTransportPlugin g_KdcTransportPlugin;

krb5_error_code fb_krb5_get_credentials(krb5_context context,
                                        krb5_flags options,
                                        krb5_ccache ccache,
                                        krb5_creds* in_creds,
                                        krb5_creds** out_creds) {
  krb5_error_code code;
  krb5_creds* ncreds = nullptr;
  krb5_tkt_creds_context ctx = nullptr;

  *out_creds = nullptr;

  /* Allocate a container. */
  ncreds = static_cast<krb5_creds*>(k5alloc(sizeof(*ncreds), &code));
  if (ncreds == nullptr)
    goto cleanup;

  /* Make and execute a krb5_tkt_creds context to get the credential. */
  code = krb5_tkt_creds_init(context, ccache, in_creds, options, &ctx);
  if (code != 0)
    goto cleanup;
  code = fb_krb5_tkt_creds_get(context, ctx);
  if (code != 0)
    goto cleanup;
  code = krb5_tkt_creds_get_creds(context, ctx, ncreds);
  if (code != 0)
    goto cleanup;

  *out_creds = ncreds;
  ncreds = nullptr;

cleanup:
  krb5_free_creds(context, ncreds);
  krb5_tkt_creds_free(context, ctx);
  return code;
}

}}}

#else

namespace apache { namespace thrift { namespace krb5 {
KdcTransportPlugin g_KdcTransportPlugin;

krb5_error_code fb_krb5_get_credentials(krb5_context context,
                                        krb5_flags options,
                                        krb5_ccache ccache,
                                        krb5_creds* in_creds,
                                        krb5_creds** out_creds) {
  // For legcay krb5 - just do a simple forward, also ensure
  // callers do not expect the plugin to be called.
  CHECK(!g_KdcTransportPlugin.isAvailable());
  return krb5_get_credentials(context, options, ccache, in_creds, out_creds);
}

}}}
#endif
