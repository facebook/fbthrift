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
#include <thrift/perf/cpp/AsyncLoadHandler.h>

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <unistd.h>

using folly::EventBase;
using apache::thrift::concurrency::Util;

namespace apache { namespace thrift { namespace test {

class AsyncLoadHandler::Burner {
 public:
  Burner(AsyncLoadHandler* handler, VoidCob cob, int64_t microseconds)
    : handler_(handler)
    , timeLeft_(microseconds)
    , rerunCob_(std::bind(&Burner::run, this))
    , finishedCob_(cob) {}

  void run() {
    // Compute how long we should burn in this invocation of run()
    uint64_t burnTime = handler_->getBurnIntervalUsec();
    if (timeLeft_ < burnTime) {
      burnTime = timeLeft_;
    }

    int64_t start = Util::currentTimeUsec();
    int64_t end = start + burnTime;

    int64_t now;
    do {
      now = Util::currentTimeUsec();
    } while (now < end);

    int64_t timeBurned = now - start;
    if (timeBurned > timeLeft_) {
      // all done
      finishedCob_();
      delete this;
    } else {
      // still need to burn more
      timeLeft_ -= timeBurned;
      // reschedule run() the next time around the loop
      handler_->getServer()->getEventBase()->runInLoop(rerunCob_);
    }
  }

 private:
  AsyncLoadHandler* handler_;
  int64_t timeLeft_;
  // store rerunCob_ as a member variable so we don't
  // have to re-bind each time run() is invoked
  VoidCob rerunCob_;
  VoidCob finishedCob_;
};

void AsyncLoadHandler::noop(VoidCob cob) {
  cob();
}

void AsyncLoadHandler::onewayNoop(VoidCob cob) {
  cob();
}

void AsyncLoadHandler::asyncNoop(VoidCob cob) {
  server_->getEventBase()->runInLoop(cob);
}

void AsyncLoadHandler::sleep(VoidCob cob, const int64_t microseconds) {
  // We only have millisecond resolution for EventBase timeouts
  server_->getEventBase()->tryRunAfterDelay(cob, microseconds / Util::US_PER_MS);
}

void AsyncLoadHandler::onewaySleep(VoidCob cob, const int64_t microseconds) {
  // We only have millisecond resolution for EventBase timeouts
  server_->getEventBase()->tryRunAfterDelay(cob, microseconds / Util::US_PER_MS);
}

void AsyncLoadHandler::burn(VoidCob cob, const int64_t microseconds) {
  Burner* burner = new Burner(this, cob, microseconds);
  burner->run();
}

void AsyncLoadHandler::onewayBurn(VoidCob cob, const int64_t microseconds) {
  Burner* burner = new Burner(this, cob, microseconds);
  burner->run();
}

void AsyncLoadHandler::badSleep(VoidCob cob, const int64_t microseconds) {
  usleep(microseconds);
  cob();
}

void AsyncLoadHandler::badBurn(VoidCob cob, const int64_t microseconds) {
  int64_t end = Util::currentTimeUsec() + microseconds;
  while (Util::currentTimeUsec() < end) {
  }
  cob();
}

void AsyncLoadHandler::throwError(VoidCob cob, ErrorCob exn_cob,
                                  const int32_t code) {
  LoadError error;
  error.code = code;
  exn_cob(error);
}

void AsyncLoadHandler::throwUnexpected(VoidCob cob, const int32_t code) {
  // FIXME: it isn't possible to implement this behavior with the async code
  //
  // Actually throwing an exception from the handler is bad, and EventBase
  // should probably be changed to fatal the entire program if that happens.
  cob();
}

void AsyncLoadHandler::onewayThrow(VoidCob cob, const int32_t code) {
  // FIXME: it isn't possible to implement this behavior with the async code
  //
  // Actually throwing an exception from the handler is bad, and EventBase
  // should probably be changed to fatal the entire program if that happens.
  cob();
}

void AsyncLoadHandler::send(VoidCob cob, const std::string& data) {
  cob();
}

void AsyncLoadHandler::onewaySend(VoidCob cob, const std::string& data) {
  cob();
}

void AsyncLoadHandler::recv(StringCob cob, const int64_t bytes) {
  std::string ret(bytes, 'a');
  cob(ret);
}

void AsyncLoadHandler::sendrecv(StringCob cob,
                                const std::string& data,
                                const int64_t recvBytes) {
  std::string ret(recvBytes, 'a');
  cob(ret);
}

void AsyncLoadHandler::echo(StringCob cob, const std::string& data) {
  cob(data);
}

void AsyncLoadHandler::add(I64Cob cob, const int64_t a, const int64_t b) {
  cob(a + b);
}

}}} // apache::thrift::test
