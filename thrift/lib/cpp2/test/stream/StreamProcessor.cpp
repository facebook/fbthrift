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
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <time.h>
#include <errno.h>

#include "thrift/lib/cpp2/async/Stream.h"
#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp2/protocol/CompactProtocol.h"
#include "thrift/lib/cpp2/test/stream/MockCallbacks.h"
#include "thrift/lib/cpp2/test/stream/MockServer.h"
#include "thrift/lib/cpp2/test/stream/MockClient.h"
#include "thrift/lib/cpp2/test/stream/gen-cpp2/stream_types.h"

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::test::cpp2;

typedef std::unique_ptr<InputStreamCallback<int>> InputCallbackPtr;
typedef std::unique_ptr<OutputStreamCallback<int>> OutputCallbackPtr;

namespace {
  struct ServerId {
    static const int16_t IN_STREAM;
    static const int16_t OUT_STREAM;
  };

  const int16_t ServerId::IN_STREAM = 1;
  const int16_t ServerId::OUT_STREAM = 2;
}

void millisleep(long milliseconds) {
  time_t seconds = milliseconds / 1000;
  long nanoseconds = (milliseconds % 1000) * 1000000;

  timespec sleeptime = {seconds, nanoseconds};
  nanosleep(&sleeptime, nullptr);
}

bool wasReset(const TTransportException& e) {
  return (e.getType() == TTransportException::INTERNAL_ERROR) &&
         (e.getErrno() == ECONNRESET);
}

bool wasResetOrClosed(const TTransportException& e) {
  std::string message = e.what();
  return (message == "Channel Closed") || wasReset(e);
}

bool closedOrDestroyed(const TTransportException& e) {
  std::string message = e.what();
  return (message == "Channel Closed" ||
          message == "Channel Destroyed");
}

bool closedLocally(const TTransportException& e) {
  std::string message = e.what();
  return (e.getType() == TTransportException::END_OF_FILE) &&
         (message == "socket closed locally");
}

bool closedLocallyOrDestroyed(const TTransportException& e) {
  std::string message = e.what();
  return (message == "Channel Destroyed") || closedLocally(e);
}

std::unique_ptr<StreamManager>
makeStreamFromMaps(StreamSource::StreamMap&& sourceMap,
                   StreamSink::StreamMap&& sinkMap,
                   PROTOCOL_TYPES protType,
                   TEventBase* eb) {

  std::unique_ptr<StreamManager> streams(new StreamManager(
        eb,
        std::move(sourceMap),
        Service::methodExceptionDeserializer,
        std::move(sinkMap),
        Service::methodExceptionSerializer));

  streams->setInputProtocol(protType);
  streams->setOutputProtocol(protType);

  return streams;
}

std::unique_ptr<StreamManager> makeFromStreams(AsyncInputStream<int>& ais,
                                               AsyncOutputStream<int>& aos,
                                               PROTOCOL_TYPES protType,
                                               TEventBase* eb) {

  apache::thrift::protocol::TType type = apache::thrift::protocol::T_I32;

  StreamSource::StreamMap sourceMap;
  auto icb = ais.makeHandler<Service>();
  icb->setType(type);
  sourceMap[ServerId::IN_STREAM] = std::move(icb);

  StreamSink::StreamMap sinkMap;
  auto ocb = aos.makeHandler<Service>();
  ocb->setType(type);
  sinkMap[ServerId::OUT_STREAM] = std::move(ocb);

  return makeStreamFromMaps(std::move(sourceMap),
                            std::move(sinkMap),
                            protType,
                            eb);
}

