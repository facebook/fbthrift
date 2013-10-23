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
#include <iostream>

#include "thrift/tutorial/cpp/stateful/ServiceAuthState.h"
#include "thrift/tutorial/cpp/stateful/ShellHandler.h"

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/server/TThreadedServer.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/transport/TServerSocket.h"

using std::endl;
using std::cout;
using namespace boost;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;

int main(int argc, char* argv[]) {
  uint16_t port = 12345;

  std::shared_ptr<ServiceAuthState> authState(new ServiceAuthState);

  typedef ShellServiceProcessorFactoryT< TBinaryProtocolT<TBufferBase> >
    ProcessorFactory;

  std::shared_ptr<ShellHandlerFactory> handlerFactory(
      new ShellHandlerFactory(authState));
  std::shared_ptr<ProcessorFactory> processorFactory(
      new ProcessorFactory(handlerFactory));

  std::shared_ptr<TServerSocket> socket(new TServerSocket(port));
  std::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory);
  std::shared_ptr<TProtocolFactory> protocolFactory(
      new TBinaryProtocolFactoryT<TBufferBase>);

  TThreadedServer server(processorFactory, socket, transportFactory,
                         protocolFactory);

  cout << "serving on port " << port << "..." << endl;
  server.serve();
  return 0;
}
