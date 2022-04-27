/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/synchronization/CallOnce.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache::thrift {

class ThriftServer;

// This is a thin wrapper over folly::Executor to convert
// the executor to a ThreadManager
//
// It only expose add() interface to act as an Executor
//
// This is only for the purpose of ResourcePool migration,
// This should not be used for any custom purpose
class ExecutorToThreadManagerAdaptor : public concurrency::ThreadManager {
 public:
  explicit ExecutorToThreadManagerAdaptor(
      folly::Executor& ex, const ThriftServer* server = nullptr)
      : ka_(folly::getKeepAliveToken(ex)), server_(server) {}

  // These are the only two interfaces that are implemented
  void add(
      std::shared_ptr<concurrency::Runnable> task,
      [[maybe_unused]] int64_t timeout = 0,
      [[maybe_unused]] int64_t expiration = 0,
      [[maybe_unused]] Source source = Source::UPSTREAM) noexcept override {
    ka_->add([task = std::move(task)]() { task->run(); });
  }

  void add(folly::Func f) override { ka_->add(std::move(f)); }

  void start() override { recordStackTrace("start"); }

  void stop() override { recordStackTrace("stop"); }

  void join() override { recordStackTrace("join"); }

  STATE state() const override {
    recordStackTrace("state");
    return concurrency::ThreadManager::STARTED;
  }

  std::shared_ptr<concurrency::ThreadFactory> threadFactory() const override {
    recordStackTrace("threadFactory");
    return std::shared_ptr<concurrency::ThreadFactory>(
        new concurrency::PosixThreadFactory());
  }

  void threadFactory(std::shared_ptr<concurrency::ThreadFactory>) override {
    recordStackTrace("threadFactory");
  }

  std::string getNamePrefix() const override {
    return "rp.executor_to_thread_manager_adaptor";
  }

  void setNamePrefix(const std::string&) override {
    recordStackTrace("setNamePrefix");
  }

  void addWorker(size_t) override { recordStackTrace("addWorker"); }

  void removeWorker(size_t) override { recordStackTrace("removeWorker"); }

  size_t idleWorkerCount() const override {
    recordStackTrace("idleWorkerCount");
    return 0;
  }

  size_t workerCount() const override {
    recordStackTrace("workerCount");
    return 0;
  }

  size_t pendingTaskCount() const override {
    recordStackTrace("pendingTaskCount");
    return 0;
  }

  size_t pendingUpstreamTaskCount() const override {
    recordStackTrace("pendingUpstreamTaskCount");
    return 0;
  }

  size_t totalTaskCount() const override {
    recordStackTrace("totalTaskCount");
    return 0;
  }

  size_t expiredTaskCount() override {
    recordStackTrace("expiredTaskCount");
    return 0;
  }

  void remove(std::shared_ptr<concurrency::Runnable>) override {
    recordStackTrace("remove");
  }

  std::shared_ptr<concurrency::Runnable> removeNextPending() override {
    recordStackTrace("removeNextPending");
    return nullptr;
  }

  void clearPending() override { recordStackTrace("clearPending"); }

  void enableCodel(bool) override { recordStackTrace("enableCodel"); }

  folly::Codel* getCodel() override {
    recordStackTrace("getCodel");
    return nullptr;
  }

  void setExpireCallback(ExpireCallback) override {
    recordStackTrace("setExpireCallback");
  }

  void setCodelCallback(ExpireCallback) override {
    recordStackTrace("setCodelCallback");
  }

  void setThreadInitCallback(InitCallback) override {
    recordStackTrace("setThreadInitCallback");
  }

  void addTaskObserver(std::shared_ptr<Observer>) override {
    recordStackTrace("addTaskObserver");
  }

  std::chrono::nanoseconds getUsedCpuTime() const override {
    recordStackTrace("getUsedCpuTime");
    return std::chrono::nanoseconds();
  }

  [[nodiscard]] KeepAlive<> getKeepAlive(
      ExecutionScope, Source) const override {
    return ka_;
  }

 private:
  folly::Executor::KeepAlive<> ka_;
  const ThriftServer* server_;
  // logging should only be done once if any as
  // it's quite expensive
  static folly::once_flag recordFlag_;

  void recordStackTrace(std::string) const;
};

} // namespace apache::thrift
