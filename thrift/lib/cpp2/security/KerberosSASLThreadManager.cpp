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

#include <thrift/lib/cpp2/security/KerberosSASLThreadManager.h>

#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>


namespace apache { namespace thrift {

using namespace std;
using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::concurrency;

SaslThreadManager::SaslThreadManager() {
  threadManager_ = concurrency::ThreadManager::newSimpleThreadManager(
    256 /* count */);

  threadManager_->threadFactory(
    std::make_shared<concurrency::PosixThreadFactory>(
    concurrency::PosixThreadFactory::kDefaultPolicy,
    concurrency::PosixThreadFactory::kDefaultPriority,
    2 // 2MB stack size, necessary for allowing pthread creation in hphp.
  ));
  threadManager_->setNamePrefix("sasl-thread");
  threadManager_->start();
}

SaslThreadManager::~SaslThreadManager() {
  threadManager_->stop();
}

}} // apache::thrift
