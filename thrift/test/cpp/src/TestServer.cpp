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
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"
#include "thrift/lib/cpp/server/example/TSimpleServer.h"
#include "thrift/lib/cpp/server/TThreadedServer.h"
#include "thrift/lib/cpp/server/example/TThreadPoolServer.h"
#include "thrift/lib/cpp/server/TNonblockingServer.h"
#include "thrift/lib/cpp/transport/TServerSocket.h"
#include "thrift/lib/cpp/transport/TTransportUtils.h"
#include "thrift/lib/cpp/transport/TSSLSocket.h"
#include "thrift/lib/cpp/transport/TSSLServerSocket.h"
#include "thrift/test/gen-cpp/ThriftTest.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <inttypes.h>
#include <signal.h>

using std::string;
using std::map;
using std::set;
using std::vector;
using std::make_pair;
using std::endl;
using std::cerr;
using std::ostringstream;
using std::invalid_argument;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using namespace thrift::test;

class TestHandler : public ThriftTestIf {
 public:
  TestHandler() {}

  void testVoid() {
    printf("testVoid()\n");
  }

  void testString(string& out, const string &thing) {
    printf("testString(\"%s\")\n", thing.c_str());
    out = thing;
  }

  int8_t testByte(const int8_t thing) {
    printf("testByte(%d)\n", (int)thing);
    return thing;
  }

  int32_t testI32(const int32_t thing) {
    printf("testI32(%d)\n", thing);
    return thing;
  }

  int64_t testI64(const int64_t thing) {
    printf("testI64(%" PRId64 ")\n", thing);
    return thing;
  }

  double testDouble(const double thing) {
    printf("testDouble(%lf)\n", thing);
    return thing;
  }

  float testFloat(const float thing) {
    printf("testFloat(%f)\n", thing);
    return thing;
  }

  void testStruct(Xtruct& out, const Xtruct &thing) {
    printf("testStruct({\"%s\", %d, %d, %" PRId64 "})\n",
           thing.string_thing.c_str(), (int)thing.byte_thing,
           thing.i32_thing, thing.i64_thing);
    out = thing;
  }

  void testNest(Xtruct2& out, const Xtruct2& nest) {
    const Xtruct &thing = nest.struct_thing;
    printf("testNest({%d, {\"%s\", %d, %d, %" PRId64 "}, %d})\n",
           (int)nest.byte_thing, thing.string_thing.c_str(),
           (int)thing.byte_thing, thing.i32_thing,
           thing.i64_thing, nest.i32_thing);
    out = nest;
  }

  void testMap(map<int32_t, int32_t> &out, const map<int32_t, int32_t> &thing) {
    printf("testMap({");
    map<int32_t, int32_t>::const_iterator m_iter;
    bool first = true;
    for (m_iter = thing.begin(); m_iter != thing.end(); ++m_iter) {
      if (first) {
        first = false;
      } else {
        printf(", ");
      }
      printf("%d => %d", m_iter->first, m_iter->second);
    }
    printf("})\n");
    out = thing;
  }

  void testSet(set<int32_t> &out, const set<int32_t> &thing) {
    printf("testSet({");
    set<int32_t>::const_iterator s_iter;
    bool first = true;
    for (s_iter = thing.begin(); s_iter != thing.end(); ++s_iter) {
      if (first) {
        first = false;
      } else {
        printf(", ");
      }
      printf("%d", *s_iter);
    }
    printf("})\n");
    out = thing;
  }

  void testList(vector<int32_t> &out, const vector<int32_t> &thing) {
    printf("testList({");
    vector<int32_t>::const_iterator l_iter;
    bool first = true;
    for (l_iter = thing.begin(); l_iter != thing.end(); ++l_iter) {
      if (first) {
        first = false;
      } else {
        printf(", ");
      }
      printf("%d", *l_iter);
    }
    printf("})\n");
    out = thing;
  }

  Numberz testEnum(const Numberz thing) {
    printf("testEnum(%d)\n", thing);
    return thing;
  }

  UserId testTypedef(const UserId thing) {
    printf("testTypedef(%" PRId64 ")\n", thing);
    return thing;
  }

  int32_t testRequestCount() {
    printf("testRequestCount not implemented\n");
    return 0;
  }

