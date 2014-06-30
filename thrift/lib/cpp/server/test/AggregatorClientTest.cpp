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
 * Test aggregation client for TNonblockingServer servers
 * Suppose to send requests to the multiple servers
 * aggregate responses
 */

#include <thrift/lib/cpp/server/test/AggregatorUtilTest.h>

#include "common/client_mgmt/TAsyncClientPool.h"
#include "common/client_mgmt/ThriftAggregatorInterface.h"
#include "common/network/NetworkUtil.h"
#include "common/process/CheckPointProfiler.h"

using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::string;
using std::vector;

using std::shared_ptr;
using boost::lexical_cast;

using namespace apache::thrift::async;
using namespace facebook::common::client_mgmt;
using namespace facebook::network;

using facebook::CheckPointProfiler;

const size_t kNumOfRounds = 10000;
const int kMaxPolicyTimeout = 10000; // 500 msec
const size_t kOneWayCallDelayUs = 100;

void cleanup(int signo) {
  fprintf(stderr, "term on signal %d\n", signo);
  exit(0);
}

static void responseAggregator(
        const vector<shared_ptr<StructResponse> >& responses,
        const vector<pair<int, string> >& errors,
        StructResponse* result) {
  CHECK(result);

  // and concate strings
  for (size_t i = 0 ; i < responses.size() && i < errors.size(); ++i) {
    if (errors[i].first) {
      LOG(ERROR) << "aggregation error for leaf #" << i <<
      " - " << errors[i].second;
      continue;
    }

    addResponse(result, *responses[i].get());
  }
}

