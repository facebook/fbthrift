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
#define __STDC_FORMAT_MACROS

#include "thrift/tutorial/cpp/stateful/gen-cpp/AuthenticatedService.h"

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/transport/TSocket.h"

using std::string;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace boost;

int main() {
  string host = "127.0.0.1";
  uint16_t port = 12345;

  std::shared_ptr<TSocket> socket(new TSocket(host, port));
  std::shared_ptr<TBufferBase> transport(new TFramedTransport(socket));
  std::shared_ptr< TBinaryProtocolT<TBufferBase> > protocol(
      new TBinaryProtocolT<TBufferBase>(transport));
  transport->open();
  AuthenticatedServiceClient client(protocol);

  SessionInfoList sessions;
  client.listSessions(sessions);

  printf("%8s %-20s %-40s %s\n", "ID", "Login Time", "Client", "Username");
  for (SessionInfoList::const_iterator it = sessions.begin();
       it != sessions.end();
       ++it) {
    struct tm localTime;
    localtime_r(&it->openTime, &localTime);
    char timeBuf[128];
    strftime(timeBuf, sizeof(timeBuf), "%F %T", &localTime);

    printf("%8" PRId64 " %-20s %-40s %s\n",
           it->id, timeBuf, it->clientInfo.c_str(), it->username.c_str());
  }

  return 0;
}
