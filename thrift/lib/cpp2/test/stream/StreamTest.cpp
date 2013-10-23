/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include <gtest/gtest.h>
#include "thrift/lib/cpp2/test/stream/gen-cpp2/StreamService.h"
#include "thrift/lib/cpp2/test/stream/gen-cpp2/SimilarService.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"

#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"

#include "thrift/lib/cpp2/async/StubSaslClient.h"
#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include "thrift/lib/cpp2/test/stream/MockCallbacks.h"

#include "thrift/lib/cpp2/test/TestUtils.h"

#include <memory>
#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <string>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::async;
using namespace facebook::wangle;

typedef std::unique_ptr<InputStreamCallback<int>> ICP;
typedef std::unique_ptr<OutputStreamCallback<int>> OCP;
typedef std::unique_ptr<InputStreamCallback<std::string>> ISCP;
typedef std::unique_ptr<OutputStreamCallback<std::string>> OSCP;
typedef std::unique_ptr<InputStreamCallback<std::vector<std::string>>> IVSCP;

class StreamService : public StreamServiceSvIf {
  Future<std::unique_ptr<std::string>>
  future_sendResponse(int64_t size) {
    return makeFuture(std::unique_ptr<std::string>(new std::string(
        boost::lexical_cast<std::string>(size))));
  }

  Future<int32_t> future_returnSimple(int32_t zero) {
    return makeFuture(zero);
  }

  Future<std::unique_ptr<Subject>> future_returnSubject(
      std::unique_ptr<Subject> param) {
    return makeFuture(std::move(param));
  }

  void returnStream(AsyncOutputStream<int32_t>& output,
                    std::unique_ptr<std::string> which) {
    output.setCallback(OCP(new OutputCallback(10)));
    EXPECT_EQ(*which, "hello");
  }

  void doNothingWithStreams(AsyncInputStream<int32_t>& intStream,
                            int32_t num,
                            std::unique_ptr<std::vector<std::string>> words,
                            std::unique_ptr<Subject> comp,
                            AsyncInputStream<std::string>& another) {
    Subject google;
    google.will.be = "huge";
    EXPECT_EQ(google, *comp);

    intStream.setCallback(ICP(new InputCallback(num)));
    another.setCallback(ISCP(new InputStringCallback(*words)));
  }

  void returnSimpleWithStreams(AsyncOutputStream<int16_t>& output,
                               AsyncInputStream<int32_t>& intStream,
                               int32_t num,
                               std::unique_ptr<std::vector<std::string>> words,
                               std::unique_ptr<Subject> comp,
                               AsyncInputStream<std::string>& another) {
    // intStream and output are ignored

    Subject there;
    there.will.be = "blood";
    EXPECT_EQ(there, *comp);

    EXPECT_EQ(num, 1729);

    another.setCallback(ISCP(new InputStringCallback(*words)));
  }

  void returnSubjectWithStreams(AsyncOutputStream<Subject>& output,
                                AsyncInputStream<int32_t>& intStream,
                                int32_t num,
                                std::unique_ptr<std::vector<std::string>> words,
                                std::unique_ptr<Subject> comp,
                                AsyncInputStream<std::string>& another) {
  }

  void returnStreamWithStreams(
      AsyncOutputStream<std::vector<Subject>>& output,
      AsyncInputStream<int>& intStream,
      int32_t num,
      std::unique_ptr<std::vector<std::string>> words,
      std::unique_ptr<Subject> person,
      AsyncInputStream<std::vector<std::string>>& another) {

    Subject& I = *person;

    auto controller = output.makeController();
    another.setCallback(IVSCP(new InputStringListCallback(
        std::move(controller))));

    intStream.setCallback(ICP(new InputCallback(num)));

    std::string whatIWantToBe;
    for(auto& word : *words) {
      whatIWantToBe += word + " ";
    }

    EXPECT_EQ(I.will.be, whatIWantToBe);
  }

  void throwException(AsyncOutputStream<int>& output,
                      int32_t size) {
    if (size < 0) {
      SimpleException exception;
      exception.msg = "You can't throw exceptions here.";
      throw exception;
    } else {
      output.setCallback(OCP(new OutputException(size)));
    }
  }

