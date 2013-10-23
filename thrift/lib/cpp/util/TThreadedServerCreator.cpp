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
#include "thrift/lib/cpp/util/TThreadedServerCreator.h"

#include "thrift/lib/cpp/server/TThreadedServer.h"
#include "thrift/lib/cpp/transport/TServerSocket.h"

using std::shared_ptr;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;

namespace apache { namespace thrift { namespace util {

shared_ptr<TServer> TThreadedServerCreator::createServer() {
  return createThreadedServer();
}

shared_ptr<TThreadedServer> TThreadedServerCreator::createThreadedServer() {
  shared_ptr<TThreadedServer> server;
  if (duplexProtocolFactory_.get() != nullptr) {
    server.reset(new TThreadedServer(processor_,
                                     createServerSocket(),
                                     getDuplexTransportFactory(),
                                     duplexProtocolFactory_,
                                     threadFactory_));
  } else {
    server.reset(new TThreadedServer(processor_,
                                     createServerSocket(),
                                     transportFactory_,
                                     getProtocolFactory(),
                                     threadFactory_));
  }
  SyncServerCreator::configureServer(server);
  return server;
}

}}} // apache::thrift::util