std::unique_ptr<StreamManager> testSimpleStreams(const std::string& msg,
                                                 PROTOCOL_TYPES protType,
                                                 Cpp2ConnContext* context,
                                                 TEventBase* eb,
                                                 ThreadManager* tm) {
  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));

  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, InputOutput) {
  MockServer server(testSimpleStreams);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

TEST(StreamProcessor, SyncInputOutput) {
  MockServer server(testSimpleStreams);
  MockClient client(server.getPort());

  SyncOutputStream<int> os;
  SyncInputStream<int> is;
  is = client.runInputOutputCall(os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }
  os.close();

  int i = 0;
  while (!is.isDone()) {
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }

  EXPECT_EQ(i, 10);

  EXPECT_TRUE(os.isClosed());
  EXPECT_TRUE(is.isDone());
  EXPECT_TRUE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

std::unique_ptr<StreamManager> testClosingStreams(const std::string& msg,
                                                  PROTOCOL_TYPES protType,
                                                  Cpp2ConnContext* context,
                                                  TEventBase* eb,
                                                  ThreadManager* tm) {

  InputCallbackPtr ic(new InputClose(10));
  AsyncInputStream<int> ais(std::move(ic));

  // we need a high limit, so that it doesn't error out when testing
  // with valgrind enabled
  OutputCallbackPtr oc(new OutputReceiveClose(100000));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, InputCloses) {
  MockServer server(testClosingStreams);
  MockClient client(server.getPort());

  // we need a high limit, so that it doesn't error out when testing
  // with valgrind enabled
  OutputCallbackPtr oc(new OutputReceiveClose(100000));
  AsyncOutputStream<int> aos(std::move(oc));

  SyncInputStream<int> is = client.runInputOutputCall(aos);

  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(is.isDone());

    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  EXPECT_FALSE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_FALSE(is.isClosed());
  is.close();

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

std::unique_ptr<StreamManager> testException(const std::string& msg,
                                             PROTOCOL_TYPES protType,
                                             Cpp2ConnContext* context,
                                             TEventBase* eb,
                                             ThreadManager* tm) {

  InputCallbackPtr ic(new InputReceiveException(10));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputException(10));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, AsyncException) {
  MockServer server(testException);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputReceiveException(10));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputException(10));
  AsyncOutputStream<int> aos(std::move(oc));
  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

TEST(StreamProcessor, SyncExceptions) {
  MockServer server(testException);
  MockClient client(server.getPort());

  SyncOutputStream<int> os;
  SyncInputStream<int> is;
  is = client.runInputOutputCall(os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);

    EXPECT_FALSE(is.isDone());
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  SimpleException exception;
  exception.msg = "Simple Exception";
  os.putException(std::make_exception_ptr(exception));

  EXPECT_FALSE(is.isDone());
  try {
    int value;
    is.get(value);
    EXPECT_TRUE(false);
  } catch (const SimpleException& exception) {
    EXPECT_EQ(exception.msg, "Simple Exception");
  } catch (...) {
    EXPECT_TRUE(false);
  }

  EXPECT_TRUE(os.isClosed());
  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

std::unique_ptr<StreamManager> testMaybeExpectError(const std::string& msg,
                                                    PROTOCOL_TYPES protType,
                                                    Cpp2ConnContext* context,
                                                    TEventBase* eb,
                                                    ThreadManager* tm) {
  InputCallbackPtr ic;
  OutputCallbackPtr oc;
  if (msg == "async error") {
    ic.reset(new InputReceiveErrorCheck(wasResetOrClosed));
    oc.reset(new OutputReceiveErrorCheck(wasResetOrClosed));
  } else if (msg == "sync error") {
    ic.reset(new InputReceiveErrorMsg("Channel Closed"));
    oc.reset(new OutputReceiveErrorMsg("Channel Closed"));
  } else {
    ic.reset(new InputCallback(100));
    oc.reset(new OutputCallback(100));
  }

  AsyncInputStream<int> ais(std::move(ic));
  AsyncOutputStream<int> aos(std::move(oc));
  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, MultiInputOutput) {
  MockServer server(testMaybeExpectError);
  MockClient client(server.getPort());

  InputCallbackPtr ic1(new InputCallback(100));
  AsyncInputStream<int> ais1(std::move(ic1));
  OutputCallbackPtr oc1(new OutputCallback(100));
  AsyncOutputStream<int> aos1(std::move(oc1));

  client.runInputOutputCall(ais1, aos1);

  InputCallbackPtr ic2(new InputCallback(100));
  AsyncInputStream<int> ais2(std::move(ic2));
  OutputCallbackPtr oc2(new OutputCallback(100));
  AsyncOutputStream<int> aos2(std::move(oc2));

  client.runInputOutputCall(ais2, aos2);

  InputCallbackPtr ic3(new InputReceiveError());
  AsyncInputStream<int> ais3(std::move(ic3));

  auto oec = new OutputError(10);
  client.registerOutputErrorCallback(oec);
  OutputCallbackPtr oc3(oec);
  AsyncOutputStream<int> aos3(std::move(oc3));

  client.runInputOutputCall(ais3, aos3, "async error");

  client.runEventBaseLoop();
}

TEST(StreamProcessor, MultiSyncInputOutput) {
  MockServer server(testMaybeExpectError);
  MockClient client(server.getPort());

  SyncOutputStream<int> os1, os2, os3;
  SyncInputStream<int> is1, is2;
  is1 = client.runInputOutputCall(os1);
  is2 = client.runInputOutputCall(os2);

  auto iec = new InputError(10);
  client.registerInputErrorCallback(iec);
  InputCallbackPtr ic3(iec);
  AsyncInputStream<int> ais3(std::move(ic3));
  client.runInputOutputCall(ais3.makeHandler<Service>(),
                            os3.makeHandler<Service>(), "sync error");

  for (int i = 0; i < 50; ++i) {
    EXPECT_FALSE(is2.isDone());
    int value;
    is2.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  for (int i = 0; i < 50; ++i) {
    os1.put(i * 100 + 10);

    EXPECT_FALSE(is1.isDone());
    int value;
    is1.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  for (int i = 0; i < 50; ++i) {
    os2.put(i * 100 + 10);
  }

  for (int i = 50; i < 100; ++i) {
    os1.put(i * 100 + 10);
    os2.put(i * 100 + 10);

    EXPECT_FALSE(is1.isDone());

    int value;
    is1.get(value);
    EXPECT_EQ(value, i * 100 + 10);

    EXPECT_FALSE(is2.isDone());

    is2.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  EXPECT_FALSE(os1.isClosed());
  os1.close();
  EXPECT_TRUE(os1.isClosed());
  EXPECT_TRUE(is1.isDone());
  EXPECT_TRUE(is1.isFinished());
  EXPECT_TRUE(is1.isClosed());

  EXPECT_FALSE(os2.isClosed());
  os2.close();
  EXPECT_TRUE(os2.isClosed());
  EXPECT_TRUE(is2.isDone());
  EXPECT_TRUE(is2.isFinished());
  EXPECT_TRUE(is2.isClosed());

  EXPECT_TRUE(os3.isClosed());
}

std::unique_ptr<StreamManager> testInputError(const std::string& msg,
                                              PROTOCOL_TYPES protType,
                                              Cpp2ConnContext* context,
                                              TEventBase* eb,
                                              ThreadManager* tm) {
  auto iec = new InputError(10);
  InputCallbackPtr ic(iec);

  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveError());
  AsyncOutputStream<int> aos(std::move(oc));

  auto stream = makeFromStreams(ais, aos, protType, eb);
  iec->setStreamManager(stream.get());

  return std::move(stream);
}

TEST(StreamProcessor, ServerInputError) {
  MockServer server(testInputError);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputReceiveErrorMsg("Timed Out"));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorMsg("Timed Out"));
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testOutputError(const std::string& msg,
                                               PROTOCOL_TYPES protType,
                                               Cpp2ConnContext* context,
                                               TEventBase* eb,
                                               ThreadManager* tm) {
  InputCallbackPtr ic(new InputReceiveError());
  AsyncInputStream<int> ais(std::move(ic));

  auto oec = new OutputError(10);
  OutputCallbackPtr oc(oec);
  AsyncOutputStream<int> aos(std::move(oc));

  auto stream = makeFromStreams(ais, aos, protType, eb);
  oec->setStreamManager(stream.get());

  return std::move(stream);
}

TEST(StreamProcessor, ServerOutputError) {
  MockServer server(testOutputError);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputReceiveErrorMsg("Timed Out"));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorMsg("Timed Out"));
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testReceiveError(const std::string& msg,
                                                PROTOCOL_TYPES protType,
                                                Cpp2ConnContext* context,
                                                TEventBase* eb,
                                                ThreadManager* tm) {
  InputCallbackPtr ic(new InputReceiveErrorCheck(wasResetOrClosed));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorCheck(wasResetOrClosed));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, ClientInputError) {
  MockServer server(testReceiveError);
  MockClient client(server.getPort());

  auto iec = new InputError(10);
  client.registerInputErrorCallback(iec);
  InputCallbackPtr ic(iec);
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveError());
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

TEST(StreamProcessor, ClientOutputError) {
  MockServer server(testReceiveError);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputReceiveError());
  AsyncInputStream<int> ais(std::move(ic));

  auto oec = new OutputError(10);
  client.registerOutputErrorCallback(oec);
  OutputCallbackPtr oc(oec);
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testOnlyInput(const std::string& msg,
                                             PROTOCOL_TYPES protType,
                                             Cpp2ConnContext* context,
                                             TEventBase* eb,
                                             ThreadManager* tm) {

  apache::thrift::protocol::TType type = apache::thrift::protocol::T_I32;

  StreamSource::StreamMap sourceMap;

  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));

  auto icb = ais.makeHandler<Service>();
  icb->setType(type);
  sourceMap[ServerId::IN_STREAM] = std::move(icb);

  StreamSink::StreamMap sinkMap;

  return makeStreamFromMaps(std::move(sourceMap),
                            std::move(sinkMap),
                            protType,
                            eb);
}

