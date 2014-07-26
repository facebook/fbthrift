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

#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache { namespace thrift {

class SaslThreadManager {
 public:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> get() {
    return threadManager_;
  }

  explicit SaslThreadManager(int threadCount = 32, int stackSizeMb = 2);
  ~SaslThreadManager();

 private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
};

}}  // apache::thrift

#endif
