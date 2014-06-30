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

#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

#include <thrift/lib/cpp/util/kerberos/Krb5Util.h>

using namespace std;
using namespace apache::thrift::krb5;

class Krb5UtilTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    name_ = "comp1/comp2@REALM_NAME";
  }

  virtual void TearDown() {
  }

  static uint count_creds(Krb5CCache& cc) {
    uint count = 0;
    for (const auto& creds : cc) {
      ++count;
    }
    return count;
  }

  static uint count_all_creds(Krb5CCache& cc) {
    uint count = 0;
    for (auto it = cc.begin(true); it != cc.end(); ++it) {
      ++count;
    }
    return count;
  }

  static uint count_ktentries(Krb5Keytab& kt) {
    uint count = 0;
    for (const auto& kte : kt) {
      ++count;
    }
    return count;
  }

  Krb5Context context_;
  const char* name_;
};

TEST_F(Krb5UtilTest, TestRaiseIf) {
  try {
    raiseIf(context_.get(), 0, "working good");
    SUCCEED();
  } catch (...) {
    FAIL();
  }

  try {
    raiseIf(context_.get(), KRB5_KDC_UNREACH, "breaking bad");
    FAIL();
  } catch (...) {
    SUCCEED();
  }
}

TEST_F(Krb5UtilTest, TestKrb5Conf) {
  ifstream ifile("/etc/krb5.conf");
  EXPECT_TRUE(ifile);

}

TEST_F(Krb5UtilTest, TestPrincipalEquals) {
  Krb5Principal p1(context_.get(), "one@REALM");
  Krb5Principal p2(context_.get(), "two@REALM");
  Krb5Principal p3(context_.get(), "one@REALM");

  EXPECT_NE(p1, p2);
  EXPECT_EQ(p1, p3);
  EXPECT_NE(p2, p3);
}

TEST_F(Krb5UtilTest, TestPrincipalParseUnparse) {
  Krb5Principal princ(context_.get(), name_);
  EXPECT_EQ(name_, folly::to<std::string>(princ));
  EXPECT_NE(nullptr, princ.get());

  Krb5Principal princ2 = std::move(princ);
  EXPECT_EQ(nullptr, princ.get());
  EXPECT_NE(nullptr, princ2.get());

  std::stringstream ss;
  ss << princ2;
  EXPECT_EQ(name_, ss.str());

  EXPECT_EQ(2, princ2.size());
  EXPECT_EQ("comp1", princ2.getComponent(0));
  EXPECT_EQ("comp2", princ2.getComponent(1));
  EXPECT_EQ("REALM_NAME", princ2.getRealm());

  Krb5Principal princ3(context_.get(), "norealm");
  EXPECT_EQ(1, princ3.size());
  EXPECT_EQ("norealm", princ3.getComponent(0));
  EXPECT_EQ("", princ3.getComponent(1));
  // This will be the default realm, whatever that is.
  EXPECT_NE("", princ3.getRealm());

  Krb5Principal princ4(context_.get(), "emptyrealm@");
  EXPECT_EQ(1, princ4.size());
  EXPECT_EQ("emptyrealm", princ4.getComponent(0));
  EXPECT_EQ("", princ4.getComponent(1));
  EXPECT_EQ("", princ4.getRealm());

  Krb5Principal princ5(context_.get(), "empty//component@REALM");
  EXPECT_EQ(3, princ5.size());
  EXPECT_EQ("empty", princ5.getComponent(0));
  EXPECT_EQ("", princ5.getComponent(1));
  EXPECT_EQ("component", princ5.getComponent(2));
}

TEST_F(Krb5UtilTest, TestGetHostRealm) {
  // Test a valid hostname which belongs to a single realm
  string hostName = "dev1169.prn2.facebook.com";
  string expectedRealm = "DEV.FACEBOOK.COM";
  vector<string> realms;
  try {
    realms = getHostRealm(context_.get(), hostName);
  } catch (...) {
    FAIL();
  }
  EXPECT_EQ(1, realms.size());
  EXPECT_EQ(expectedRealm, realms[0]);

  // For bogus hostnames, we get an empty string as the realm
  string bogusHostName = "bogus.host.name.com";
  expectedRealm = "";
  try {
    realms = getHostRealm(context_.get(), bogusHostName);
  } catch (...) {
    LOG(INFO) << "Caught exception for bogus hostname...";
    FAIL();
  }
  EXPECT_EQ(1, realms.size());
  EXPECT_EQ(expectedRealm, realms[0]);

  // TODO(sandeepkk): Test when a hostname returns more than one realm
}

