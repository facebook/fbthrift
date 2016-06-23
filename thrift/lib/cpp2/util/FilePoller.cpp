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
folly::Singleton<
    apache::thrift::FilePoller,
    apache::thrift::FilePoller::ThriftInternalPollerTag>
    thriftInternalFilePoller;
}

namespace apache {
namespace thrift {

thread_local bool FilePoller::ThreadProtector::polling_ = false;

FilePoller::FilePoller(std::chrono::milliseconds pollInterval) {
  init(pollInterval);
}

FilePoller::~FilePoller() { scheduler_.shutdown(); }

void FilePoller::init(std::chrono::milliseconds pollInterval) {
  scheduler_.setThreadName("file-poller");
  scheduler_.addFunction(
      [this] { this->checkFiles(); }, pollInterval, "file-poller");
  start();
}

void FilePoller::checkFiles() noexcept {
  std::lock_guard<std::mutex> lg(filesMutex_);
  ThreadProtector tp;
  for (auto& fData : fileDatum_) {
    auto modData = getFileModData(fData.first);
    auto& fileData = fData.second;
    if (fileData.condition(fileData.modData, modData) && fileData.yCob) {
      fileData.yCob();
    } else if (fileData.nCob) {
      fileData.nCob();
    }
    fileData.modData = modData;
  }
}

void FilePoller::initFileData(
    const std::string& fName,
    FileData& fData) noexcept {
  auto modData = getFileModData(fName);
  fData.modData.exists = modData.exists;
  fData.modData.modTime = modData.modTime;
}

void FilePoller::addFileToTrack(
    const std::string& fileName,
    Cob yCob,
    Cob nCob,
    Condition condition) {
  if (ThreadProtector::inPollerThread()) {
    LOG(ERROR) << "Adding files from a callback is disallowd";
    return;
  }
  std::lock_guard<std::mutex> lg(filesMutex_);
  fileDatum_[fileName] = FileData(yCob, nCob, condition);
  initFileData(fileName, fileDatum_[fileName]);
}

void FilePoller::removeFileToTrack(const std::string& fileName) {
  if (ThreadProtector::inPollerThread()) {
    LOG(ERROR) << "Removing files from a callback is disallowd";
    return;
  }
  std::lock_guard<std::mutex> lg(filesMutex_);
  fileDatum_.erase(fileName);
}

FilePoller::FileModificationData FilePoller::getFileModData(
    const std::string& path) noexcept {
  struct stat info;
  int ret = stat(path.c_str(), &info);
  if (ret != 0) {
    return FileModificationData{false, 0};
  }
  return FileModificationData{true, info.st_mtime};
}
}
}