  int32_t testPreServe() {
    printf("testPreServe not implemented\n");
    return 0;
  }

  int32_t testNewConnection() {
    printf("testNewConnection not implemented\n");
    return 0;
  }

  int32_t testConnectionDestroyed() {
    printf("testConnectionDestroyed not implemented\n");
    return 0;
  }

  void testMapMap(map<int32_t, map<int32_t,int32_t> > &mapmap,
                  const int32_t hello) {
    printf("testMapMap(%d)\n", hello);

    map<int32_t,int32_t> pos;
    map<int32_t,int32_t> neg;
    for (int i = 1; i < 5; i++) {
      pos.insert(make_pair(i,i));
      neg.insert(make_pair(-i,-i));
    }

    mapmap.insert(make_pair(4, pos));
    mapmap.insert(make_pair(-4, neg));

  }

  void testInsanity(map<UserId, map<Numberz,Insanity> > &insane,
                    const Insanity &argument) {
    printf("testInsanity()\n");

    Xtruct hello;
    hello.string_thing = "Hello2";
    hello.byte_thing = 2;
    hello.i32_thing = 2;
    hello.i64_thing = 2;

    Xtruct goodbye;
    goodbye.string_thing = "Goodbye4";
    goodbye.byte_thing = 4;
    goodbye.i32_thing = 4;
    goodbye.i64_thing = 4;

    Insanity crazy;
    crazy.userMap.insert(make_pair(EIGHT, 8));
    crazy.xtructs.push_back(goodbye);

    Insanity looney;
    crazy.userMap.insert(make_pair(FIVE, 5));
    crazy.xtructs.push_back(hello);

    map<Numberz, Insanity> first_map;
    map<Numberz, Insanity> second_map;

    first_map.insert(make_pair(TWO, crazy));
    first_map.insert(make_pair(THREE, crazy));

    second_map.insert(make_pair(SIX, looney));

    insane.insert(make_pair(1, first_map));
    insane.insert(make_pair(2, second_map));

    printf("return");
    printf(" = {");
    map<UserId, map<Numberz,Insanity> >::const_iterator i_iter;
    for (i_iter = insane.begin(); i_iter != insane.end(); ++i_iter) {
      printf("%" PRId64 " => {", i_iter->first);
      map<Numberz,Insanity>::const_iterator i2_iter;
      for (i2_iter = i_iter->second.begin();
           i2_iter != i_iter->second.end();
           ++i2_iter) {
        printf("%d => {", i2_iter->first);
        map<Numberz, UserId> userMap = i2_iter->second.userMap;
        map<Numberz, UserId>::const_iterator um;
        printf("{");
        for (um = userMap.begin(); um != userMap.end(); ++um) {
          printf("%d => %" PRId64 ", ", um->first, um->second);
        }
        printf("}, ");

        vector<Xtruct> xtructs = i2_iter->second.xtructs;
        vector<Xtruct>::const_iterator x;
        printf("{");
        for (x = xtructs.begin(); x != xtructs.end(); ++x) {
          printf("{\"%s\", %d, %d, %" PRId64 "}, ", x->string_thing.c_str(),
                 (int)x->byte_thing, x->i32_thing, x->i64_thing);
        }
        printf("}");

        printf("}, ");
      }
      printf("}, ");
    }
    printf("}\n");


  }

  void testMulti(Xtruct &hello, const int8_t arg0, const int32_t arg1,
                 const int64_t arg2,
                 const std::map<int16_t, std::string>  &arg3,
                 const Numberz arg4, const UserId arg5) {
    printf("testMulti()\n");

    hello.string_thing = "Hello2";
    hello.byte_thing = arg0;
    hello.i32_thing = arg1;
    hello.i64_thing = (int64_t)arg2;
  }

  void testException(const std::string &arg) {
    printf("testException(%s)\n", arg.c_str());
    if (arg.compare("Xception") == 0) {
      Xception e;
      e.errorCode = 1001;
      e.message = arg;
      throw e;
    } else if (arg.compare("ApplicationException") == 0) {
      apache::thrift::TLibraryException e;
      throw e;
    } else {
      Xtruct result;
      result.string_thing = arg;
      return;
    }
  }