TEST(StreamProcessor, OnlyOutputTest) {
  MockServer server(testOnlyInput);
  MockClient client(server.getPort());

  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));

  client.runOutputCall(aos);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testOnlyOutput(const std::string& msg,
                                              PROTOCOL_TYPES protType,
                                              Cpp2ConnContext* context,
                                              TEventBase* eb,
                                              ThreadManager* tm) {

  apache::thrift::protocol::TType type = apache::thrift::protocol::T_I32;

  StreamSource::StreamMap sourceMap;

  StreamSink::StreamMap sinkMap;

  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));

  auto ocb = aos.makeHandler<Service>();
  ocb->setType(type);
  sinkMap[ServerId::OUT_STREAM] = std::move(ocb);

  return makeStreamFromMaps(std::move(sourceMap),
                            std::move(sinkMap),
                            protType,
                            eb);
}

TEST(StreamProcessor, OnlyInputTest) {
  MockServer server(testOnlyOutput);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));

  client.runInputCall(ais);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testIgnoreStreams(const std::string& msg,
                                                 PROTOCOL_TYPES protType,
                                                 Cpp2ConnContext* context,
                                                 TEventBase* eb,
                                                 ThreadManager* tm) {
  InputCallbackPtr ic(new InputIgnore(0));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputIgnore(100000));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, StreamIgnoreTest) {
  MockServer server(testIgnoreStreams);
  MockClient client(server.getPort());

  int16_t expectedOutputId = 1;
  int16_t unexpectedInputId = 1111;

  InputCallbackPtr ic(new InputIgnore(0));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputIgnore(100000));
  AsyncOutputStream<int> aos(std::move(oc));

  auto icb = ais.makeHandler<Service>();
  icb->setType(apache::thrift::protocol::T_I32);

  StreamSource::StreamMap sourceMap;
  sourceMap[unexpectedInputId] = std::move(icb);

  auto ocb = aos.makeHandler<Service>();
  ocb->setType(apache::thrift::protocol::T_LIST);

  StreamSink::StreamMap sinkMap;
  sinkMap[expectedOutputId] = std::move(ocb);

  TEventBase* eventBase = client.getChannel()->getEventBase();
  std::unique_ptr<StreamManager> stream(new StreamManager(
        eventBase,
        std::move(sourceMap),
        Service::methodExceptionDeserializer,
        std::move(sinkMap),
        Service::methodExceptionSerializer));

  ProtocolType type = static_cast<ProtocolType>(
      client.getChannel()->getProtocolId());
  stream->setOutputProtocol(type);

  client.runCall(std::move(stream), false);
  client.runEventBaseLoop();
}

