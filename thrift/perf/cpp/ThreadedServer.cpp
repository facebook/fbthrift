/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/perf/cpp/LoadHandler.h>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/server/example/TThreadedServer.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/TServerSocket.h>

#include "common/services/cpp/ServiceFramework.h"

#include "common/config/Flags.h"
#include <iostream>
#include <signal.h>

using namespace boost;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;

DEFINE_bool(enable_service_framework, false,
    "Run ServiceFramework to track service stats");
DEFINE_int32(port, 1234, "server port");
DEFINE_bool(framed, true, "use framed transport");
DEFINE_bool(header, false, "use THeaderProtocol");

DEFINE_int32(send_timeout, 0, "send timeout");
DEFINE_int32(recv_timeout, 0, "receive timeout");
DEFINE_int32(accept_timeout, 0, "accept timeout");
DEFINE_int32(send_buffer, 0, "TCP send buffer");
DEFINE_int32(recv_buffer, 0, "TCP receive buffer");

void setTunables(TServerSocket* socket) {
  if (FLAGS_send_timeout > 0) {
    socket->setSendTimeout(FLAGS_send_timeout);
  }
  if (FLAGS_recv_timeout > 0) {
    socket->setRecvTimeout(FLAGS_recv_timeout);
  }
  if (FLAGS_accept_timeout) {
    socket->setAcceptTimeout(FLAGS_accept_timeout);
  }

  if (FLAGS_send_buffer) {
    socket->setTcpSendBuffer(FLAGS_send_buffer);
  }
  if (FLAGS_recv_buffer) {
    socket->setTcpRecvBuffer(FLAGS_recv_buffer);
  }
}

int main(int argc, char* argv[]) {
  facebook::config::Flags::initFlags(&argc, &argv, true);

  signal(SIGINT, exit);

  if (argc != 1) {
    fprintf(stderr, "error: unhandled arguments:");
    for (int n = 1; n < argc; ++n) {
      fprintf(stderr, " %s", argv[n]);
    }
    fprintf(stderr, "\n");
    return 1;
  }

  // transport and protocol factories
  std::shared_ptr<TTransportFactory> transportFactory;
  if (FLAGS_framed) {
    transportFactory.reset(new TFramedTransportFactory);
  } else {
    transportFactory.reset(new TBufferedTransportFactory);
  }
  std::shared_ptr<TProtocolFactory> protocolFactory;
  std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory;
  std::shared_ptr<TDuplexTransportFactory> duplexTransportFactory;
  if (FLAGS_header) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_UNFRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HTTP_SERVER_TYPE] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
    THeaderProtocolFactory *factory = new THeaderProtocolFactory();
    factory->setClientTypes(clientTypes);
    duplexProtocolFactory = std::shared_ptr<TDuplexProtocolFactory>(factory);
    transportFactory.reset(new TTransportFactory());
    duplexTransportFactory =
      std::shared_ptr<TDuplexTransportFactory>(
        new TSingleTransportFactory<TTransportFactory>(transportFactory));
  } else {
    protocolFactory.reset(new TBinaryProtocolFactoryT<TBufferBase>);
  }

  // server socket
  std::shared_ptr<TServerSocket> serverSocket(new TServerSocket(FLAGS_port));

  // handler and processor
  std::shared_ptr<LoadHandler> handler(new LoadHandler);
  typedef TBinaryProtocolT<TBufferBase> ProtocolType;
  typedef LoadTestProcessorT<ProtocolType> LoadProcessor;
  std::shared_ptr<LoadProcessor> processor(new LoadProcessor(handler));

  // the server itself
  scoped_ptr<TThreadedServer> server;
  if (FLAGS_header) {
    server.reset(new TThreadedServer(processor, serverSocket,
                                     duplexTransportFactory,
                                     duplexProtocolFactory));
  } else {
    server.reset(new TThreadedServer(processor, serverSocket,
                                     transportFactory, protocolFactory));
  }

  // set tunable parameters
  setTunables(serverSocket.get());

  std::unique_ptr<facebook::services::ServiceFramework> fwk;
  if (FLAGS_enable_service_framework) {
    fwk.reset(
        new facebook::services::ServiceFramework("ThreadedServer Load Tester"));
    fwk->go(false /* waitUntilStop */);
  }

  std::cout << "Serving requests on port " << FLAGS_port << "...\n";
  server->serve();

  return 0;
}