static void s_threadFunction(int threadId,
                             int numServers,
                             TAsyncClientPool<AggregatorTestCobClient>& pool,
                             int* stopFlag,
                             int* startFlag,
                             bool oneWayCalls) {

  while (*startFlag == 0) {
    sleep(1);
  }

  LOG(INFO) << "Testing two ways, one argument aggregation...";
  for (size_t rounds = 0; rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<  AggregatorTestCobClient,
                                StructResponse>
            aggregatorSendRecv( pool,
            &AggregatorTestCobClient::recv_sendStructRecvStruct
                              );

    StructRequest request;
    randomData(&request);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::sendStructRecvStruct,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::cref(request));

    // prepare aggregate results
    StructResponse ethalon, aggregated;
    zeroResponse(&aggregated);
    zeroResponse(&ethalon);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::AggrFunc
      aggrCob = std::bind(&responseAggregator,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            &aggregated
                                           );

    aggregatorSendRecv.sendRecvAndAggr(fSend, &aggrCob);

    // prepare ethalon
    for (uint16_t index = 0; index < numServers; ++index) {
      addRequest(&ethalon, request);
    }

    CHECK(equalResult(ethalon, aggregated));
  }

  LOG(INFO) << "Testing one way, one argument aggregation...";
  for (size_t rounds = 0; oneWayCalls && rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<AggregatorTestCobClient>
            aggregatorSendRecv(pool);

    StructRequest request;
    randomData(&request);

    ThriftAggregatorInterface<AggregatorTestCobClient>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::sendStructNoRecv,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::cref(request));

    if (!aggregatorSendRecv.sendRecvAndAggr(fSend)) {
      LOG(ERROR) << "Test failed!";
    }
    usleep(kOneWayCallDelayUs);
  }


  LOG(INFO) << "Testing two ways, multiple arguments aggregation...";
  for (size_t rounds = 0; rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<  AggregatorTestCobClient,
                                StructResponse>
            aggregatorSendRecv( pool,
            &AggregatorTestCobClient::recv_sendMultiParamsRecvStruct
                              );

    StructRequest request;
    randomData(&request);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::SendFunc
    fSend = std::bind(&AggregatorTestCobClient::sendMultiParamsRecvStruct,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              request.i32Val,
                              request.i64Val,
                              request.doubleVal,
                              request.stringVal,
                              std::cref(request));

    // prepare aggregate results
    StructResponse aggregated, ethalon;
    zeroResponse(&aggregated);
    zeroResponse(&ethalon);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::AggrFunc
      aggrCob = std::bind(&responseAggregator,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            &aggregated
                                           );

    if (!aggregatorSendRecv.sendRecvAndAggr(fSend, &aggrCob)) {
      LOG(ERROR) << "Test failed!";
    }

    // prepare ethalon
    for (uint16_t index = 0; index < numServers; ++index) {
      addRequest(&ethalon, request.i32Val,
                              request.i64Val,
                              request.doubleVal,
                              request.stringVal,
                              request);
    }

    CHECK(equalResult(ethalon, aggregated));
  }


  LOG(INFO) << "Testing one way, multiple argument aggregation...";
  for (size_t rounds = 0; oneWayCalls && rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<AggregatorTestCobClient>
            aggregatorSendRecv(pool);

    StructRequest request;
    defaultData(&request);

    ThriftAggregatorInterface<AggregatorTestCobClient>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::sendMultiParamsNoRecv,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              request.i32Val,
                              request.i64Val,
                              request.doubleVal,
                              request.stringVal,
                              std::cref(request));
    if (!aggregatorSendRecv.sendRecvAndAggr(fSend)) {
      LOG(ERROR) << "Test failed!";
    }
    usleep(kOneWayCallDelayUs);
  }

  LOG(INFO) << "Testing two ways, no arguments aggregation...";
  for (size_t rounds = 0; rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<  AggregatorTestCobClient,
                                StructResponse>
            aggregatorSendRecv( pool,
            &AggregatorTestCobClient::recv_noSendRecvStruct
                              );

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::noSendRecvStruct,
                              std::placeholders::_1,
                              std::placeholders::_2
                              );

    // prepare aggregate results
    StructRequest request;
    defaultData(&request);

    StructResponse aggregated, ethalon;
    zeroResponse(&aggregated);
    zeroResponse(&ethalon);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::AggrFunc
      aggrCob = std::bind(&responseAggregator,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            &aggregated
                                           );

    if (!aggregatorSendRecv.sendRecvAndAggr(fSend, &aggrCob)) {
      LOG(ERROR) << "Test failed!";
    }

    // prepare ethalon
    for (uint16_t index = 0; index < numServers; ++index) {
      addRequest(&ethalon, request);
    }

    CHECK(equalResult(ethalon, aggregated));
  }

  LOG(INFO) << "Testing one way, no arguments aggregation...";
  for (size_t rounds = 0; oneWayCalls && rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<AggregatorTestCobClient>
            aggregatorSendRecv(pool);

    ThriftAggregatorInterface<AggregatorTestCobClient>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::noSendNoRecv,
                              std::placeholders::_1,
                              std::placeholders::_2);
    if (!aggregatorSendRecv.sendRecvAndAggr(fSend)) {
      LOG(ERROR) << "Test failed!";
    }
    usleep(kOneWayCallDelayUs);
  }


// ---------------- asynchronous methods ------------
  LOG(INFO) << "Async testing two ways, one argument aggregation...";
  for (size_t rounds = 0; rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<  AggregatorTestCobClient,
                                StructResponse>
            aggregatorSendRecv( pool,
            &AggregatorTestCobClient::recv_sendStructRecvStruct
                              );

    StructRequest request;
    randomData(&request);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::sendStructRecvStruct,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::cref(request));

    // prepare aggregate results
    StructResponse ethalon, aggregated;
    zeroResponse(&aggregated);
    zeroResponse(&ethalon);

    ThriftAggregatorInterface<  AggregatorTestCobClient,
                              StructResponse>::AggrFunc
      aggrCob = std::bind(&responseAggregator,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            &aggregated
                                           );

    aggregatorSendRecv.sendRecvAndAggrAsync(fSend, &aggrCob);

    // pump events
    pool.waitForCompletion();

    // prepare ethalon
    for (uint16_t index = 0; index < numServers; ++index) {
      addRequest(&ethalon, request);
    }

    CHECK(equalResult(ethalon, aggregated));
  }

  LOG(INFO) << "Async testing one way, one argument aggregation...";
  for (size_t rounds = 0; oneWayCalls && rounds < kNumOfRounds; ++rounds) {
    // construct aggregator object
    ThriftAggregatorInterface<AggregatorTestCobClient>
            aggregatorSendRecv(pool);

    StructRequest request;
    randomData(&request);

    ThriftAggregatorInterface<AggregatorTestCobClient>::SendFunc
      fSend = std::bind(&AggregatorTestCobClient::sendStructNoRecv,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::cref(request));

    if (!aggregatorSendRecv.sendRecvAndAggrAsync(fSend)) {
      LOG(ERROR) << "Test failed!";
    }

    // pump events
    pool.waitForCompletion();

    usleep(kOneWayCallDelayUs);
  }


  *stopFlag = 1;
}

