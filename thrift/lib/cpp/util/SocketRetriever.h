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

#ifndef THRIFT_SOCKETRETRIEVER_H_
#define THRIFT_SOCKETRETRIEVER_H_

#include "thrift/lib/cpp/protocol/TProtocol.h"
#include "thrift/lib/cpp/transport/TSocket.h"

namespace apache { namespace thrift { namespace util {


class SocketRetriever {
public:
  static std::shared_ptr<transport::TSocket> getSocket(
      std::shared_ptr<transport::TTransport> t);

  static std::shared_ptr<transport::TSocket> getSocket(
      protocol::TProtocol *p);
  static std::shared_ptr<transport::TSocket> getSocket(
      std::shared_ptr<protocol::TProtocol> p);

  template <class Client_>
  static std::shared_ptr<transport::TSocket> getSocket(
      Client_ *client) {
    return client ?
       getSocket(client->getOutputProtocol()) :
       std::shared_ptr<transport::TSocket>();
  }

  template <class Client_>
  static std::shared_ptr<transport::TSocket> getSocket(
      std::shared_ptr<Client_> client) {
    return getSocket(client.get());
  }

private:
  static std::shared_ptr<transport::TSocket> castToSocket(
      std::shared_ptr<transport::TTransport> t);
};


}}}

#endif
