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

#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/Exception.h"
#include "thrift/lib/cpp/concurrency/Monitor.h"
#include "thrift/lib/cpp/concurrency/Thread.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/concurrency/Codel.h"
#include "folly/Conv.h"
#include "ThreadManager.h"
#include "PosixThreadFactory.h"
#include "folly/MPMCQueue.h"
#include "thrift/lib/cpp/async/Request.h"

#include <memory>

#include <assert.h>
#include <queue>
#include <set>
#include <atomic>

#if defined(DEBUG)
#include <iostream>
#endif //defined(DEBUG)

DEFINE_bool(codel_enabled, false, "Enable codel queue timeout algorithm");

namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using apache::thrift::async::RequestContext;

shared_ptr<ThreadManager> ThreadManager::newThreadManager() {
  return make_shared<ThreadManager::Impl>();
}

}}} // apache::thrift::concurrency