TEST(StreamProcessor, MultiStreamIgnoreTest) {
  MockServer server(testSimpleStreams);
  MockClient client(server.getPort());

  int16_t expectedInputId = 2;
  int16_t expectedOutputId = 1;
  int16_t randomInputId = 2222;
  int16_t randomOutputId = 3333;

  InputCallbackPtr ic1(new InputCallback(10));
  AsyncInputStream<int> ais1(std::move(ic1));
  InputCallbackPtr ic2(new InputIgnore(0));
  AsyncInputStream<int> ais2(std::move(ic2));
  OutputCallbackPtr oc1(new OutputCallback(10));
  AsyncOutputStream<int> aos1(std::move(oc1));
  OutputCallbackPtr oc2(new OutputIgnore(100000));
  AsyncOutputStream<int> aos2(std::move(oc2));

  auto icb1 = ais1.makeHandler<Service>();
  icb1->setType(apache::thrift::protocol::T_I32);

  auto icb2 = ais2.makeHandler<Service>();
  icb2->setType(apache::thrift::protocol::T_I32);

  StreamSource::StreamMap sourceMap;
  sourceMap[expectedInputId] = std::move(icb1);
  sourceMap[randomInputId] = std::move(icb2);

  auto ocb1 = aos1.makeHandler<Service>();
  ocb1->setType(apache::thrift::protocol::T_I32);

  auto ocb2 = aos2.makeHandler<Service>();
  ocb2->setType(apache::thrift::protocol::T_I32);

  StreamSink::StreamMap sinkMap;
  sinkMap[expectedOutputId] = std::move(ocb1);
  sinkMap[randomOutputId] = std::move(ocb2);

  TEventBase* eventBase = client.getChannel()->getEventBase();
  std::unique_ptr<StreamManager> stream(new StreamManager(
        eventBase,
        std::move(sourceMap),
        Service::methodExceptionDeserializer,
        std::move(sinkMap),
        Service::methodExceptionSerializer));

  ProtocolType type = static_cast<ProtocolType>(
      client.getChannel()->getProtocolId());
  stream->setOutputProtocol(type);

  client.runCall(std::move(stream), false);
  client.runEventBaseLoop();
}

