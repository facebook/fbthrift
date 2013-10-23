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
#include <functional>

#include "thrift/lib/cpp2/async/Stream.h"
#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp2/test/stream/gen-cpp2/stream_types.h"
#include "thrift/lib/cpp2/test/stream/MockCallbacks.h"

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using apache::thrift::protocol::TType;

struct GeneratedStreamConstants {
  static const uint32_t INT_STREAM_ID;
};

const uint32_t GeneratedStreamConstants::INT_STREAM_ID = 1;

typedef std::unique_ptr<InputStreamCallback<int>> InputCallbackPtr;
typedef std::unique_ptr<OutputStreamCallback<int>> OutputCallbackPtr;
typedef std::function<void(StreamManager& manager)> LoopCallback;

std::vector<OutputError*> outputErrorCallbacks;
std::vector<InputError*> inputErrorCallbacks;

class MockChannel : public StreamChannelCallback,
                    public MockEventBaseCallback {
  public:
    MockChannel(StreamSource::StreamMap&& sourceMap,
                StreamSink::StreamMap&& sinkMap,
                bool isSync,
                const LoopCallback& loopCallback)
      : manager_(this,
                 std::move(sourceMap),
                 Service::methodExceptionDeserializer,
                 std::move(sinkMap),
                 Service::methodExceptionSerializer),
        loopCallback_(loopCallback),
        stoppedPtr_(nullptr),
        deletedPtr_(nullptr),
        runningLoop_(false),
        hasSent_(false),
        hasReceived_(false),
        isSync_(isSync) {
      manager_.setChannelCallback(this);
      manager_.setInputProtocol(ProtocolType::T_BINARY_PROTOCOL);
      manager_.setOutputProtocol(ProtocolType::T_BINARY_PROTOCOL);
    }

    void onStreamSend(std::unique_ptr<folly::IOBuf>&& data) {
      ASSERT_FALSE(hasSent_);
      ASSERT_TRUE(static_cast<bool>(data));

      if (hasReceived_) {
        ASSERT_TRUE(static_cast<bool>(data_));
        data_->prependChain(std::move(data));

      } else {
        ASSERT_FALSE(static_cast<bool>(data_));
        data_ = std::move(data);
      }

      hasSent_ = true;
      hasReceived_ = true;
    }

    void onOutOfLoopStreamError(const std::exception_ptr& error) {
      delete this;
    }

    void run() {
      runningLoop_ = !isSync_;
      manager_.notifySend();
      runEventBaseLoop();
    }

    void startEventBaseLoop() {
      bool sync = isSync_;
      bool stopped = false;
      stoppedPtr_ = &stopped;
      bool deleted = false;
      deletedPtr_ = &deleted;

      runningLoop_ = true;
      runEventBaseLoop();

      if (sync && deleted) {
        EXPECT_TRUE(stopped);
      }
    }

    void runEventBaseLoop() {
      while (runningLoop_) {
        if (hasSent_) {
          hasSent_ = false;
          manager_.notifySend();
        }

        if (manager_.isDone()) {
          delete this;
          return;
        }

        if (hasReceived_) {
          hasReceived_ = false;

          std::unique_ptr<folly::IOBuf> temp;
          std::swap(data_, temp);

          manager_.notifyReceive(std::move(temp));
        }

        if (manager_.isDone()) {
          delete this;
          return;
        }

        if (loopCallback_) {
          loopCallback_(manager_);
        }

        if (manager_.isDone()) {
          delete this;
          return;
        }
      }
    }

    void stopEventBaseLoop() {
      runningLoop_ = false;
      if (stoppedPtr_ != nullptr) {
        *stoppedPtr_ = true;
      }
    }

    StreamManager* getStreamManager() {
      return &manager_;
    }

    ~MockChannel() {
      if (deletedPtr_ != nullptr) {
        *deletedPtr_ = true;
      }
    }

  private:
    StreamManager manager_;
    std::unique_ptr<folly::IOBuf> data_;
    LoopCallback loopCallback_;
    bool* stoppedPtr_;
    bool* deletedPtr_;
    bool runningLoop_;
    bool hasSent_;
    bool hasReceived_;
    bool isSync_;
};

