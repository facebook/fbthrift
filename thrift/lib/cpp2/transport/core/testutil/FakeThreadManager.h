/*
 * Copyright 2017-present Facebook, Inc.
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

#pragma once

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache {
namespace thrift {

class FakeThreadManager : public apache::thrift::concurrency::ThreadManager {
 public:
  ~FakeThreadManager() override {}

  void setThrowOnAdd(bool val) {
    throwOnAdd_ = val;
  }

  void start() override {}

  void join() override {}

  void add(
      std::shared_ptr<apache::thrift::concurrency::Runnable> task,
      int64_t /*timeout*/,
      int64_t /*expiration*/,
      bool /*cancellable*/,
      bool /*numa*/) {
    if (throwOnAdd_) {
      // Simulates an overloaded server.
      throw std::exception();
    } else {
      auto thread = factory_.newThread(task);
      thread->start();
    }
  }

  // Following methods are not required for this fake object.

  void add(folly::Func /*f*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void stop() override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  STATE state() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return STARTED;
  }

  std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory()
      const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return std::shared_ptr<apache::thrift::concurrency::ThreadFactory>();
  }

  void threadFactory(
      std::shared_ptr<apache::thrift::concurrency::ThreadFactory> /*value*/)
      override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  std::string getNamePrefix() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return "";
  }

  void setNamePrefix(const std::string& /*name*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void addWorker(size_t /*value*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void removeWorker(size_t /*value*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  size_t idleWorkerCount() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  size_t workerCount() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  size_t pendingTaskCount() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  size_t totalTaskCount() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  size_t pendingTaskCountMax() const override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  size_t expiredTaskCount() override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return 1;
  }

  bool tryAdd(std::shared_ptr<apache::thrift::concurrency::Runnable> /*task*/)
      override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return true;
  }

  void remove(std::shared_ptr<apache::thrift::concurrency::Runnable> /*task*/)
      override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  std::shared_ptr<apache::thrift::concurrency::Runnable> removeNextPending()
      override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return std::shared_ptr<apache::thrift::concurrency::Runnable>();
  }

  void setExpireCallback(ExpireCallback /*expireCallback*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void setCodelCallback(ExpireCallback /*expireCallback*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void setThreadInitCallback(InitCallback /*initCallback*/) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  void enableCodel(bool) override {
    LOG(FATAL) << "Method not implemented in this fake object";
  }

  folly::Codel* getCodel() override {
    LOG(FATAL) << "Method not implemented in this fake object";
    return nullptr;
  }

 private:
  apache::thrift::concurrency::PosixThreadFactory factory_;
  bool throwOnAdd_{false};
};

} // namespace thrift
} // namespace apache
