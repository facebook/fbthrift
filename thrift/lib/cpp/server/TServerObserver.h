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
#ifndef THRIFT_SERVER_TNONBLOCKINGSERVEROBSERVER_H_
#define THRIFT_SERVER_TNONBLOCKINGSERVEROBSERVER_H_ 1

#include <stdint.h>
#include <memory>

namespace apache { namespace thrift { namespace server {

class TServerObserver {
 public:

  virtual ~TServerObserver() {}

  TServerObserver() : sampleRate_(0) {}
  explicit TServerObserver(uint32_t sampleRate) : sampleRate_(sampleRate) {}

  class CallTimestamps {
  public:
    uint64_t readBegin;
    uint64_t readEnd;
    uint64_t processBegin;
    uint64_t processEnd;
    uint64_t writeBegin;
    uint64_t writeEnd;

    CallTimestamps() {
      init();
    }

    void init() {
      readBegin = readEnd = 0;
      processBegin = processEnd = 0;
      writeBegin = writeEnd = 0;
    }

  };

  // Notifications for various events on the TNonblockingServer
  virtual void connAccepted() {}

  virtual void connDropped() {}

  virtual void saslError() {}

  virtual void saslFallBack() {}

  virtual void saslComplete() {}

  virtual void taskKilled() {}

  virtual void taskTimeout() {}

  virtual void serverOverloaded() {}

  virtual void receivedRequest() {}

  virtual void queuedRequests(int32_t numRequests) {}

  virtual void queueTimeout() {}

  virtual void sentReply() {}

  virtual void activeRequests(int32_t numRequests) {}

  virtual void callCompleted(const CallTimestamps& runtimes) {}

  // The observer has to specify a sample rate for callCompleted notifications
  inline uint32_t getSampleRate() const {
    return sampleRate_;
  }

 protected:
  uint32_t sampleRate_;
};

class TServerObserverFactory {
 public:
  virtual std::shared_ptr<TServerObserver> getObserver() = 0;
  virtual ~TServerObserverFactory() {}
};

}}} // apache::thrift::server
#endif
