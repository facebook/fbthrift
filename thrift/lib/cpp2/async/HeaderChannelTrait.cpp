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
#include <thrift/lib/cpp2/async/HeaderChannelTrait.h>
#include <thrift/lib/cpp/TApplicationException.h>

namespace apache {
namespace thrift {

// We set a persistent header so we don't have to include a header in every
// message on the wire. If clientType changes from last used (e.g. SASL
// client connects to SASL-disabled server and falls back to non-SASL),
// replace the header.
void HeaderChannelTrait::updateClientType(CLIENT_TYPE ct) {
  if (prevClientType_ != ct) {
    if (ct == THRIFT_HEADER_SASL_CLIENT_TYPE) {
      setPersistentAuthHeader(true);
    } else if (ct == THRIFT_HEADER_CLIENT_TYPE) {
      setPersistentAuthHeader(false);
    }
    prevClientType_ = ct;
  }
}

void HeaderChannelTrait::setClientType(CLIENT_TYPE ct) {
  checkSupportedClient(ct);
  clientType_ = ct;
}

void HeaderChannelTrait::setSupportedClients(
    std::bitset<CLIENT_TYPES_LEN> const* clients) {
  if (clients) {
    supported_clients = *clients;
    // Let's support insecure Header if SASL isn't explicitly supported.
    // It's ok for both to be supported by the caller, too.
    if (!supported_clients[THRIFT_HEADER_SASL_CLIENT_TYPE]) {
      supported_clients[THRIFT_HEADER_CLIENT_TYPE] = true;
    }

    if (supported_clients[THRIFT_HEADER_SASL_CLIENT_TYPE]) {
      setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
    } else {
      setClientType(THRIFT_HEADER_CLIENT_TYPE);
    }
  } else {
    setSecurityPolicy(THRIFT_SECURITY_DISABLED);
  }
}

bool HeaderChannelTrait::isSupportedClient(CLIENT_TYPE ct) {
  return supported_clients[ct];
}

void HeaderChannelTrait::checkSupportedClient(CLIENT_TYPE ct) {
  if (!isSupportedClient(ct)) {
    throw TApplicationException(TApplicationException::UNSUPPORTED_CLIENT_TYPE,
                                "Transport does not support this client type");
  }
}

void HeaderChannelTrait::setSecurityPolicy(THRIFT_SECURITY_POLICY policy) {
  std::bitset<CLIENT_TYPES_LEN> clients;

  switch (policy) {
    case THRIFT_SECURITY_DISABLED: {
      clients[THRIFT_UNFRAMED_DEPRECATED] = true;
      clients[THRIFT_FRAMED_DEPRECATED] = true;
      clients[THRIFT_HTTP_SERVER_TYPE] = true;
      clients[THRIFT_HTTP_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_CLIENT_TYPE] = true;
      clients[THRIFT_FRAMED_COMPACT] = true;
      break;
    }
    case THRIFT_SECURITY_PERMITTED: {
      clients[THRIFT_UNFRAMED_DEPRECATED] = true;
      clients[THRIFT_FRAMED_DEPRECATED] = true;
      clients[THRIFT_HTTP_SERVER_TYPE] = true;
      clients[THRIFT_HTTP_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_SASL_CLIENT_TYPE] = true;
      clients[THRIFT_FRAMED_COMPACT] = true;
      break;
    }
    case THRIFT_SECURITY_REQUIRED: {
      clients[THRIFT_HEADER_SASL_CLIENT_TYPE] = true;
      break;
    }
  }

  setSupportedClients(&clients);
  securityPolicy_ = policy;
}
}
} // apache::thrift
