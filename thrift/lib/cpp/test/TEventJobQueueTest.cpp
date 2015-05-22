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

#include <signal.h>
#include <pthread.h>

#include <thrift/lib/cpp/async/TEventJobQueue.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncTimeout.h>

#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace boost;

using std::cerr;
using std::endl;
using std::list;
using apache::thrift::async::TEventBase;
using apache::thrift::async::TEventJobQueue;
using apache::thrift::async::TEventRunnable;
using apache::thrift::async::TAsyncTimeout;


class SimpleRunnable : public TEventRunnable {
 public:
  SimpleRunnable(TEventBase *origEventBase, int x, int *sum)
      : origEventBase_(origEventBase),
        x_(x),
        sum_(sum) {}
  ~SimpleRunnable() override {}

  void run() override {
    int result = x_ * x_;
    origEventBase_->runInEventBaseThread([this, result] {
        (*sum_) += result;
        if (*sum_ == 264) {
          origEventBase_->terminateLoopSoon();
        }
        delete this;
      });
  }

 private:
  TEventBase *origEventBase_;
  int x_;
  int *sum_;
};

// TODO: Move this to the test/util library
class EventBaseAborter : public TAsyncTimeout {
 public:
  EventBaseAborter(TEventBase* eventBase, uint32_t timeoutMS)
    : TAsyncTimeout(eventBase, TAsyncTimeout::InternalEnum::INTERNAL)
    , eventBase_(eventBase) {
    scheduleTimeout(timeoutMS);
  }

  void timeoutExpired() noexcept override {
    BOOST_FAIL("test timed out");
    eventBase_->terminateLoopSoon();
  }

 private:
  TEventBase* eventBase_;
};

/**
 * Dispatch a list of integers to the queue to be squared and sum the squares
 * in the main thread
 */
BOOST_AUTO_TEST_CASE(SimpleJobQueueTest) {
  TEventBase eventBase;
  EventBaseAborter eba(&eventBase, 1000);
  TEventJobQueue jobQueue(4);
  int data[] = { 8, 6, 7, 5, 3, 0, 9 };
  int sum = 0;

  jobQueue.init();

  for (auto x: data) {
    jobQueue.enqueueJob(new SimpleRunnable(&eventBase, x, &sum));
  }

  eventBase.loopForever();

  jobQueue.shutdown();

  cerr << "SimpleJobQueueTest test completed" << endl;
}

/**
 * Test the numThreads and thread factory options
 */
BOOST_AUTO_TEST_CASE(ArgsJobQueueTest) {
  TEventBase eventBase;
  EventBaseAborter eba(&eventBase, 1000);
  TEventJobQueue jobQueue;
  jobQueue.setNumThreads(4);

  apache::thrift::concurrency::PosixThreadFactory factory;
  factory.setDetached(true);

  int data[] = { 8, 6, 7, 5, 3, 0, 9 };
  int sum = 0;

  jobQueue.init(&factory);

  for (auto x: data) {
    jobQueue.enqueueJob(new SimpleRunnable(&eventBase, x, &sum));
  }

  eventBase.loopForever();

  jobQueue.shutdown();

  cerr << "ArgsJobQueueTest test completed" << endl;
}

/**
 * Catch any race conditions between startup and shutdown.
 */
BOOST_AUTO_TEST_CASE(ShortLivedJobQueueTest) {
  TEventJobQueue jobQueue(4);

  jobQueue.init();

  jobQueue.shutdown();

  cerr << "ShortLivedJobQueueTest test completed" << endl;
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
    "TEventJobQueueTest";
  signal(SIGPIPE, SIG_IGN);

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
