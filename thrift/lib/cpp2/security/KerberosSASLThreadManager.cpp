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
#include <thrift/lib/cpp2/security/SecurityLogger.h>


namespace apache { namespace thrift {

using namespace std;
using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::concurrency;

DEFINE_int32(max_connections_per_sasl_thread, -1,
             "Cap on the number of simultaneous secure connection setups "
             "per sasl thread. -1 is no cap.");

const int kPendingFailureRatio = 1000;

SaslThreadManager::SaslThreadManager(std::shared_ptr<SecurityLogger> logger,
                                     int threadCount, int stackSizeMb)
  : logger_(logger) {
  secureConnectionsInProgress_ = 0;
  if (FLAGS_max_connections_per_sasl_thread > 0) {
    maxSimultaneousSecureConnections_ =
      threadCount * FLAGS_max_connections_per_sasl_thread;
  } else {
    maxSimultaneousSecureConnections_ = -1;
  }
  threadManager_ = concurrency::ThreadManager::newSimpleThreadManager(
    threadCount);

  threadManager_->threadFactory(
    std::make_shared<concurrency::PosixThreadFactory>(
    concurrency::PosixThreadFactory::kDefaultPolicy,
    concurrency::PosixThreadFactory::kDefaultPriority,
    stackSizeMb
  ));
  threadManager_->setNamePrefix("sasl-thread");
  threadManager_->start();
}

SaslThreadManager::~SaslThreadManager() {
  threadManager_->stop();
}

void SaslThreadManager::start(
  std::shared_ptr<concurrency::FunctionRunner>&& f) {
  concurrency::Guard g(mutex_);
  logger_->log("start");
  if (maxSimultaneousSecureConnections_ > 0) {
    // This prevents pendingSecureStarts_ from growing entirely without
    // bound. The bound is set extremely high, though, so it's unlikely
    // it will ever be hit.
    if (pendingSecureStarts_.size() >
        maxSimultaneousSecureConnections_ * kPendingFailureRatio) {
      logger_->log("pending_secure_starts_overflow");
      throw concurrency::TooManyPendingTasksException(
        "pendingSecureStarts_ overflow: too many connections attempted");
    }
    if (secureConnectionsInProgress_ >= maxSimultaneousSecureConnections_) {
      pendingSecureStarts_.push_back(f);
      logger_->logValue("pending_secure_starts", pendingSecureStarts_.size());
      return;
    }
  }
  ++secureConnectionsInProgress_;
  logger_->logValue("secure_connections_in_progress",
                    secureConnectionsInProgress_);
  threadManager_->add(f);
}

void SaslThreadManager::end() {
  concurrency::Guard g(mutex_);
  logger_->log("end");
  if (!pendingSecureStarts_.empty()) {
    threadManager_->add(pendingSecureStarts_.front());
    pendingSecureStarts_.pop_front();
    logger_->logValue("pending_secure_starts", pendingSecureStarts_.size());
    logger_->log("add_pending");
  } else {
    DCHECK(secureConnectionsInProgress_ > 0);
    --secureConnectionsInProgress_;
    logger_->logValue("secure_connections_in_progress",
                 secureConnectionsInProgress_);
  }
}

}} // apache::thrift