void runMockCall(std::unique_ptr<InputStreamCallbackBase>&& icb,
                 std::unique_ptr<OutputStreamCallbackBase>&& ocb,
                 const LoopCallback& loopCallback) {

  bool isSync = icb->isSync() || ocb->isSync();

  StreamSource::StreamMap sourceMap;
  icb->setType(TType::T_I32);
  sourceMap[GeneratedStreamConstants::INT_STREAM_ID] = std::move(icb);

  StreamSink::StreamMap sinkMap;
  ocb->setType(TType::T_I32);
  sinkMap[GeneratedStreamConstants::INT_STREAM_ID] = std::move(ocb);

  MockChannel* channel = new MockChannel(std::move(sourceMap),
                                         std::move(sinkMap),
                                         isSync,
                                         loopCallback);

  for (auto callback: outputErrorCallbacks) {
    callback->setStreamManager(channel->getStreamManager());
  }
  outputErrorCallbacks.clear();

  for (auto callback: inputErrorCallbacks) {
    callback->setStreamManager(channel->getStreamManager());
  }
  inputErrorCallbacks.clear();

  channel->run();
}

void runMockCall(AsyncInputStream<int>& ais,
                 AsyncOutputStream<int>& aos,
                 const LoopCallback& loopCallback = nullptr) {
  runMockCall(ais.makeHandler<Service>(),
              aos.makeHandler<Service>(),
              loopCallback);
}

void runMockCall(AsyncOutputStream<int>& aos,
                 const LoopCallback& loopCallback = nullptr) {
  InputCallbackPtr ic(new DummyInputCallback());
  AsyncInputStream<int> ais(std::move(ic));
  runMockCall(ais, aos, loopCallback);
}

void runMockCall(SyncInputStream<int>& sis,
                 SyncOutputStream<int>& sos,
                 const LoopCallback& loopCallback = nullptr) {
  runMockCall(sis.makeHandler<Service>(),
              sos.makeHandler<Service>(),
              loopCallback);
}

void runMockCall(InputCallbackPtr&& ic,
                 SyncOutputStream<int>& sos,
                 const LoopCallback& loopCallback = nullptr) {
  AsyncInputStream<int> ais(std::move(ic));

  runMockCall(ais.makeHandler<Service>(),
              sos.makeHandler<Service>(),
              loopCallback);
}

void runMockCall(SyncInputStream<int>& sis,
                 OutputCallbackPtr&& oc,
                 const LoopCallback& loopCallback = nullptr) {
  AsyncOutputStream<int> aos(std::move(oc));

  runMockCall(sis.makeHandler<Service>(),
              aos.makeHandler<Service>(),
              loopCallback);
}

TEST(StreamClasses, AsyncOutput) {
  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));
  runMockCall(aos);
}

TEST(StreamClasses, AsyncOutputException) {
  OutputCallbackPtr oc(new OutputException(10));
  AsyncOutputStream<int> aos(std::move(oc));
  runMockCall(aos);
}

TEST(StreamClasses, AsyncOutputError) {
  auto oec = new OutputError(10);
  outputErrorCallbacks.push_back(oec);
  OutputCallbackPtr oc(oec);
  AsyncOutputStream<int> aos(std::move(oc));
  runMockCall(aos);
}

TEST(StreamClasses, AsyncInput) {
  OutputCallbackPtr oc(new OutputCallback(10));
  AsyncOutputStream<int> aos(std::move(oc));

  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));

  runMockCall(ais, aos);
}