  void simpleTwoWayStream(AsyncOutputStream<int>& output,
                          AsyncInputStream<int>& intStream) {
    output.setCallback(OCP(new OutputCallback(10)));
    intStream.setCallback(ICP(new InputCallback(20)));
  }

  void simpleOneWayStream(AsyncInputStream<int>& intStream) {
    intStream.setCallback(ICP(new InputCallback(10)));
  }

  void sum(AsyncOutputStream<int>& sum,
           AsyncInputStream<int>& nums) {
    auto controller = sum.makeController();
    nums.setCallback(ICP(new InputSumCallback(std::move(controller))));
  }

  void ignoreStreams(AsyncOutputStream<int>& output,
                     AsyncInputStream<int>& intStream) {
    ADD_FAILURE(); // we shouldn't end up calling this function
  }
};

class SimilarService : public SimilarServiceSvIf {
  public:
    Future<int32_t> future_ignoreStreams() {
      return makeFuture(1745);
    }

    void doNothing(AsyncOutputStream<int>& output,
                   AsyncInputStream<int>& intStream) {
      output.setCallback(OCP(new OutputReceiveClose(0)));
      intStream.setCallback(ICP(new InputCallback(0)));
    }
};

template <typename Server>
std::shared_ptr<ThriftServer> getServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::unique_ptr<Server>(new Server()));
  server->setSaslEnabled(true);
  server->setSaslServerFactory(
    [] (apache::thrift::async::TEventBase* evb) {
      return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
    }
  );

  return server;
}

template <typename Client>
class ClientTester {
  typedef std::unique_ptr<HeaderClientChannel,
          apache::thrift::async::TDelayedDestruction::Destructor> ChannelPtr;
  public:
    explicit ClientTester(int port)
      : socket_(TAsyncSocket::newSocket(&base_, "127.0.0.1", port)),
        channel_(new HeaderClientChannel(socket_)),
        client(ChannelPtr(channel_)) {
      channel_->setTimeout(10000);
    }

    ClientTester()
      : socket_(TAsyncSocket::newSocket(
          &base_, "127.0.0.1",
          Server::get(getServer<StreamService>)->getAddress().getPort())),
        channel_(new HeaderClientChannel(socket_)),
        client(ChannelPtr(channel_)) {
      channel_->setTimeout(10000);
    }

    void loop() {
      base_.loop();
    }

    void loopForever() {
      base_.loopForever();
    }

  private:
    TEventBase base_;
    std::shared_ptr<TAsyncSocket> socket_;
    HeaderClientChannel* channel_;

  public:
    Client client;
};

typedef ClientTester<StreamServiceAsyncClient> Tester;

TEST(StreamTest, SyncClientTest) {
  Tester tester;

  std::string response;
  tester.client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "64");
}

TEST(StreamTest, SimpleTest) {
  Tester tester;

  AsyncInputStream<int> ic(ICP(new InputCallback(10)));
  tester.client.returnStream(ic, "hello");
  tester.loop();
}

TEST(StreamTest, SimpleTwoWayTest) {
  Tester tester;

  AsyncInputStream<int> ic(ICP(new InputCallback(10)));
  AsyncOutputStream<int> oc(OCP(new OutputCallback(20)));
  tester.client.simpleTwoWayStream(ic, oc);
  tester.loop();
}

TEST(StreamTest, ExceptionTest) {
  Tester tester;

  AsyncInputStream<int> ic(ICP(new InputReceiveException(10)));
  tester.client.throwException(ic, 10);
  tester.loop();
}

TEST(StreamTest, SyncTest) {
  Tester tester;

  SyncInputStream<int32_t> is;
  SyncOutputStream<int32_t> os;
  is = tester.client.sync_simpleTwoWayStream(os);

  int i = 0;
  while (!is.isDone()) {
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }

  EXPECT_EQ(i, 10);

  for (int i = 0; i < 20; ++i) {
    os.put(i * 100 + 10);
  }
  os.close();

  EXPECT_TRUE(os.isClosed());
  EXPECT_TRUE(is.isDone());
  EXPECT_TRUE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamTest, SyncOneWayTest) {
  Tester tester;

  SyncOutputStream<int32_t> os;
  tester.client.sync_simpleOneWayStream(os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }
  os.close();

  EXPECT_TRUE(os.isClosed());
}