  void testMultiException(Xtruct &result,
                          const std::string &arg0,
                          const std::string &arg1) {

    printf("testMultiException(%s, %s)\n", arg0.c_str(), arg1.c_str());

    if (arg0.compare("Xception") == 0) {
      Xception e;
      e.errorCode = 1001;
      e.message = "This is an Xception";
      throw e;
    } else if (arg0.compare("Xception2") == 0) {
      Xception2 e;
      e.errorCode = 2002;
      e.struct_thing.string_thing = "This is an Xception2";
      throw e;
    } else {
      result.string_thing = arg1;
      return;
    }
  }

  void testOneway(int sleepFor) {
    printf("testOneway(%d): Sleeping...\n", sleepFor);
    sleep(sleepFor);
    printf("testOneway(%d): done sleeping!\n", sleepFor);
  }
};


class TestProcessorEventHandler : public TProcessorEventHandler {
  virtual void* getContext(const char* fn_name,
                           TConnectionContext* serverContext) {
    return new std::string(fn_name);
  }
  virtual void freeContext(void* ctx, const char* fn_name) {
    delete static_cast<std::string*>(ctx);
  }
  virtual void preRead(void* ctx, const char* fn_name) {
    communicate("preRead", ctx, fn_name);
  }
  virtual void postRead(void* ctx, const char* fn_name, uint32_t bytes) {
    communicate("postRead", ctx, fn_name);
  }
  virtual void preWrite(void* ctx, const char* fn_name) {
    communicate("preWrite", ctx, fn_name);
  }
  virtual void postWrite(void* ctx, const char* fn_name, uint32_t bytes) {
    communicate("postWrite", ctx, fn_name);
  }
  virtual void asyncComplete(void* ctx, const char* fn_name) {
    communicate("asyncComplete", ctx, fn_name);
  }
  virtual void handlerError(void* ctx, const char* fn_name) {
    communicate("handlerError", ctx, fn_name);
  }

  void communicate(const char* event, void* ctx, const char* fn_name) {
    std::cout << event << ": " << *static_cast<std::string*>(ctx)
              << " = " << fn_name << std::endl;
  }
};