TEST(StreamClasses, AsyncInputException) {
  OutputCallbackPtr oc(new OutputException(10));
  AsyncOutputStream<int> aos(std::move(oc));
  auto ol = aos.makeController();

  InputCallbackPtr ic(new InputReceiveException(10));
  AsyncInputStream<int> ais(std::move(ic));
  auto il = ais.makeController();

  runMockCall(ais, aos);

  EXPECT_TRUE(ol->isClosed());
  EXPECT_TRUE(il->isClosed());
}

TEST(StreamClasses, AsyncInputError) {
  OutputCallbackPtr oc(new OutputReceiveError());
  AsyncOutputStream<int> aos(std::move(oc));
  auto ol = aos.makeController();

  auto iec = new InputError(10);
  inputErrorCallbacks.push_back(iec);
  InputCallbackPtr ic(iec);
  AsyncInputStream<int> ais(std::move(ic));
  auto il = ais.makeController();

  runMockCall(ais, aos);

  EXPECT_TRUE(ol->isClosed());
  EXPECT_TRUE(il->isClosed());
}

TEST(StreamClasses, AsyncInputClose) {
  // 21 represents the maximum number items that should be output
  // before the close signal from the input gets received this
  // number is 21 due to the batching method used in OutputReceiveClose
  // namely: 1 + 2 + 3 + 4 + 5 + 6 = 21
  OutputCallbackPtr oc(new OutputReceiveClose(21));
  AsyncOutputStream<int> aos(std::move(oc));

  InputCallbackPtr ic(new InputClose(10));
  AsyncInputStream<int> ais(std::move(ic));

  runMockCall(ais, aos);
}

TEST(StreamClasses, AsyncOutputController) {
  AsyncOutputStream<int> aos;
  auto ol = aos.makeController();

  InputCallbackPtr ic(new InputCallback(10));
  AsyncInputStream<int> ais(std::move(ic));

  int i = 0;
  bool put = false;
  bool closed = false;
  bool afterClosed = false;
  auto lc = [&] (StreamManager& manager) {
    if (i < 10) {
      EXPECT_FALSE(ol->isClosed());
      ol->put(i * 100 + 10);
      ++i;
      put = true;

    } else if (!ol->isClosed()) {
      EXPECT_EQ(10, i);
      ol->close();
      EXPECT_TRUE(ol->isClosed());
      closed = true;

    } else {
      EXPECT_EXCEPTION(ol->put(11), "Stream has already been closed.");

      ol->close();
      EXPECT_TRUE(ol->isClosed());
      afterClosed = true;
    }
  };

  runMockCall(ais, aos, lc);

  EXPECT_TRUE(put);
  EXPECT_TRUE(closed);
  EXPECT_TRUE(afterClosed);
}

TEST(StreamClasses, AsyncInputController) {
  OutputCallbackPtr oc(new OutputReceiveClose(50));
  AsyncOutputStream<int> aos(std::move(oc));

  InputCallbackPtr ic(new InputReceiveClose(50));
  AsyncInputStream<int> ais(std::move(ic));
  auto il = ais.makeController();

  int i = 0;
  bool closed = false;
  bool afterClosed = false;
  auto lc = [&] (StreamManager& manager) {
    if (i < 5) {
      ++i;

    } else if (!il->isClosed()) {
      EXPECT_EQ(i, 5);
      il->close();
      EXPECT_TRUE(il->isClosed());
      closed = true;

    } else {
      il->close();
      EXPECT_TRUE(il->isClosed());
      afterClosed = true;
    }
  };

  runMockCall(ais, aos, lc);

  EXPECT_TRUE(closed);
  EXPECT_TRUE(afterClosed);
}