TEST(StreamTest, AsyncComplexObjectTest) {
  Tester tester;

  int numItems = 10;
  std::vector<std::string> words = {"one", "two", "three", "four", "five"};

  Subject facebook;
  facebook.will.be = "huge";

  AsyncOutputStream<int> ints(OCP(new OutputCallback(numItems)));
  AsyncOutputStream<std::string> strings(OSCP(new OutputStringCallback(words)));
  tester.client.doNothingWithStreams(ints, numItems, words, facebook, strings);

  tester.loop();
}

TEST(StreamTest, SyncComplexObjectTest) {
  Tester tester;

  std::vector<std::vector<std::string>> randomStrings({
    {"Lorem", "ipsum", "dolor", "sit", "amet,", "consectetur", "adipiscing"},
    {"elit.", "Nunc", "venenatis", "ipsum", "ac", "molestie", "euismod."},
    {"Cras", "consequat", "urna", "scelerisque,", "tincidunt", "velit", "non,"},
    {"hendrerit", "orci.", "Praesent", "non", "malesuada", "quam."},
    {"Phasellus", "odio", "nisl,", "volutpat", "non", "tempus", "non,", "odio"},
    {"nisl,", "volutpat", "non", "tempus", "non,", "mattis", "sed", "odio."},
    {"Donec", "gravida", "nisl", "nec", "interdum", "tempor.", "Praesent"},
    {"dictum", "metus", "eu", "pharetra", "dignissim.", "In", "hendrerit"},
    {"mollis", "turpis", "a", "tempor.", "Aliquam", "erat", "volutpat."},
    {"Aenean", "non", "lorem", "quis", "risus", "condimentum", "auctor."},
    {"Quisque", "laoreet,", "augue", "eget", "interdum", "lacinia,", "nibh"},
    {"risus", "volutpat", "tortor,", "sed", "scelerisque", "tortor", "dolor"},
    {"vitae"}
  });

  int numItems = 20;

  Subject I;
  I.will.be = "a level 90 arcane mage ";
  std::vector<std::string> words = {"a", "level", "90", "arcane", "mage"};

  SyncInputStream<std::vector<Subject>> subjectStream;
  SyncOutputStream<std::vector<std::string>> stringStream;
  SyncOutputStream<int> intStream;
  subjectStream = tester.client.sync_returnStreamWithStreams(
                  intStream, numItems, words, I, stringStream);

  for (auto& strings: randomStrings) {
    stringStream.put(strings);
  }
  stringStream.close();

  uint32_t i = 0;
  while (!subjectStream.isDone()) {
    std::vector<Subject> them;
    subjectStream.get(them);

    if (i < randomStrings.size()) {
      EXPECT_EQ(them.size(), randomStrings[i].size());

      if (them.size() == randomStrings[i].size()) {
        for (uint32_t j = 0; j < randomStrings[i].size(); ++j) {
          EXPECT_EQ(them[j].will.be, randomStrings[i][j]);
        }
      }
    }

    ++i;
  }

  for (int i = 0; i < 20; ++i) {
    intStream.put(i * 100 + 10);
  }
  intStream.close();

  EXPECT_EQ(i, randomStrings.size());

  EXPECT_TRUE(intStream.isClosed());
  EXPECT_TRUE(stringStream.isClosed());
  EXPECT_TRUE(subjectStream.isDone());
  EXPECT_TRUE(subjectStream.isFinished());
  EXPECT_TRUE(subjectStream.isClosed());
}

TEST(StreamTest, AsyncInterleavedTest) {
  Tester tester;

  AsyncInputStream<int> ic(ICP(new InputCallback(10)));
  AsyncOutputStream<int> oc(OCP(new OutputCallback(20)));
  tester.client.simpleTwoWayStream(ic, oc);

  int expectedSum = 0;
  for (int i = 0; i < 10; ++i) {
    expectedSum += i * 100 + 10;
  }

  AsyncOutputStream<int> nums(OCP(new OutputCallback(10)));
  AsyncInputStream<int> sum(ICP(new InputSingletonCallback(expectedSum)));
  tester.client.sum(sum, nums);

  tester.client.sendResponse([](ClientReceiveState&& state) {
    std::string response;
    StreamServiceAsyncClient::recv_sendResponse(response, state);
    EXPECT_EQ("1234", response);
  }, 1234);

  tester.loop();
}