TEST_F(Krb5UtilTest, TestCCache) {
  // Test that we can create a default cache
  auto ccache = Krb5CCache::makeDefault();
  // Test that we can resolve some cache name
  auto ccache2 = Krb5CCache::makeResolve(
    "FILE:/var/run/ccache/krb5cc_doesnotexist");
  // Create a new in-memory cache
  string name3;
  {
    auto ccache3 = Krb5CCache::makeNewUnique("MEMORY");
    ccache3.initialize(Krb5Principal(context_.get(), "client3").get());
    EXPECT_EQ(0, ccache3.getServicePrincipalList().size());
    EXPECT_EQ(0, count_creds(ccache3));
    name3 = ccache3.getName();
  }
  // Test the cache persists after close.
  auto ccache3b = Krb5CCache::makeResolve(name3);
  EXPECT_NO_THROW(ccache3b.getClientPrincipal());
  // Test a cache which deletes itself.
  string name4;
  {
    auto ccache4 = Krb5CCache::makeNewUnique("MEMORY");
    ccache4.initialize(Krb5Principal(context_.get(), "client4").get());
    EXPECT_EQ(0, ccache4.getServicePrincipalList().size());
    EXPECT_EQ(0, count_creds(ccache4));
    ccache4.setDestroyOnClose();
    name4 = ccache4.getName();
  }
  auto ccache4b = Krb5CCache::makeResolve(name4);
  EXPECT_THROW(ccache4b.getClientPrincipal(), std::runtime_error);
}

TEST_F(Krb5UtilTest, TestCCacheGetLifetime) {
  auto client = Krb5Principal(context_.get(), "client@TESTREALM");
  auto tgt = Krb5Principal(context_.get(), "krbtgt/TESTREALM@TESTREALM");
  auto local_service =
    Krb5Principal(context_.get(), "local_service@TESTREALM");
  auto remote_tgt =
    Krb5Principal(context_.get(), "krbtgt/REMOTEREALM@TESTREALM");
  auto remote_service =
    Krb5Principal(context_.get(), "remote_service@REMOTEREALM");
  auto missing_service =
    Krb5Principal(context_.get(), "missing_service@MISSINGREALM");

  auto ccache = Krb5CCache::makeNewUnique("MEMORY");
  krb5_error_code code = krb5_cc_initialize(
    context_.get(), ccache.get(), client.get());

  krb5_creds creds;
  memset(&creds, 0, sizeof(creds));
  creds.client = client.get();

  creds.server = tgt.get();
  creds.times.starttime = 1000000000;
  creds.times.endtime = creds.times.starttime + 86400;
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing config");

  creds.server = local_service.get();
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing config");

  creds.server = remote_tgt.get();
  creds.times.starttime += 60;
  creds.times.endtime += 60;
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing config");

  creds.server = remote_service.get();
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing config");

  auto lt = ccache.getLifetime();
  EXPECT_EQ(lt, make_pair(1000000000ul, 1000086400ul));

  lt = ccache.getLifetime(client.get());
  EXPECT_EQ(lt, make_pair(1000000000ul, 1000086400ul));

  lt = ccache.getLifetime(local_service.get());
  EXPECT_EQ(lt, make_pair(1000000000ul, 1000086400ul));

  lt = ccache.getLifetime(remote_service.get());
  EXPECT_EQ(lt, make_pair(1000000060ul, 1000086460ul));

  lt = ccache.getLifetime(missing_service.get());
  EXPECT_EQ(lt, make_pair(0ul, 0ul));
}

TEST_F(Krb5UtilTest, TestCCIterator) {
  auto client = Krb5Principal(context_.get(), "client@TESTREALM");
  auto config =
    Krb5Principal(context_.get(), "krb5_ccache_conf_data@X-CACHECONF:");
  auto service = Krb5Principal(context_.get(), "service@TESTREALM");

  // Create a new in-memory cache
  auto ccache = Krb5CCache::makeNewUnique("MEMORY");
  krb5_error_code code = krb5_cc_initialize(
    context_.get(), ccache.get(), client.get());
  raiseIf(context_.get(), code, "initializing ccache");

  krb5_creds creds;
  memset(&creds, 0, sizeof(creds));
  creds.client = client.get();

  creds.server = config.get();
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing config");
  EXPECT_EQ(0, count_creds(ccache));
  EXPECT_EQ(1, count_all_creds(ccache));

  creds.server = service.get();
  code = krb5_cc_store_cred(context_.get(), ccache.get(), &creds);
  raiseIf(context_.get(), code, "storing service");
  EXPECT_EQ(1, count_creds(ccache));
  EXPECT_EQ(2, count_all_creds(ccache));
}

TEST_F(Krb5UtilTest, TestKTIterator) {
  Krb5Keytab keytab(context_.get(), "MEMORY:foo");
  auto p1 = Krb5Principal(context_.get(), "p1@REALM");
  auto p2 = Krb5Principal(context_.get(), "p2@REALM");

  krb5_keytab_entry kte;
  memset(&kte, 0, sizeof(kte));
  kte.principal = p1.get();
  krb5_error_code code = krb5_kt_add_entry(context_.get(), keytab.get(), &kte);
  raiseIf(context_.get(), code, "storing p1");
  kte.principal = p2.get();
  code = krb5_kt_add_entry(context_.get(), keytab.get(), &kte);
  raiseIf(context_.get(), code, "storing p2");

  EXPECT_EQ(2, count_ktentries(keytab));
}

TEST_F(Krb5UtilTest, TestKeytab) {
  // Test keytab constructors
  Krb5Keytab keytab1(context_.get());
  Krb5Keytab keytab2(context_.get(), "/etc/krb5.keytab");
}

TEST_F(Krb5UtilTest, Krb5InitCredsOptTest) {
  // Test opt constructors
  Krb5InitCredsOpt opt(context_.get());
}
