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
#include <thrift/lib/cpp/test/ScopedEventBaseThread.h>

#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>

using apache::thrift::async::TEventBase;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::Runnable;

namespace {

class EventBaseRunner : public Runnable {
 public:
  explicit EventBaseRunner(TEventBase *eventBase) : eventBase_(eventBase) {}

  virtual void run() {
    eventBase_->loopForever();
  }
 private:
  TEventBase *eventBase_;
};

} // unnamed namespace

namespace apache { namespace thrift { namespace test {

ScopedEventBaseThread::ScopedEventBaseThread()
  : eventBase_(new TEventBase()),
    thread_() {
  std::shared_ptr<EventBaseRunner> runner(
      new EventBaseRunner(eventBase_.get()));
  PosixThreadFactory threadFactory;
  threadFactory.setDetached(false);
  thread_ = threadFactory.newThread(runner);
  thread_->start();
}

ScopedEventBaseThread::~ScopedEventBaseThread() {
  eventBase_->terminateLoopSoon();
  thread_->join();
}

ScopedEventBaseThread::ScopedEventBaseThread(ScopedEventBaseThread&& other)
  : eventBase_(std::move(other.eventBase_)),
    thread_(std::move(other.thread_)) {
  other.thread_.reset();
}

ScopedEventBaseThread& ScopedEventBaseThread::operator=(
    ScopedEventBaseThread&& other) {
  swap(eventBase_, other.eventBase_);
  swap(thread_, other.thread_);
  return *this;
}

}}} // apache::thrift::test