TEST(StreamClasses, AsyncUsage) {
  OutputCallbackPtr oc1(new DummyOutputCallback());
  OutputCallbackPtr oc2(new OutputCallback(10));
  OutputCallbackPtr oc3(new DummyOutputCallback());
  AsyncOutputStream<int> aos;
  aos.setCallback(std::move(oc1));
  aos.setCallback(nullptr);
  aos.setCallback(std::move(oc2));

  auto ol1 = aos.makeController();

  EXPECT_EXCEPTION(ol1->close(), "Stream has not been used in a call.");
  EXPECT_EXCEPTION(ol1->put(1), "Stream has not been used in a call.");

  auto ol2 = aos.makeController();

  InputCallbackPtr ic1(new DummyInputCallback());
  InputCallbackPtr ic2(new InputCallback(10));
  InputCallbackPtr ic3(new DummyInputCallback());
  AsyncInputStream<int> ais;
  ais.setCallback(std::move(ic1));
  ais.setCallback(nullptr);
  ais.setCallback(std::move(ic2));

  auto il1 = ais.makeController();

  EXPECT_EXCEPTION(il1->close(), "Stream has not been used in a call.");

  auto il2 = ais.makeController();

  runMockCall(ais, aos);

  EXPECT_EXCEPTION(aos.setCallback(std::move(oc3)),
                   "Stream has already been used in a call.");
  EXPECT_EXCEPTION(ais.setCallback(std::move(ic3)),
                   "Stream has already been used in a call.");

  EXPECT_EXCEPTION(aos.makeController(),
                   "Stream has already been used in a call.");

  EXPECT_EXCEPTION(ais.makeController(),
                   "Stream has already been used in a call.");
}

TEST(StreamClasses, SyncOutput) {
  SyncOutputStream<int> os;
  InputCallbackPtr ic(new InputCallback(10));

  runMockCall(std::move(ic), os);

  // this is the most likely usage of the code
  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }

  // the stream is closed in the destructor
}

TEST(StreamClasses, SyncOutputMove) {
  SyncOutputStream<int> os;
  InputCallbackPtr ic(new InputCallback(10));

  runMockCall(std::move(ic), os);

  SyncOutputStream<int> * stream = &os;

  auto createdStream = new SyncOutputStream<int>();
  auto moveAssignedStream = new SyncOutputStream<int>();

  // creation using linked
  auto moveConstructedStream(std::move(*stream));

  // moving linked into unlinked then to itself
  (*moveAssignedStream = std::move(moveConstructedStream))
    = std::move(*moveAssignedStream);

  // moving unlinked into unlined
  *createdStream = std::move(moveConstructedStream);

  // moving unlinked into linked
  *moveAssignedStream = std::move(*createdStream);

  stream = createdStream;

  for (int i = 0; i < 10; ++i) {
    stream->put(i * 100 + 10);
  }

  EXPECT_FALSE(stream->isClosed());
  delete moveAssignedStream;

  // deleting the stream will automatically close the stream
  EXPECT_FALSE(stream->isClosed());
  delete createdStream;

  // the stream should have been deleted here
}

TEST(StreamClasses, SyncOutputException) {
  SyncOutputStream<int> os;
  InputCallbackPtr ic(new InputReceiveException(10));

  runMockCall(std::move(ic), os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }

  EXPECT_FALSE(os.isClosed());

  SimpleException exception;
  exception.msg = "Simple Exception";
  os.putException(std::make_exception_ptr(exception));

  EXPECT_TRUE(os.isClosed());
}

TEST(StreamClasses, SyncOutputError) {
  SyncOutputStream<int> os;
  auto iec = new InputError(10);
  inputErrorCallbacks.push_back(iec);
  InputCallbackPtr ic(iec);

  runMockCall(std::move(ic), os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }

  EXPECT_THROW(os.put(11 * 100 + 10), TestException);
  EXPECT_TRUE(os.isClosed());

  EXPECT_EXCEPTION(os.put(12 * 100 + 10), "Stream has already been closed.");

  EXPECT_TRUE(os.isClosed());
  os.close();
  EXPECT_TRUE(os.isClosed());
}

