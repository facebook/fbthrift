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

/**
 * Test server for TNonblockingServer; implements "AggrAsyncTest" service
 * for the following calls:
 *
 * StructResponse sendStructRecvStruct(1:StructRequest request)
 *    -- takes struct and returns the same struct + error code
 *    -- and answerString string.
 *
 * oneway void sendStructNoRecv(1:StructRequest request)
 *    -- sends a struct to a one way server call.
 *
 * StructResponse sendMultiParamsRecvStruct
 *    1:i32 i32Val,
 *    2:i64 i64Val,
 *    3:double doubleVal,
 *    4:string stringVal,
 *    5:StructRequest structVal,
 *  )
 *    -- accepts multiple input parametes of different types, returns struct.
 *
 * oneway void sendMultiParamsRecvStruct
 *    1:i32 i32Val,
 *    2:i64 i64Val,
 *    3:double doubleVal,
 *    4:string stringVal,
 *    5:StructRequest structVal,
 *  )
 *    -- accepts multiple input parametes of different types, one way call.
 *
 * oneway void noSendNoRecv()
 *    -- accepts nothing and returns nothing - one way call.
 *
 */
#include "common/fb303/cpp/FacebookBase.h"

#include <thrift/lib/cpp/server/test/AggregatorUtilTest.h>

#include <thrift/lib/cpp/server/example/TNonblockingServer.h>

#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>

#include <thrift/lib/cpp/transport/TServerSocket.h>
#include <thrift/lib/cpp/transport/TTransportUtils.h>

#include "common/fb303/cpp/TFunctionStatHandler.h"
#include "common/fb303/cpp/TClientInfo.h"


using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;
using std::shared_ptr;
using namespace apache::thrift::server;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;

using namespace facebook::fb303;

class AggregatorTestHandler : virtual public AggregatorTestIf,
       public facebook::fb303::FacebookBase {
  volatile facebook::fb303::fb_status status_;
public:
  AggregatorTestHandler()
    : FacebookBase("NonAsyncTest") {
  }
  facebook::fb303::fb_status getStatus() override { return status_; }

  void sendStructRecvStruct(StructResponse& _return,
                            const StructRequest& request) override {
    _return.request = request;
    _return.errorCode = 0;
    _return.answerString = "";
    toAnswerString(&_return.answerString, request);
  }
  void sendStructNoRecv(const StructRequest& request) override {
    string answerString;
    toAnswerString(&answerString, request);
    T_DEBUG_T("got oneway query %s; calling cob\n", answerString.c_str());
  }
  void sendMultiParamsRecvStruct(StructResponse& _return,
                                 const int32_t i32Val,
                                 const int64_t i64Val,
                                 const double doubleVal,
                                 const string& stringVal,
                                 const StructRequest& structVal) override {
    _return.request = structVal;
    _return.errorCode = 0;
    _return.answerString = "";
    toAnswerString(&_return.answerString, i32Val, i64Val, doubleVal, stringVal,
                    structVal);
  }

  void sendMultiParamsNoRecv(const int32_t i32Val,
                             const int64_t i64Val,
                             const double doubleVal,
                             const string& stringVal,
                             const StructRequest& structVal) override {
    string answerString;
    toAnswerString(&answerString, i32Val, i64Val, doubleVal, stringVal,
                    structVal);
    T_DEBUG_T("got oneway query %s; calling cob\n", answerString.c_str());
  }

  void noSendRecvStruct(StructResponse& _return) override {
    defaultData(&_return.request);
    _return.errorCode = 0;
    _return.answerString = "";
    toAnswerString(&_return.answerString, _return.request);
  }

  void noSendNoRecv() override {
     T_DEBUG_T("got oneway query without params; calling cob\n");
  }
};

void cleanup(int signo) {
  fprintf(stderr, "term on signal %d\n", signo);
  exit(0);
}

std::shared_ptr<TServerEventHandler> serverEventHandler;

void showStats(int) {
  vector<string> stats;
  static_cast<TClientInfoServerHandler*>(
    serverEventHandler.get())->getStatsStrings(stats);
  cout << "----------------------------------------------------------" << endl;
  for (vector<string>::iterator it = stats.begin(); it != stats.end(); ++it) {
    cout << *it << endl;
  }
  cout << "." << endl;
  struct itimerval tick;
  tick.it_interval.tv_sec = tick.it_value.tv_sec = 5;
  tick.it_interval.tv_usec = tick.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &tick, nullptr);
  signal(SIGALRM, showStats);
}

int main(int argc, char**argv) {
  if (argc != 3) {
    cerr << "usage: " << argv[0] << " {port} {#threads}" << endl;
    return 1;
  }

  signal(SIGINT, cleanup);

  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  srandom(random() + now.tv_nsec + now.tv_sec + getpid());

  struct itimerval tick;
  tick.it_interval.tv_sec = tick.it_value.tv_sec = 5;
  tick.it_interval.tv_usec = tick.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &tick, nullptr);
  signal(SIGALRM, showStats);

  shared_ptr<AggregatorTestHandler> handler(new AggregatorTestHandler());
  shared_ptr<TProcessor> processor(new AggregatorTestProcessor(handler));
  shared_ptr<TServiceFunctionStatHandler> eventHandler(
    new TClientInfoCallStatsHandler(handler->getDynamicCounters()));
  processor->addEventHandler(eventHandler);
  shared_ptr<TBinaryProtocolFactory> protocolFactory(
    new TBinaryProtocolFactory());
  shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(lexical_cast<uint16_t>(argv[2]), 0);
  shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);

  threadManager->start();

  TNonblockingServer server(processor, protocolFactory,
          lexical_cast<uint16_t>(argv[1]), threadManager);

  serverEventHandler.reset(new TClientInfoServerHandler);
  server.setServerEventHandler(serverEventHandler);

  protocolFactory->setStrict(true, true);
  std::cout << "About to serve port " << argv[1] << "." << std::endl;

  server.serve();
  return 0;
}
