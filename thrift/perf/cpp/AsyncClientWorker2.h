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
#ifndef THRIFT_TEST_PERF_ASYNCCLIENTWORKER2_H_
#define THRIFT_TEST_PERF_ASYNCCLIENTWORKER2_H_ 1

#include <thrift/perf/cpp/ClientLoadConfig.h>
#include <thrift/perf/if/gen-cpp2/LoadTest.h>
#include <thrift/lib/cpp/test/loadgen/Worker.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>

#include "servicerouter/client/cpp2/ClientFactory.h"

using apache::thrift::test::ClientLoadConfig;
using apache::thrift::loadgen::Worker;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::THeaderProtocolFactory;

namespace apache { namespace thrift {

// Before updating for header format LoadTestClientT was specialized on
// TBinaryProtocolT<TBufferBase>, but in practice it didn't seem to
// affect the timing.

class AsyncClientWorker2 : public Worker<
    LoadTestAsyncClient,
    ClientLoadConfig> {
 public:

  AsyncClientWorker2() :
      eb_(),
      binProtoFactory_(),
      duplexProtoFactory_(apache::thrift::protocol::T_BINARY_PROTOCOL, true),
      sslContext_(new folly::SSLContext()) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;

    duplexProtoFactory_.setClientTypes(clientTypes);
    setupSSLContext();
  }

  typedef LoadTestAsyncClient Client;
  typedef Worker<Client, ClientLoadConfig> Parent;

  std::shared_ptr<Client> createConnection() override;
  // this is now a no-op, AsyncClientWorker::run works differently
  // from Worker::run
  void performOperation(
      const std::shared_ptr<Client>& /* client */,
      uint32_t /* opType */) override{}
  void run() override;

 private:

  void setupSSLContext();

  folly::EventBase eb_;
  TBinaryProtocolFactory binProtoFactory_;
  THeaderProtocolFactory duplexProtoFactory_;
  std::shared_ptr<folly::SSLContext> sslContext_;

  struct SessionDeleter {
    void operator()(SSL_SESSION* s) {
      SSL_SESSION_free(s);
    }
  };
  std::unique_ptr<SSL_SESSION, SessionDeleter> session_;
};

}} // apache::thrift

#endif // THRIFT_TEST_PERF_CLIENTWORKER_H_
