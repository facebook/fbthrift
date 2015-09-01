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

#include <folly/experimental/TestUtil.h>
#include <folly/gen/Base.h>
#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/tutorial/cpp/stateful/ServiceAuthState.h>
#include <thrift/tutorial/cpp/stateful/ShellHandler.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::stateful;

class ShellServerTest : public testing::Test {
public:
  EventBase eb;

  template <typename T>
  T sync(Future<T> f) { return f.waitVia(&eb).get(); }

  const fs::path& path() const { return cd_.path(); }

private:
  test::ChangeToTempDir cd_;
};

TEST_F(ShellServerTest, demo) {
  auto authState = make_shared<ServiceAuthState>();
  auto handler = make_shared<ShellHandlerFactory>(authState);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<ShellServiceAsyncClient>(eb);

  EXPECT_EQ(
      vector<string>({""}),
      gen::from(sync(client->future_listSessions()))
      | gen::field(&SessionInfo::username)
      | gen::as<vector>());

  sync(client->future_authenticate("blah"));
  EXPECT_EQ(
      vector<string>({"blah"}),
      gen::from(sync(client->future_listSessions()))
      | gen::field(&SessionInfo::username)
      | gen::as<vector>());

  EXPECT_EQ(path().string(), sync(client->future_pwd()));

  test::TemporaryDirectory q("bah", path());
  sync(client->future_chdir(q.path().string()));
  EXPECT_EQ(q.path().string(), sync(client->future_pwd()));

  test::TemporaryFile z("hahaha", q.path());
  EXPECT_EQ(
      vector<string>({".", "..", z.path().filename().string()}),
      gen::from(sync(client->future_listDirectory(q.path().string())))
      | gen::field(&StatInfo::name)
      | gen::order
      | gen::as<vector>());

  writeFile(string{"hello\nworld\n"}, z.path().c_str());
  EXPECT_EQ(
      "hello\nworld\n",
      sync(client->future_cat(z.path().filename().string())));
}