TEST(StreamClasses, SyncOutputPrecall) {
  SyncOutputStream<int> os;
  InputCallbackPtr ic(new DummyInputCallback());

  EXPECT_EXCEPTION(os.put(11 * 100 + 10),
                   "Stream has not been used in a call.");
  EXPECT_FALSE(os.isClosed());

  EXPECT_EXCEPTION(os.putException(std::make_exception_ptr("A random string.")),
                   "Stream has not been used in a call.");
  EXPECT_FALSE(os.isClosed());

  EXPECT_EXCEPTION(os.close(), "Stream has not been used in a call.");
  EXPECT_FALSE(os.isClosed());

  runMockCall(std::move(ic), os);
  EXPECT_FALSE(os.isClosed());

  os.close();
  EXPECT_TRUE(os.isClosed());

  os.close();
  EXPECT_TRUE(os.isClosed());

  EXPECT_EXCEPTION(os.put(12 * 100 + 10), "Stream has already been closed.");
  EXPECT_TRUE(os.isClosed());

  EXPECT_EXCEPTION(os.putException(std::make_exception_ptr("A random string.")),
                   "Stream has already been closed.");
  EXPECT_TRUE(os.isClosed());
}

TEST(StreamClasses, SyncInput) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputCallback(10));

  runMockCall(is, std::move(oc));

  int i = 0;
  while (!is.isDone()) {
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }

  EXPECT_EQ(i, 10);

  EXPECT_TRUE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputMove) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputCallback(10));

  runMockCall(is, std::move(oc));

  SyncInputStream<int> * stream = &is;

  auto createdStream = new SyncInputStream<int>();
  auto moveAssignedStream = new SyncInputStream<int>();

  // creation using linked
  auto moveConstructedStream(std::move(*stream));

  // moving linked into unlinked then to itself
  (*moveAssignedStream = std::move(moveConstructedStream))
    = std::move(*moveAssignedStream);

  // moving unlinked into unlined
  *createdStream = std::move(moveConstructedStream);

  // moving unlinked into linked
  *moveAssignedStream = std::move(*createdStream);

  stream = createdStream;

  EXPECT_FALSE(stream->isDone());
  EXPECT_FALSE(stream->isFinished());
  EXPECT_FALSE(stream->isClosed());

  int i = 0;
  while (!stream->isDone()) {
    EXPECT_FALSE(stream->isDone());
    EXPECT_FALSE(stream->isFinished());
    EXPECT_FALSE(stream->isClosed());

    int value;
    stream->get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }

  EXPECT_EQ(i, 10);

  EXPECT_TRUE(stream->isDone());
  EXPECT_TRUE(stream->isFinished());
  EXPECT_TRUE(stream->isClosed());

  delete moveAssignedStream;
  delete createdStream;
}

TEST(StreamClasses, SyncInputException) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputException(10));

  runMockCall(is, std::move(oc));

  int value;

  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(is.isDone());
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  EXPECT_FALSE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_FALSE(is.isClosed());

  try {
    is.get(value);
    EXPECT_TRUE(false);

  } catch (const SimpleException& exception) {
    EXPECT_EQ(exception.msg, "Simple Exception");

  } catch (...) {
    EXPECT_TRUE(false);
  }

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());

  EXPECT_EXCEPTION(is.get(value), "Stream has already been closed.");

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());

  is.close();

  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputError) {
  SyncInputStream<int> is;

  auto oec = new OutputError(10);
  outputErrorCallbacks.push_back(oec);
  OutputCallbackPtr oc(oec);

  runMockCall(is, std::move(oc));

  int value;

  int i = 0;

  EXPECT_EXCEPTION(is.get(value), "No item available.");

  EXPECT_FALSE(is.isFinished());
  EXPECT_FALSE(is.isClosed());

  while (i < 10 && !is.isDone()) {
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
    ++i;
  }

  EXPECT_EQ(i, 10);
  EXPECT_THROW(is.isDone(), TestException);

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());

  EXPECT_EXCEPTION(is.get(value), "Stream has already been closed.");

  is.close();

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputClose) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputReceiveClose(6));

  runMockCall(is, std::move(oc));

  // the destructor of the input stream automatically calls close
}