TEST(StreamTest, SyncInterleavedTest) {
  Tester tester;

  std::string response;

  SyncOutputStream<int32_t> nums;
  StreamSingleton<int32_t> sum = tester.client.sync_sum(nums);

  tester.client.sync_sendResponse(response, 1001);
  EXPECT_EQ(response, "1001");

  int mySum = 0;
  for (int i = 0; i < 10; ++i) {
    nums.put(i * 100 + 10);
    mySum += i * 100 + 10;
  }

  tester.client.sync_sendResponse(response, 42);
  EXPECT_EQ(response, "42");

  SyncInputStream<int32_t> is;
  SyncOutputStream<int32_t> os;
  is = tester.client.sync_simpleTwoWayStream(os);

  int i = 0;
  while (!is.isDone()) {
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }
  EXPECT_EQ(i, 10);

  nums.close();
  EXPECT_TRUE(nums.isClosed());

  for (int i = 0; i < 20; ++i) {
    os.put(i * 100 + 10);
  }
  os.close();

  EXPECT_TRUE(os.isClosed());
  EXPECT_TRUE(is.isDone());
  EXPECT_TRUE(is.isFinished());
  EXPECT_TRUE(is.isClosed());

  int result;
  sum.get(result);

  EXPECT_EQ(mySum, result);

  EXPECT_TRUE(nums.isClosed());
}

TEST(StreamTest, UnexpectedStreamTest) {
  apache::thrift::util::ScopedServerThread server(getServer<SimilarService>());
  ClientTester<StreamServiceAsyncClient> tester(server.getAddress()->getPort());

  // we are calling a function on the sever that expects streams
  // but we are not providing any streams
  tester.client.sync_doNothing();

  // we are calling a function on the sever that does not expect any streams
  // but we are providing streams
  AsyncOutputStream<int> aos(OCP(new OutputReceiveClose(0)));
  AsyncInputStream<int> ais(ICP(new InputCallback(0)));
  tester.client.ignoreStreams(ais, aos);
  tester.loop();

  // we are calling a function on the sever that does not expect any streams
  // but we are providing streams
  SyncOutputStream<int> sos;
  SyncInputStream<int> sis = tester.client.sync_ignoreStreams(sos);
  EXPECT_TRUE(sos.isClosed());
  EXPECT_TRUE(sis.isClosed());
  EXPECT_TRUE(sis.isDone());
  EXPECT_TRUE(sis.isFinished());
}

TEST(StreamTest, SyncAsyncMixedTest) {
  Tester tester;

  int numItems = 10;
  std::vector<std::string> words = {"one", "two", "three", "four", "five"};

  Subject microsoft;
  microsoft.will.be = "huge";

  RpcOptions options;

  SyncOutputStream<int> ints;
  AsyncOutputStream<std::string> strings(OSCP(new OutputStringCallback(words)));
  tester.client.doNothingWithStreams(
      options,
      ints.makeHandler<StreamServiceStreamSerializer>(),
      numItems,
      words,
      microsoft,
      strings.makeHandler<StreamServiceStreamSerializer>(),
      nullptr);

  for (int i = 0; i < numItems; ++i) {
    ints.put(i * 100 + 10);
  }
  ints.close();

  EXPECT_TRUE(ints.isClosed());
}

TEST(StreamTest, NotSetTest) {
  Tester tester;

  int num = 1729;
  std::vector<std::string> words = {
    "alpha", "beta", "gamma", "delta", "epsilon"
  };

  Subject there;
  there.will.be = "blood";

  RpcOptions options;

  SyncOutputStream<int> ints;
  SyncOutputStream<std::string> strings;
  StreamSingleton<int16_t> future;
  future = tester.client.sync_returnSimpleWithStreams(
      ints, num, words, there, strings);

  for (auto& word: words) {
    strings.put(word);
  }
  strings.close();

  int16_t result;
  EXPECT_EXCEPTION(future.get(result), "No item available.");

  EXPECT_TRUE(strings.isClosed());

  // we can't actually check that ints.isClosed(), because it is possible that
  // the server yet hasn't sent back the close flag for this unexpected stream
  //EXPECT_TRUE(ints.isClosed());
}
