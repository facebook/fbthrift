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
#include <thrift/lib/cpp2/util/FilePoller.h>

#include <sys/stat.h>

using namespace folly;

namespace {

// Returns <file exists, file update time>
std::pair<bool, time_t> getFileProps(const std::string& path) {
  struct stat info;
  int ret = stat(path.c_str(), &info);
  if (ret != 0) {
    return std::make_pair(false, 0);
  }
  return std::make_pair(true, info.st_mtime);
}
}

namespace apache {
namespace thrift {

FilePoller::FilePoller(std::string fileName,
                       std::chrono::milliseconds pollInterval)
    : fileName_(std::move(fileName)) {
  auto fileProps = getFileProps(fileName_);
  fileExists_ = fileProps.first;
  lastUpdateAt_ = fileProps.second;
  scheduler_.setThreadName("file-poller");
  scheduler_.addFunction(
      [this] { this->checkFile(); }, pollInterval, "file-poller");
  scheduler_.start();
}

FilePoller::~FilePoller() { scheduler_.shutdown(); }

void FilePoller::addCallback(std::function<void()> callback) {
  SharedMutex::WriteHolder holder(mutex_);
  callbacks_.push_back(std::move(callback));
}

void FilePoller::checkFile() noexcept {
  if (updateFileState()) {
    SharedMutex::ReadHolder holder(mutex_);
    for (auto& callback : callbacks_) {
      callback();
    }
  }
}

bool FilePoller::updateFileState() noexcept {
  bool stateChanged = false;
  auto fileProps = getFileProps(fileName_);
  bool fileNowExists = fileProps.first;
  // If the file did not exist before, and now exists.
  if (fileNowExists ^ fileExists_) {
    fileExists_ = fileNowExists;
    stateChanged = true;
  }

  if (!fileExists_) {
    return stateChanged;
  }

  const time_t mtime = fileProps.second;
  if (!mtime) {
    LOG(ERROR) << "File exists, and bad modified time";
  }
  if (mtime != lastUpdateAt_) {
    lastUpdateAt_ = mtime;
    stateChanged = true;
  }
  return stateChanged;
}
}
}