int main(int argc, char**argv) {
  if (argc != 6) {
    cerr << "usage: " << argv[0]
         << " {first_port} {#servers} {#threads} {IP} {OneWayCalls}" << endl;
    return 1;
  }

  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  ::srandom(::random() + now.tv_nsec + now.tv_sec + getpid());

  signal(SIGINT, cleanup);

  shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
  shared_ptr<TBinaryProtocolFactory> protocolFactory(
    new TBinaryProtocolFactory());

  // -- use pool to make aggregated calls
  uint16_t first_port = lexical_cast<uint16_t>(argv[1]);
  uint16_t numServers = lexical_cast<uint16_t>(argv[2]);
  uint16_t numThreads = lexical_cast<uint16_t>(argv[3]);
  string IP = argv[4];
  string hostIpPort = IP.empty() ?
                      NetworkUtil::getLocalIPv4() :
                      IP;
  uint16_t oneWayCalls = lexical_cast<uint16_t>(argv[5]);

  // -- initialize client pool from command line parameters
  TEventBaseManager ebm;
  TAsyncClientPool<AggregatorTestCobClient> pool(&ebm);

  Policy* policy = pool.getPolicy();
  policy->setConnTimeout(kMaxPolicyTimeout);
  policy->setSendTimeout(kMaxPolicyTimeout);
  policy->setRecvTimeout(kMaxPolicyTimeout);


  // -- use pool to make aggregated calls
  for (uint16_t index = 0; index < numServers; ++index) {
    // name of group
    string groupName = "Group#" + lexical_cast<string>(index);
    Group* group = pool.getGroup(groupName, true);
    // host IP:port
    string hostIpPort = IP.empty() ?
                      NetworkUtil::getLocalIPv4() :
                      IP;
    hostIpPort += ":";
    hostIpPort += lexical_cast<string>(first_port + index);
    group->createHost(hostIpPort);
  }

  std::vector<std::shared_ptr<Thread> > proccessThreads;
  std::vector<int> completedFlags;
  completedFlags.resize(numThreads, 0);
  std::vector<int> startFlags;
  startFlags.resize(numThreads, 0);

  LOG(INFO) << "Starting threads";
  // - start processing threads
  for (int index = 0; index < numThreads; ++index) {
    std::shared_ptr<FunctionRunner> procRunner(new FunctionRunner(
        std::bind(s_threadFunction, index, numServers, std::ref(pool),
                        &completedFlags[index], &startFlags[index],
                        oneWayCalls != 0)));
    proccessThreads.push_back(threadFactory->newThread(procRunner));
    proccessThreads.back()->start();
  }

  CP_START("", "");

  FOR_EACH(startFlag, startFlags) {
    *startFlag = 1;
  }

  while (true) {
    int completed = 0;
    FOR_EACH(completedFlag, completedFlags) {
      completed += *completedFlag;
    }

    if (completed == numThreads) {
      break;
    }

    sleep(1);
  }

  LOG(INFO) << "Joining threads";


  for (std::vector<std::shared_ptr<Thread> >::iterator
         processingThread = proccessThreads.begin();
         processingThread != proccessThreads.end();
         ++processingThread) {
    (*processingThread)->join();
  }
  proccessThreads.clear();

  return 0;
}