TEST(StreamClasses, SyncInputCloseWithData) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputReceiveClose(6));

  runMockCall(is, std::move(oc));

  // this call loads the item
  EXPECT_FALSE(is.isDone());

  EXPECT_FALSE(is.isFinished());
  EXPECT_FALSE(is.isClosed());

  is.close();

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputPrecall) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new DummyOutputCallback());

  int value;

  EXPECT_EXCEPTION(is.isDone(), "Stream has not been used in a call.");
  EXPECT_FALSE(is.isClosed());

  EXPECT_EXCEPTION(is.get(value), "Stream has not been used in a call.");
  EXPECT_FALSE(is.isClosed());

  EXPECT_EXCEPTION(is.close(), "Stream has not been used in a call.");
  EXPECT_FALSE(is.isClosed());

  runMockCall(is, std::move(oc));
  EXPECT_FALSE(is.isClosed());

  is.close();
  EXPECT_TRUE(is.isClosed());

  EXPECT_TRUE(is.isDone());

  EXPECT_EXCEPTION(is.get(value), "Stream has already been closed.");
  EXPECT_TRUE(is.isClosed());

  is.close();
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputOutput) {
  SyncInputStream<int> is;
  SyncOutputStream<int> os;

  runMockCall(is, os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }

  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(is.isDone());
    EXPECT_FALSE(is.isFinished());
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  for (int i = 10; i < 20; ++i) {
    os.put(i * 100 + 10);

    EXPECT_FALSE(is.isDone());
    EXPECT_FALSE(is.isDone());
    EXPECT_FALSE(is.isDone());
    EXPECT_FALSE(is.isFinished());

    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  os.close();
  EXPECT_TRUE(is.isDone());
  EXPECT_TRUE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputOutputException) {
  SyncInputStream<int> is;
  SyncOutputStream<int> os;

  runMockCall(is, os);

  for (int i = 0; i < 10; ++i) {
    os.put(i * 100 + 10);
  }

  SimpleException exception;
  exception.msg = "Simple Exception";
  os.putException(std::make_exception_ptr(exception));

  EXPECT_TRUE(os.isClosed());

  int j = 0;
  try {
    while (!is.isDone()) {
      EXPECT_FALSE(is.isFinished());
      int value;
      is.get(value);
      EXPECT_EQ(value, j * 100 + 10);
      ++j;
    }
  } catch (const SimpleException& exception) {
    EXPECT_EQ(j, 10);
    EXPECT_EQ(exception.msg, "Simple Exception");
  } catch (...) {
    EXPECT_TRUE(false);
  }

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, SyncInputOutputExceptionClose) {
  SyncInputStream<int> is;
  OutputCallbackPtr oc(new OutputException(10));

  runMockCall(is, std::move(oc));

  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(is.isDone());
    EXPECT_FALSE(is.isFinished());
    int value;
    is.get(value);
    EXPECT_EQ(value, i * 100 + 10);
  }

  EXPECT_FALSE(is.isDone());
  EXPECT_FALSE(is.isDone());

  // we close before the exception gets thrown
  EXPECT_FALSE(is.isClosed());
  is.close();

  EXPECT_TRUE(is.isDone());
  EXPECT_FALSE(is.isFinished());
  EXPECT_TRUE(is.isClosed());
}

TEST(StreamClasses, AsyncCoop) {
  OutputCallbackPtr oc(new OutputCoopCallback(20));
  AsyncOutputStream<int> aos(std::move(oc));
  auto ol = aos.makeController();

  InputCallbackPtr ic(new InputCoopCallback(10, std::move(ol)));
  AsyncInputStream<int> ais(std::move(ic));

  runMockCall(ais, aos);
}
