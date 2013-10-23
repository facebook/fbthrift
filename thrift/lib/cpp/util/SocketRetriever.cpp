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

#include "SocketRetriever.h"

#include <typeinfo>
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/transport/THeaderTransport.h"

namespace apache { namespace thrift { namespace util {


std::shared_ptr<transport::TSocket> SocketRetriever::castToSocket(
      std::shared_ptr<transport::TTransport> t) {
  std::shared_ptr<transport::TSocket> socket =
      std::dynamic_pointer_cast<transport::TSocket>(t);
  if (!socket) {
    T_DEBUG("Casting to TSocket failed");
  }
  return socket;
}

std::shared_ptr<transport::TSocket> SocketRetriever::getSocket(
    std::shared_ptr<transport::TTransport> t) {
  if (!t) {
    return std::shared_ptr<transport::TSocket>();
  }

  // check THeaderTransport before TFramedTransport because THeaderTransport is
  // a subclass of TFramedTransport, so that dynamic_cast would succeed as well,
  // but yield incorrect results.
  std::shared_ptr<transport::THeaderTransport> ht =
      std::dynamic_pointer_cast<transport::THeaderTransport>(t);
  if (ht) {
    return castToSocket(ht->getUnderlyingTransport());
  }

  std::shared_ptr<transport::TFramedTransport> ft =
      std::dynamic_pointer_cast<transport::TFramedTransport>(t);
  if (ft) {
    return castToSocket(ft->getUnderlyingTransport());
  }

  std::shared_ptr<transport::TBufferedTransport> bt =
      std::dynamic_pointer_cast<transport::TBufferedTransport>(t);
  if (bt) {
    return castToSocket(bt->getUnderlyingTransport());
  }

  return castToSocket(t);
}

std::shared_ptr<transport::TSocket> SocketRetriever::getSocket(
    protocol::TProtocol *p) {
  return getSocket(p->getTransport());
}

std::shared_ptr<transport::TSocket> SocketRetriever::getSocket(
    std::shared_ptr<protocol::TProtocol> p) {
  return getSocket(p.get());
}


}}}