std::unique_ptr<StreamManager> testTaskTimeout(const std::string& msg,
                                               PROTOCOL_TYPES protType,
                                               Cpp2ConnContext* context,
                                               TEventBase* eb,
                                               ThreadManager* tm) {
  InputCallbackPtr ic(new InputReceiveErrorMsg("Receive Expired"));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorMsg("Receive Expired"));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, ServerTaskTimeout) {
  MockServer server(testTaskTimeout);
  MockClient client(server.getPort());

  server.setTaskTimeout(std::chrono::milliseconds(100));

  SyncOutputStream<int> os;
  SyncInputStream<int> is = client.runInputOutputCall(os);

  millisleep(200);

  os.close();

  EXPECT_EXCEPTION(is.close(), "Timed Out");
}

std::unique_ptr<StreamManager> testIdleTimeout(const std::string& msg,
                                               PROTOCOL_TYPES protType,
                                               Cpp2ConnContext* context,
                                               TEventBase* eb,
                                               ThreadManager* tm) {
  InputCallbackPtr ic(new InputReceiveErrorCheck(closedLocallyOrDestroyed));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorCheck(closedLocallyOrDestroyed));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, ServerIdleTimeout) {
  std::chrono::milliseconds idleTimeout(100);

  MockServer server(testIdleTimeout, idleTimeout);
  MockClient client(server.getPort());

  SyncOutputStream<int> os;
  SyncInputStream<int> is = client.runInputOutputCall(os);

  millisleep(200);

  os.close();

  try {
    is.close();
    ADD_FAILURE() << "No exception thrown";
  } catch (const TTransportException& e) {
    EXPECT_TRUE(wasReset(e)) << e.what();
  } catch (...) {
    ADD_FAILURE() << "Unexpected exception thrown";
  }
}

std::unique_ptr<StreamManager> testClientTimeout(const std::string& msg,
                                                 PROTOCOL_TYPES protType,
                                                 Cpp2ConnContext* context,
                                                 TEventBase* eb,
                                                 ThreadManager* tm) {
  long sleepTime = 200;
  InputCallbackPtr ic(new InputReceiveErrorCheck(closedOrDestroyed));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputTimeoutCheck(sleepTime, closedOrDestroyed));
  AsyncOutputStream<int> aos(std::move(oc));

  return makeFromStreams(ais, aos, protType, eb);
}

TEST(StreamProcessor, ClientTimeout) {
  MockServer server(testClientTimeout);
  MockClient client(server.getPort());

  InputCallbackPtr ic(new InputReceiveErrorMsg("Timed Out"));
  AsyncInputStream<int> ais(std::move(ic));
  OutputCallbackPtr oc(new OutputReceiveErrorMsg("Timed Out"));
  AsyncOutputStream<int> aos(std::move(oc));

  client.runInputOutputCall(ais, aos);
  client.runEventBaseLoop();
}
