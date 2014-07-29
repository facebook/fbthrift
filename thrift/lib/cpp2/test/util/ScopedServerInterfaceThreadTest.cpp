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

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/util/gen-cpp2/SimpleService.h>
#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::util::cpp2;

class SimpleServiceImpl : public virtual SimpleServiceSvIf {
public:
  ~SimpleServiceImpl() {}
  virtual void async_tm_add(
      unique_ptr<HandlerCallback<int64_t>> cb,
      int64_t a,
      int64_t b) override {
    cb->result(a + b);
  }
};

TEST(ScopedServerInterfaceThread, nada) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());
}

TEST(ScopedServerInterfaceThread, example) {
  ScopedServerInterfaceThread ssit(
    make_shared<SimpleServiceImpl>());

  EventBase eb;
  SimpleServiceAsyncClient cli(
    HeaderClientChannel::newChannel(
      TAsyncSocket::newSocket(
        &eb, ssit.getAddress())));

  EXPECT_EQ(6, cli.sync_add(-3, 9));
}
