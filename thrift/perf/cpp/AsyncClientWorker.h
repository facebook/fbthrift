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
#ifndef THRIFT_TEST_PERF_ASYNCCLIENTWORKER_H_
#define THRIFT_TEST_PERF_ASYNCCLIENTWORKER_H_ 1

#include "thrift/perf/if/gen-cpp/LoadTest.h"
#include "thrift/lib/cpp/test/loadgen/Worker.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"

namespace apache { namespace thrift {

namespace protocol {
class TProtocol;
}
namespace transport {
class TBufferBase;
}

namespace async {
class TAsyncSocket;
}

namespace test {

class ClientLoadConfig;
class AsyncRunner;
// Before updating for header format LoadTestClientT was specialized on
// TBinaryProtocolT<TBufferBase>, but in practice it didn't seem to
// affect the timing.

class AsyncClientWorker : public loadgen::Worker<
    LoadTestCobClientT<protocol::TProtocol>,
    ClientLoadConfig> {
 public:

  AsyncClientWorker() :
      eb_(),
      binProtoFactory_(),
      duplexProtoFactory_(protocol::T_BINARY_PROTOCOL, -1, true) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;

    duplexProtoFactory_.setClientTypes(clientTypes);
  }

  typedef LoadTestCobClientT<protocol::TProtocol>
    Client;
  typedef loadgen::Worker<Client, ClientLoadConfig> Parent;

  std::shared_ptr<apache::thrift::async::TAsyncSocket> createSocket();

  virtual std::shared_ptr<Client> createConnection() {
    return std::shared_ptr<Client>() ;
  }
  std::shared_ptr<Client> createConnection(
    std::shared_ptr<apache::thrift::async::TAsyncSocket> socket);
  AsyncRunner *createRunner(
    std::shared_ptr<apache::thrift::async::TAsyncSocket> socket);

  // this is now a no-op, AsyncClientWorker::run works differently
  // from Worker::run
  virtual void performOperation(const std::shared_ptr<Client>& client,
                                uint32_t opType) {} ;
  virtual void run();

 private:

  apache::thrift::async::TEventBase eb_;
  protocol::TBinaryProtocolFactory binProtoFactory_;
  protocol::THeaderProtocolFactory duplexProtoFactory_;
  std::list<AsyncRunner *> clients_;
};

}}} // apache::thrift::test

#endif // THRIFT_TEST_PERF_CLIENTWORKER_H_
