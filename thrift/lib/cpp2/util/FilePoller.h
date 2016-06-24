/*
 * Copyright 2016 Facebook, Inc.
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

#include <chrono>
#include <folly/SharedMutex.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/ScopedEventBaseThread.h>

namespace apache {
namespace thrift {

/**
 * Polls for updates in the files. This poller uses
 * modified times to track changes in the files,
 * so it is the responsibility of the caller to check
 * whether the contents have actually changed.
 * It also assumes that when the file is modified, the
 * modfied time changes. This is a reasonable
 * assumption to make, so it should not be used in
 * situations where files may be modified without
 * changing the modified time.
 */
class FilePoller {
 public:
  FilePoller(std::string fileName, std::chrono::milliseconds pollInterval);

  ~FilePoller();

  FilePoller(const FilePoller& other) = delete;
  FilePoller& operator=(const FilePoller& other) = delete;

  FilePoller(FilePoller&& other) = delete;
  FilePoller&& operator=(FilePoller&& other) = delete;

  void stop() { scheduler_.shutdown(); }

  void start() { scheduler_.start(); }

  /**
   * Adds a callback for changes in the file.
   * The thread that the callback is invoked on is not
   * specified, so the caller needs to ensure thread
   * safety on their own.
   */
  void addCallback(std::function<void()> callback);

 private:
  bool updateFileState() noexcept;
  void checkFile() noexcept;

  std::string fileName_;
  folly::FunctionScheduler scheduler_;
  std::vector<std::function<void()>> callbacks_;

  std::string functionName_;
  time_t lastUpdateAt_{0};
  bool fileExists_{false};
  // protects callbacks
  folly::SharedMutex mutex_;
};
}
}
