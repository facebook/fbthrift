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

#include "gen-cpp2/TestService.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"

#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"

#include "thrift/lib/cpp2/async/StubSaslClient.h"
#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::test::cpp2::TestServiceAsyncClient;

class TestInterface : public TestServiceSvIf {
  void sendResponse(std::string& _return, int64_t size) {
    if (size >= 0) {
      usleep(size);
    }

    _return = "test" + boost::lexical_cast<std::string>(size);
  }

  void noResponse(int64_t size) {
    usleep(size);
  }

  void echoRequest(std::string& _return, std::unique_ptr<std::string> req) {
    _return = *req + "ccccccccccccccccccccccccccccccccccccccccccccc";
  }

  typedef apache::thrift::HandlerCallback<std::unique_ptr<std::string>>
      StringCob;
  void async_tm_serializationTest(std::unique_ptr<StringCob> callback,
                                  bool inEventBase) {
    std::unique_ptr<std::string> sp(new std::string("hello world"));
    callback->result(std::move(sp));
  }

  void async_eb_eventBaseAsync(std::unique_ptr<StringCob> callback) {
    std::unique_ptr<std::string> hello(new std::string("hello world"));
    callback->result(std::move(hello));
  }

  void async_tm_notCalledBack(std::unique_ptr<
                              apache::thrift::HandlerCallback<void>> cb) {
  }
};

std::shared_ptr<ThriftServer> getServer() {
  std::shared_ptr<ThriftServer> server(new ThriftServer);
  std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory(
      new apache::thrift::concurrency::PosixThreadFactory);
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    threadManager(
        apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(1,
          5,
          false,
          2));
  threadManager->threadFactory(threadFactory);
  threadManager->start();
  server->setThreadManager(threadManager);
  server->setPort(0);
  server->setSaslEnabled(true);
  server->setSaslServerFactory(
      [] (apache::thrift::async::TEventBase* evb) {
        return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
      }
  );
  server->setInterface(std::unique_ptr<TestInterface>(new TestInterface));
  return server;
}

int SyncClientTest() {

  ScopedServerThread sst(getServer());
  auto port = sst.getAddress()->getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  TestServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  std::string response;
  client.sync_sendResponse(response, 64);
  assert(response == "test64");
  return 0;
}

int main(int argc, char** argv) {

  return SyncClientTest();
}
