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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <folly/Baton.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/Range.h>
#include <folly/experimental/TestUtil.h>
#include <thrift/lib/cpp2/util/FilePoller.h>

using namespace testing;
using namespace folly;
using namespace folly::test;
using namespace apache::thrift;

class FilePollerTest : public testing::Test {
 public:
  void createFile() { File(tmpFile, O_CREAT); }

  TemporaryDirectory tmpDir;
  fs::path tmpFilePath{tmpDir.path() / "file-poller"};
  std::string tmpFile{tmpFilePath.string()};
};

void updateModifiedTime(const std::string& fileName, bool forward = true) {
  auto previous = std::chrono::system_clock::from_time_t(
      boost::filesystem::last_write_time(fileName));
  auto diff = std::chrono::seconds(10);
  std::chrono::system_clock::time_point newTime;
  if (forward) {
    newTime = previous + diff;
  } else {
    newTime = previous - diff;
  }

  time_t newTimet = std::chrono::system_clock::to_time_t(newTime);
  boost::filesystem::last_write_time(fileName, newTimet);
}

TEST_F(FilePollerTest, TestUpdateFile) {
  createFile();
  Baton<> baton;
  bool updated = false;
  FilePoller poller(std::chrono::milliseconds(1));
  poller.addFileToTrack(tmpFile, [&]() {
    updated = true;
    baton.post();
  });
  updateModifiedTime(tmpFile);
  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(5)));
  ASSERT_TRUE(updated);
}

TEST_F(FilePollerTest, TestUpdateFileBackwards) {
  createFile();
  Baton<> baton;
  bool updated = false;
  FilePoller poller(std::chrono::milliseconds(1));
  poller.addFileToTrack(tmpFile, [&]() {
    updated = true;
    baton.post();
  });
  updateModifiedTime(tmpFile, false);
  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(5)));
  ASSERT_TRUE(updated);
}

TEST_F(FilePollerTest, TestCreateFile) {
  Baton<> baton;
  bool updated = false;
  createFile();
  remove(tmpFile.c_str());
  FilePoller poller(std::chrono::milliseconds(1));
  poller.addFileToTrack(tmpFile, [&]() {
    updated = true;
    baton.post();
  });
  File(creat(tmpFile.c_str(), O_RDONLY));
  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(5)));
  ASSERT_TRUE(updated);
}

TEST_F(FilePollerTest, TestDeleteFile) {
  Baton<> baton;
  bool updated = false;
  createFile();
  FilePoller poller(std::chrono::milliseconds(1));
  poller.addFileToTrack(tmpFile, [&]() {
    updated = true;
    baton.post();
  });
  remove(tmpFile.c_str());
  ASSERT_FALSE(baton.timed_wait(std::chrono::seconds(5)));
  ASSERT_FALSE(updated);
}

void waitForUpdate(
    std::mutex& m,
    std::condition_variable& cv,
    bool& x,
    bool expect = true) {
  std::unique_lock<std::mutex> lk(m);
  if (!x) {
    cv.wait_for(lk, std::chrono::seconds(5), [&] { return x; });
  }
  if (expect) {
    ASSERT_TRUE(x);
  } else {
    ASSERT_FALSE(x);
  }
  x = false;
}

TEST_F(FilePollerTest, TestTwoUpdatesAndDelete) {
  createFile();
  std::mutex m;
  std::condition_variable cv;

  bool updated = false;
  FilePoller poller(std::chrono::milliseconds(1));
  poller.addFileToTrack(tmpFile, [&]() {
    std::unique_lock<std::mutex> lk(m);
    updated = true;
    cv.notify_one();
  });

  updateModifiedTime(tmpFile);
  ASSERT_NO_FATAL_FAILURE(waitForUpdate(m, cv, updated));

  updateModifiedTime(tmpFile, false);
  ASSERT_NO_FATAL_FAILURE(waitForUpdate(m, cv, updated));

  remove(tmpFile.c_str());
  ASSERT_NO_FATAL_FAILURE(waitForUpdate(m, cv, updated, false));
}
