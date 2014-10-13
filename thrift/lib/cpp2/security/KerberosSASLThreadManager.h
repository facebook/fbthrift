/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KERBEROS_SASL_THREAD_MANAGER_H
#define KERBEROS_SASL_THREAD_MANAGER_H

#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

#include <deque>

namespace apache { namespace thrift {

class SecurityLogger;

class SaslThreadManager {
 public:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> get() {
    return threadManager_;
  }

  explicit SaslThreadManager(std::shared_ptr<SecurityLogger> logger,
                             int threadCount = 32, int stackSizeMb = 2);
  ~SaslThreadManager();

  // start() should be used when starting a new SASL negotiation.  It
  // will queue the argument for later if there's too many connection
  // setups already in progress.
  void start(std::shared_ptr<concurrency::FunctionRunner>&& f);

  // end() calls should be paired with start() calls.  When a SASL
  // connection setup completes (success or failure), this will start
  // another if any have been queued.
  void end();

 private:
  std::shared_ptr<SecurityLogger> logger_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  apache::thrift::concurrency::Mutex mutex_;
  unsigned int secureConnectionsInProgress_;
  unsigned int maxSimultaneousSecureConnections_;
  std::deque<std::shared_ptr<apache::thrift::concurrency::FunctionRunner>>
    pendingSecureStarts_;
};

}}  // apache::thrift

#endif
