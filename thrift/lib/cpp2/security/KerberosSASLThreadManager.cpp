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

#include <folly/ExceptionWrapper.h>
#include <folly/ThreadName.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>

#include <chrono>
#include <thread>

namespace apache { namespace thrift {

using namespace std;
using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::concurrency;

DEFINE_int32(max_connections_per_sasl_thread, -1,
             "Cap on the number of simultaneous secure connection setups "
             "per sasl thread. -1 is no cap.");
DEFINE_int32(sasl_health_check_thread_period_ms, 1000,
             "Number of ms between SASL thread health checks");

const int kPendingFailureRatio = 1000;

SaslThreadManager::SaslThreadManager(std::shared_ptr<SecurityLogger> logger,
                                     int threadCount, int stackSizeMb)
  : logger_(logger)
  , lastActivity_()
  , healthy_(true) {

  secureConnectionsInProgress_ = 0;
  if (FLAGS_max_connections_per_sasl_thread > 0) {
    maxSimultaneousSecureConnections_ =
      threadCount * FLAGS_max_connections_per_sasl_thread;
  } else {
    maxSimultaneousSecureConnections_ = -1;
  }

  VLOG(1) << "Starting " << threadCount << " threads for SaslThreadManager";

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

  // We have to schedule it before we start looping evb
  scheduleThreadManagerHealthCheck();
  healthCheckThread_  = std::thread([this] {
      folly::setThreadName("sasl-thread-health");
      healthCheckEvb_.loopForever();
  });
}

void SaslThreadManager::threadManagerHealthCheck() {
  std::deque<std::shared_ptr<apache::thrift::concurrency::Runnable>>
    queueToDrain;
  {
    concurrency::Guard g(mutex_);
    auto cur = std::chrono::steady_clock::now();
    auto diff = cur - lastActivity_;
    auto idleWorkers = threadManager_->idleWorkerCount();
    if (idleWorkers == 0 &&
        diff > std::chrono::milliseconds(FLAGS_sasl_health_check_thread_period_ms)) {
      healthy_ = false;
      while (auto el = threadManager_->removeNextPending()) {
        queueToDrain.push_back(el);
      };
      // We are activating all queued up tasks (for draining),
      // lets update the counter tracking how many we have.
      secureConnectionsInProgress_ += pendingSecureStarts_.size();
      // Add the pending tasks to the draining queue
      while(!pendingSecureStarts_.empty()) {
        queueToDrain.push_back(pendingSecureStarts_.front());
        pendingSecureStarts_.pop_front();
      }
    } else {
      healthy_ = true;
    }
  }

  // Run queued work. Note that while draining, these runnables will
  // basically be noops since we've marked the SASL thread pool state as
  // unhealthy.
  while (!queueToDrain.empty()) {
    auto& el = queueToDrain.front();
    el->run();
    queueToDrain.pop_front();
  }
}

bool SaslThreadManager::isHealthy() {
  return healthy_;
}

void SaslThreadManager::recordActivity() {
  concurrency::Guard g(mutex_);
  lastActivity_ = std::chrono::steady_clock::now();
}

void SaslThreadManager::scheduleThreadManagerHealthCheck() {
  healthCheckEvb_.tryRunAfterDelay([this]() {
    threadManagerHealthCheck();
    scheduleThreadManagerHealthCheck();
  }, FLAGS_sasl_health_check_thread_period_ms);
}

SaslThreadManager::~SaslThreadManager() {
  healthCheckEvb_.terminateLoopSoon();
  healthCheckThread_.join();
  // We should run the health check one last time to make sure
  // we drain all the queues before shutdown
  threadManagerHealthCheck();
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
  logger_->logValue("secure_connections_in_progress",
                    secureConnectionsInProgress_);
  auto ew = folly::try_and_catch<TooManyPendingTasksException>([&]() {
    threadManager_->add(f);
    ++secureConnectionsInProgress_;
  });
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
