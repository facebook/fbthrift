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

#pragma once

#include <krb5.h>
#include <functional>

namespace apache { namespace thrift { namespace krb5 {

krb5_error_code fb_krb5_get_credentials(krb5_context context,
                                        krb5_flags options,
                                        krb5_ccache ccache,
                                        krb5_creds* in_creds,
                                        krb5_creds** out_creds);

struct KdcTransportPlugin {
#ifdef KRB5_GC_NO_STORE
  static bool isSupported() { return true; }
#else
  // Legacy krb5 is not supported.
  static bool isSupported() { return false; }
#endif

  std::function<bool(const krb5_data* message,
                     const krb5_data* realm,
                     krb5_data* reply,
                     void** replyStorage)> fnSendAndRecv;
  std::function<void(void** replyStorage)> fnDtorStorage;
  std::function<bool()> fnIsAvailable;

  bool isAvailable() {
    bool success = (fnIsAvailable != nullptr) && fnIsAvailable();
    return success;
  }
};

extern KdcTransportPlugin g_KdcTransportPlugin;

}}}
