/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

void HeaderChannelTrait::updateClientType(CLIENT_TYPE ct) {
  if (prevClientType_ != ct) {
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
    supported_clients[THRIFT_HEADER_CLIENT_TYPE] = true;

    setClientType(THRIFT_HEADER_CLIENT_TYPE);
  }
}

bool HeaderChannelTrait::isSupportedClient(CLIENT_TYPE ct) {
  return supported_clients[ct];
}

void HeaderChannelTrait::checkSupportedClient(CLIENT_TYPE ct) {
  if (!isSupportedClient(ct)) {
    throw TApplicationException(
        TApplicationException::UNSUPPORTED_CLIENT_TYPE,
        "Transport does not support this client type");
  }
}

HeaderChannelTrait::HeaderChannelTrait() {
  std::bitset<CLIENT_TYPES_LEN> clients;

  clients[THRIFT_UNFRAMED_DEPRECATED] = true;
  clients[THRIFT_UNFRAMED_COMPACT_DEPRECATED] = true;
  clients[THRIFT_FRAMED_DEPRECATED] = true;
  clients[THRIFT_HTTP_SERVER_TYPE] = true;
  clients[THRIFT_HTTP_CLIENT_TYPE] = true;
  clients[THRIFT_HEADER_CLIENT_TYPE] = true;
  clients[THRIFT_FRAMED_COMPACT] = true;

  setSupportedClients(&clients);
}
} // namespace thrift
} // namespace apache