int main(int argc, char **argv) {

  int port = 9090;
  string serverType = "simple";
  string protocolType = "binary";
  size_t workerCount = 4;
  bool   ssl = false;
  bool header = false;

  ostringstream usage;

  usage <<
    argv[0] << " [--port=<port number>] [--server-type=<server-type>] " <<
    "[--protocol-type=<protocol-type>] [--workers=<worker-count>] " <<
    "[--processor-events]" << endl <<
    "\t\tserver-type\t\ttype of server, \"simple\", \"thread-pool\", " <<
    "\"threaded\", or \"nonblocking\".  Default is " << serverType << endl <<
    "\t\tprotocol-type\t\ttype of protocol, \"binary\", \"header\", " <<
    "\"ascii\", or \"xml\".  Default is " << protocolType << endl <<
    "\t\tworkers\t\tNumber of thread pools workers.  Only valid for " <<
    "thread-pool server type.  Default is " << workerCount << endl;

  map<string, string>  args;

  for (int ix = 1; ix < argc; ix++) {
    string arg(argv[ix]);
    if (arg.compare(0,2, "--") == 0) {
      size_t end = arg.find_first_of("=", 2);
      if (end != string::npos) {
        args[string(arg, 2, end - 2)] = string(arg, end + 1);
      } else {
        args[string(arg, 2)] = "true";
      }
    } else {
      throw invalid_argument("Unexcepted command line token: "+arg);
    }
  }

  try {

    if (!args["port"].empty()) {
      port = atoi(args["port"].c_str());
    }

    if (!args["server-type"].empty()) {
      serverType = args["server-type"];
      if (serverType == "simple") {
      } else if (serverType == "thread-pool") {
      } else if (serverType == "threaded") {
      } else if (serverType == "nonblocking") {
      } else {
        throw invalid_argument("Unknown server type "+serverType);
      }
    }

    if (!args["protocol-type"].empty()) {
      protocolType = args["protocol-type"];
      if (protocolType == "binary") {
      } else if (protocolType == "header") {
        header = true;
      } else if (protocolType == "ascii") {
        throw invalid_argument("ASCII protocol not supported");
      } else if (protocolType == "xml") {
        throw invalid_argument("XML protocol not supported");
      } else {
        throw invalid_argument("Unknown protocol type "+protocolType);
      }
    }

    if (!args["workers"].empty()) {
      workerCount = atoi(args["workers"].c_str());
    }
  } catch (std::exception& e) {
    cerr << e.what() << endl;
    cerr << usage;
  }

    // OpenSSL may trigger SIGPIPE when remote sends connection reset.
    if (args["ssl"] == "true") {
      ssl = true;
      signal(SIGPIPE, SIG_IGN);
    }

  // Dispatcher
  std::shared_ptr<TProtocolFactory> protocolFactory;
  std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory;
  if (header) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_UNFRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HTTP_CLIENT_TYPE] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
    THeaderProtocolFactory* factory = new THeaderProtocolFactory();
    factory->setClientTypes(clientTypes);
    duplexProtocolFactory = std::shared_ptr<TDuplexProtocolFactory>(factory);
  } else {
    protocolFactory = std::shared_ptr<TProtocolFactory>(
      new TBinaryProtocolFactoryT<TBufferBase>());
  }

  std::shared_ptr<TestHandler> testHandler(new TestHandler());

  std::shared_ptr<TProcessor> testProcessor(
      new ThriftTestProcessor(testHandler));


  if (!args["processor-events"].empty()) {
    testProcessor->setEventHandler(std::shared_ptr<TProcessorEventHandler>(
          new TestProcessorEventHandler()));
  }

  // Transport
  std::shared_ptr<TServerSocket> serverSocket;

  if (ssl) {
    std::shared_ptr<SSLContext> sslContext(new SSLContext());
    sslContext->loadCertificate("./server-certificate.pem");
    sslContext->loadPrivateKey("./server-private-key.pem");
    sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    std::shared_ptr<TSSLSocketFactory> sslSocketFactory(
      new TSSLSocketFactory(sslContext));
    serverSocket = std::shared_ptr<TServerSocket>(
      new TSSLServerSocket(port, sslSocketFactory));
  } else {
    serverSocket = std::shared_ptr<TServerSocket>(new TServerSocket(port));
  }

  // Factory
  std::shared_ptr<TTransportFactory> transportFactory(
    new TBufferedTransportFactory());
  std::shared_ptr<TDuplexTransportFactory> duplexTransportFactory(
    new TSingleTransportFactory<TTransportFactory>(transportFactory));
  std::shared_ptr<TServer> server;

  if (serverType == "simple") {

    // Server
    FBCODE_DISABLE_DEPRECATED_WARNING("Testing TSimpleServer")
    server = std::shared_ptr<TServer>(new TSimpleServer(testProcessor,
                                                   serverSocket,
                                                   transportFactory,
                                                   protocolFactory));
    FBCODE_RESTORE_DEPRECATED_WARNING()

    printf("Starting the server on port %d...\n", port);

  } else if (serverType == "thread-pool") {

    std::shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(workerCount);

    std::shared_ptr<PosixThreadFactory> threadFactory =
      std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());

    threadManager->threadFactory(threadFactory);

    threadManager->start();

    FBCODE_DISABLE_DEPRECATED_WARNING("Testing TThreadPoolServer")
    server = std::shared_ptr<TServer>(new TThreadPoolServer(testProcessor,
                                                       serverSocket,
                                                       transportFactory,
                                                       protocolFactory,
                                                       threadManager));
    FBCODE_RESTORE_DEPRECATED_WARNING()

    printf("Starting the server on port %d...\n", port);

  } else if (serverType == "threaded") {

    server = std::shared_ptr<TServer>(new TThreadedServer(testProcessor,
                                                     serverSocket,
                                                     transportFactory,
                                                     protocolFactory));

    printf("Starting the server on port %d...\n", port);

  } else if (serverType == "nonblocking") {
    server = std::shared_ptr<TServer>(new TNonblockingServer(testProcessor,
                                                        protocolFactory, port));

    printf("Starting the nonblocking server on port %d...\n", port);
  }

  if (header) {
    server->setDuplexTransportFactory(duplexTransportFactory);
    server->setDuplexProtocolFactory(duplexProtocolFactory);
  }

  server->serve();

  printf("done.\n");
  return 0;
}
