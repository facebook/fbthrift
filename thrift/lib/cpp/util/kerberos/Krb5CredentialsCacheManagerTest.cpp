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
  explicit Krb5CredentialsCacheManagerTest(
      const std::string& principal = "")
    : Krb5CredentialsCacheManager(principal) {
    // Kill the manage thread
    stopThread();
  }

  std::unique_ptr<Krb5CCache> kInit() {
    auto mem = Krb5CredentialsCacheManager::kInit();
    client_ = folly::make_unique<Krb5Principal>(mem->getClientPrincipal());
    return mem;
  }

  using Krb5CredentialsCacheManager::getCache;
  using Krb5CredentialsCacheManager::buildRenewedCache;
  using Krb5CredentialsCacheManager::readInCache;
  using Krb5CredentialsCacheManager::writeOutCache;
  using Krb5CredentialsCacheManager::importMemoryCache;
  using Krb5CredentialsCacheManager::doTgsReq;
};

class KrbCCTest : public ::testing::Test {
protected:
  KrbCCTest() :
    manager_(),
    testHost_("host/dev1544.prn1.facebook.com@PRN1.PROD.FACEBOOK.COM"),
    testHost2_("host/dev1506.prn1.facebook.com@PRN1.PROD.FACEBOOK.COM") {
    setenv("KRB5_CONFIG", "/etc/krb5-thrift.conf", 0);
  }
  virtual void SetUp() {}
  virtual void TearDown() {}

  std::unique_ptr<Krb5CCache> getCacheWithOneCred() {
    // kinit and do request
    auto mem = manager_.kInit();
    Krb5Context context;
    Krb5Principal principal(context.get(), testHost_);
    manager_.incUsedService(testHost_);
    manager_.doTgsReq(principal.get(), *mem);
    int count = mem->getServicePrincipalList(false).size();
    EXPECT_GT(count, 1);
    return mem;
  }

  Krb5CredentialsCacheManagerTest manager_;
  string testHost_;
  string testHost2_;
};

TEST_F(KrbCCTest, TestKinit) {
  auto mem = manager_.kInit();
  EXPECT_EQ(1, mem->getServicePrincipalList(false).size());
}

TEST_F(KrbCCTest, TestKinitNoPrinc) {
  Krb5CredentialsCacheManagerTest manager("no/such/principal@REALM");
  ASSERT_THROW(manager.kInit(), runtime_error);
}

TEST_F(KrbCCTest, TestRead) {
  std::shared_ptr<Krb5CCache> mem  = manager_.kInit();
  manager_.importMemoryCache(mem);
  manager_.writeOutCache(1000);
  mem = manager_.readInCache();
  EXPECT_EQ(1, mem->getServicePrincipalList(false).size());
}

TEST_F(KrbCCTest, TestWrite1) {
  std::shared_ptr<Krb5CCache> mem = manager_.kInit();
  manager_.importMemoryCache(mem);
  // Should persist tgt no matter what the number is.
  manager_.writeOutCache(0);
  mem = manager_.readInCache();
  EXPECT_EQ(1, mem->getServicePrincipalList(false).size());
}

TEST_F(KrbCCTest, TestRequest) {
  auto mem = getCacheWithOneCred();
  EXPECT_EQ(1, mem->getServicePrincipalList(true).size());
}

TEST_F(KrbCCTest, TestPersistThenRead) {
  std::shared_ptr<Krb5CCache> mem = getCacheWithOneCred();
  int count = mem->getServicePrincipalList(false).size();

  // persist, then re-read
  manager_.importMemoryCache(mem);
  manager_.writeOutCache(100);
  mem = manager_.readInCache();

  EXPECT_EQ(count, mem->getServicePrincipalList(false).size());
}

TEST_F(KrbCCTest, TestPersist0ThenRead) {
  std::shared_ptr<Krb5CCache> mem = getCacheWithOneCred();
  int count = mem->getServicePrincipalList(false).size();

  // persist, then re-read
  manager_.importMemoryCache(mem);
  manager_.writeOutCache(0);
  mem = manager_.readInCache();

  EXPECT_EQ(count - 1, mem->getServicePrincipalList(false).size());
}

TEST_F(KrbCCTest, TestBuildRenewedCache) {
  Krb5Context context;
  Krb5Principal principal(context.get(), testHost_);
  std::shared_ptr<Krb5CCache> mem = getCacheWithOneCred();
  auto time = mem->getLifetime(principal.get());
  ASSERT_NE(time.second, 0);
  int count = mem->getServicePrincipalList(false).size();
  sleep(1);
  manager_.importMemoryCache(mem);
  mem = manager_.buildRenewedCache();
  auto time2 = mem->getLifetime(principal.get());
  ASSERT_NE(time2.second, 0);

  EXPECT_EQ(count, mem->getServicePrincipalList(false).size());
  EXPECT_GT(time2.second, time.second);
}

TEST_F(KrbCCTest, TestPersistFrequency) {
  // kinit and do request
  std::shared_ptr<Krb5CCache> mem = manager_.kInit();
  Krb5Context context;
  Krb5Principal principal(context.get(), testHost_);
  Krb5Principal principal2(context.get(), testHost2_);

  // get two tickets in the cache
  manager_.doTgsReq(principal.get(), *mem);
  manager_.doTgsReq(principal2.get(), *mem);
  manager_.incUsedService(testHost_);
  manager_.incUsedService(testHost2_);

  // make sure we actually have 2
  EXPECT_EQ(2, mem->getServicePrincipalList(true).size());

  // persist / reread all of them to double check
  manager_.importMemoryCache(mem);
  manager_.writeOutCache(100);
  mem = manager_.readInCache();
  EXPECT_EQ(2, mem->getServicePrincipalList(true).size());

  // Now increment host 2 twice
  manager_.incUsedService(testHost2_);

  // Only persist 1, make sure it's the more frequent one.
  manager_.writeOutCache(1);
  mem = manager_.readInCache();
  EXPECT_EQ(1, mem->getServicePrincipalList(true).size());
  auto element = mem->getServicePrincipalList(true);
  EXPECT_EQ(testHost2_, folly::to<string>(element[0]));
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
