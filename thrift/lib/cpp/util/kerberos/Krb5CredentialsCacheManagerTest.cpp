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

#include <sstream>
#include <gtest/gtest.h>

#include "folly/Memory.h"
#include "thrift/lib/cpp/util/kerberos/Krb5Util.h"
#include "thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h"

using namespace std;
using namespace apache::thrift::krb5;

class Krb5CredentialsCacheManagerTest :
  public apache::thrift::krb5::Krb5CredentialsCacheManager {
 public:
  using Krb5CredentialsCacheManager::readInCache;
  using Krb5CredentialsCacheManager::writeOutCache;
  using Krb5CredentialsCacheManager::stopThread;
};

class KrbCCTest : public ::testing::Test {
protected:
  KrbCCTest() :
    testHost1_("host/dev1544.prn1.facebook.com@PRN1.PROD.FACEBOOK.COM") {
    setenv("KRB5_CONFIG", "/etc/krb5-thrift.conf", 0);
  }
  virtual void SetUp() {}
  virtual void TearDown() {}
  Krb5CredentialsCacheManagerTest& getManager() {
    static Krb5CredentialsCacheManagerTest manager;
    return manager;
  }
  string testHost1_;
};

TEST_F(KrbCCTest, TestRead) {
  Krb5Context ctx;
  Krb5Principal host(ctx.get(), testHost1_);
  getManager().waitForCache(host);
  getManager().stopThread();

  getManager().writeOutCache(1000);
  auto mem = getManager().readInCache();
  EXPECT_LT(1, mem->getServicePrincipalList(false).size());

  // Should persist tgt no matter what the number is.
  getManager().writeOutCache(0);
  mem = getManager().readInCache();
  EXPECT_LT(0, mem->getServicePrincipalList(false).size());
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
