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
#ifndef MOCKTIMEOUTMANAGER_H
#define MOCKTIMEOUTMANAGER_H

#include "thrift/lib/cpp/async/TimeoutManager.h"

#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <glog/logging.h>

namespace apache {
namespace thrift {
namespace test {

class MockTimeoutManager : public async::TimeoutManager {
 public:

  MOCK_METHOD2(
      attachTimeoutManager,
      void(async::TAsyncTimeout*, async::TimeoutManager::InternalEnum));

  MOCK_METHOD1(detachTimeoutManager, void(async::TAsyncTimeout*));

  MOCK_METHOD2(
      scheduleTimeout,
      bool(async::TAsyncTimeout*, std::chrono::milliseconds));

  MOCK_METHOD1(cancelTimeout, void(async::TAsyncTimeout*));

  MOCK_METHOD0(bumpHandlingTime, bool());
  MOCK_METHOD0(isInTimeoutManagerThread, bool());
};

}}} // apache::thrift::test
#endif
