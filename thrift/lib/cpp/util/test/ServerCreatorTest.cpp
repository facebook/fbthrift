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
#include <boost/test/unit_test.hpp>

#include "thrift/lib/cpp/ClientUtil.h"
#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/util/TEventServerCreator.h"
#include "thrift/lib/cpp/util/TNonblockingServerCreator.h"
#include "thrift/lib/cpp/util/example/TSimpleServerCreator.h"
#include "thrift/lib/cpp/util/TThreadedServerCreator.h"
#include "thrift/lib/cpp/util/example/TThreadPoolServerCreator.h"
#include "thrift/perf/if/gen-cpp/LoadTest.h"
#include "thrift/perf/cpp/AsyncLoadHandler.h"
#include "thrift/perf/cpp/LoadHandler.h"

#include <iostream>

using namespace boost;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::test;
using namespace apache::thrift::util;
using std::string;
using std::cerr;
using std::endl;

///////////////////////////////////////////////////////////////////////////
// Test cases
//////////////////////////////////////////////////////////////////////////

void checkLoadServer(const TSocketAddress* address, bool framed = true) {
  std::shared_ptr<LoadTestClient> client =
    createClientPtr<LoadTestClient>(address, framed);

  string input = "foobar";
  string output;
  client->echo(output, input);

  BOOST_CHECK_EQUAL(output, "foobar");
}

/*
 * Really basic tests to verify that we can start a server and
 * send it a request
 */

template<typename ServerCreatorT, typename HandlerT, typename ProcessorT>
void testServerCreator() {
  std::shared_ptr<HandlerT> handler(new HandlerT);
  // TODO: the generated code should make it possible to automatically create a
  // default processor from a specified handler.  This way we wouldn't have to
  // explicitly create a processor.
  std::shared_ptr<ProcessorT> processor(new ProcessorT(handler));

  // Start the server thread
  ServerCreatorT serverCreator(processor, 0);
  ScopedServerThread st(&serverCreator);

  // Make sure the server is running
  checkLoadServer(st.getAddress());
}

template<typename ServerCreatorT>
void testServerCreator() {
  testServerCreator<ServerCreatorT, LoadHandler, LoadTestProcessor>();
}

BOOST_AUTO_TEST_CASE(SimpleServer) {
  // "Testing TSimpleServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testServerCreator<TSimpleServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(ThreadedServer) {
  testServerCreator<TThreadedServerCreator>();
}

BOOST_AUTO_TEST_CASE(ThreadPoolServer) {
  // "Testing TThreadPoolServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testServerCreator<TThreadPoolServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(NonblockingServer) {
  testServerCreator<TNonblockingServerCreator>();
}

BOOST_AUTO_TEST_CASE(EventServerTaskQueueMode) {
  testServerCreator<TEventServerCreator>();
}

BOOST_AUTO_TEST_CASE(EventServerNativeMode) {
  testServerCreator<TEventServerCreator, AsyncLoadHandler,
                    LoadTestAsyncProcessor>();
}

/*
 * Test server behavior if we can't bind to the requested port
 */

template<typename ServerCreatorT, typename HandlerT, typename ProcessorT>
void testBindFailure() {
  std::shared_ptr<HandlerT> handler(new HandlerT);
  std::shared_ptr<ProcessorT> processor(new ProcessorT(handler));

  // Start a server thread
  ServerCreatorT serverCreator(processor, 0);
  ScopedServerThread st(&serverCreator);

  // Double check that the first server is running
  checkLoadServer(st.getAddress());

  // Try to start a second server listening on the same port
  ServerCreatorT serverCreator2(processor, st.getAddress()->getPort());
  std::shared_ptr<TServer> server = serverCreator2.createServer();

  // Call serve() and verify that it throws an exception
  // TODO: We should set some sort of timeout, so that if an error occurs and
  // the server does manage to start, we won't block forever.
  try {
    server->serve();
    BOOST_ERROR("we expected bind() to fail, but the server returned "
                "successfully from serve()");
  } catch (const TTransportException& ex) {
    // Serve should throw a TTransportException
    BOOST_CHECK_EQUAL(ex.getType(), TTransportException::COULD_NOT_BIND);
  }
}

template<typename ServerCreatorT>
void testBindFailure() {
  testBindFailure<ServerCreatorT, LoadHandler, LoadTestProcessor>();
}

BOOST_AUTO_TEST_CASE(SimpleServerBindFailure) {
  // "Testing TSimpleServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testBindFailure<TSimpleServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(ThreadedServerBindFailure) {
  testBindFailure<TThreadedServerCreator>();
}

BOOST_AUTO_TEST_CASE(ThreadPoolServerBindFailure) {
  // "TestingTThreadPoolServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testBindFailure<TThreadPoolServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(NonblockingServerBindFailure) {
  testBindFailure<TNonblockingServerCreator>();
}

BOOST_AUTO_TEST_CASE(EventServerBindFailure) {
  testBindFailure<TEventServerCreator, AsyncLoadHandler,
                  LoadTestAsyncProcessor>();
}

/*
 * Make sure ScopedServerThread raises an exception in the original thread
 * if it fails to start the server.
 */

template<typename ServerCreatorT, typename HandlerT, typename ProcessorT>
void testThreadedBindFailure() {
  std::shared_ptr<HandlerT> handler(new HandlerT);
  std::shared_ptr<ProcessorT> processor(new ProcessorT(handler));

  // Start a server thread
  ServerCreatorT serverCreator(processor, 0);
  ScopedServerThread st(&serverCreator);

  // Double check that the first server is running
  checkLoadServer(st.getAddress());

  // Try to start a second server listening on the same port
  ScopedServerThread st2;
  try {
    ServerCreatorT serverCreator2(processor, st.getAddress()->getPort());
    st2.start(&serverCreator2);
    BOOST_ERROR("we expected bind() to fail, but the server thread started "
                "successfully");
  } catch (const TTransportException& ex) {
    // Serve should throw a TTransportException
    BOOST_CHECK_EQUAL(ex.getType(), TTransportException::COULD_NOT_BIND);
  }
}

template<typename ServerCreatorT>
void testThreadedBindFailure() {
  return testThreadedBindFailure<ServerCreatorT, LoadHandler,
                                 LoadTestProcessor>();
}

BOOST_AUTO_TEST_CASE(SimpleServerThreadedBindFailure) {
  // "Testing TSimpleServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testThreadedBindFailure<TSimpleServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(ThreadedServerThreadedBindFailure) {
  testThreadedBindFailure<TThreadedServerCreator>();
}

BOOST_AUTO_TEST_CASE(ThreadPoolServerThreadedBindFailure) {
  // "Testing TThreadPoolServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  testThreadedBindFailure<TThreadPoolServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(NonblockingServerThreadedBindFailure) {
  testThreadedBindFailure<TNonblockingServerCreator>();
}

BOOST_AUTO_TEST_CASE(EventServerThreadedBindFailure) {
  testThreadedBindFailure<TEventServerCreator, AsyncLoadHandler,
                          LoadTestAsyncProcessor>();
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "ServerCreatorTest";

  if (argc != 1) {
    cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      cerr << " " << argv[n];
    }
    cerr << endl;
    exit(1);
  }

  return nullptr;
}
