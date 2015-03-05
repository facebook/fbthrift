/*
 * Copyright 2015 Facebook, Inc.
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

#include <fstream>
#include <gtest/gtest.h>

#include <folly/experimental/TestUtil.h>
#include <thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h>

DECLARE_string(thrift_cc_manager_kill_switch_file);

using namespace std;
using namespace apache::thrift::krb5;

class Krb5CredentialsCacheManagerTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    // Set up a temporary directory for the test
    tmpDir_ = folly::make_unique<folly::test::TemporaryDirectory>(
      "krb5ccmanagertest");

    // Modify the gflag for cc manager kill switch for the test to a file in
    // the temporary directory
    FLAGS_thrift_cc_manager_kill_switch_file = folly::to<string>(
      tmpDir_->path().string(),
      "/cc_manager_kill_switch");

    ctx_ = folly::make_unique<Krb5Context>();
  }

  virtual void TearDown() {
  }

  virtual void enableCCManagaerKillSwitch(){
    // Enable kill switch (by touching the file) before starting the cc manager
    // thread.
    ofstream killSwitchFile;
    killSwitchFile.open(FLAGS_thrift_cc_manager_kill_switch_file);
    killSwitchFile.close();
  }

  unique_ptr<Krb5Context> ctx_;
  unique_ptr<folly::test::TemporaryDirectory> tmpDir_;

  /* Initializing this starts the cc manager thread. Since we might want to do
   * something (e.g. enable kill switch) before starting the thread, this is
   * intentionally not initialized in the SetUp(). The tests should initialize
   * this as needed.
   */
  unique_ptr<Krb5CredentialsCacheManager> ccManager_;
};

TEST_F(Krb5CredentialsCacheManagerTest, KillSwitchTest) {
  // Activate the cc manager kill switch before starting the thread.
  enableCCManagaerKillSwitch();
  ccManager_ = folly::make_unique<Krb5CredentialsCacheManager>();

  // Since the cc manager kill switch is activated before startig the cc manager
  // thread, the underlying ccStore should not be initialized.
  EXPECT_FALSE(ccManager_->waitUntilCacheStoreInitialized());
}
