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
#include "MockClient.h"

#include <gtest/gtest.h>

#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp2/protocol/CompactProtocol.h"
#include "thrift/lib/cpp2/test/stream/gen-cpp2/stream_types.h"

#include <utility>
#include <chrono>

using namespace apache::thrift;
using namespace apache::thrift::async;
using apache::thrift::protocol::TType;

namespace {
  struct ClientId {
    static const int16_t OUT_STREAM;
    static const int16_t IN_STREAM;
  };

  const int16_t ClientId::OUT_STREAM = 1;
  const int16_t ClientId::IN_STREAM = 2;
}

class ClientSerializer {
  private:
    static const int16_t METHOD_SIMPLE_EXCEPTION_ID = -1;

  public:
    template<typename Reader>
    static uint32_t deserialize(Reader& reader, int& value) {
      return reader.readI32(value);
    }

    static std::exception_ptr methodExceptionDeserializer(StreamReader& reader){
      int16_t exceptionId = reader.readExceptionId();
      switch (exceptionId) {
        case METHOD_SIMPLE_EXCEPTION_ID:
          return reader.readExceptionItem<
                    apache::thrift::test::cpp2::SimpleException>();
        default:
          reader.skipException();
          // TODO change this to return an TApplicationException instead of
          //      a runtime_error in the generated code
          return std::make_exception_ptr(std::runtime_error(
                    "Unknown exception received."));
      }
    }

    template <typename Writer>
    static uint32_t serialize(Writer& writer, const int& value) {
      return writer.writeI32(value);
    }

    static void methodExceptionSerializer(StreamWriter& writer,
                                          const std::exception_ptr& exception) {
      try {
        std::rethrow_exception(exception);
      } catch (const apache::thrift::test::cpp2::SimpleException& e) {
        writer.writeExceptionItem(METHOD_SIMPLE_EXCEPTION_ID, e);
      }
    }
};

MockClient::MockClient(int port)
    : channel_(new HeaderClientChannel(makeSocket(port))) {
  channel_->setTimeout(10000);
}

RequestChannel* MockClient::getChannel() {
    return channel_.get();
  }

SyncInputStream<int> MockClient::runInputOutputCall(AsyncOutputStream<int>& aos,
                                                    const std::string& msg) {
  return runInputOutputCall(aos.makeHandler<ClientSerializer>(), msg);
}

SyncInputStream<int> MockClient::runInputOutputCall(SyncOutputStream<int>& os,
                                                    const std::string& msg) {
  return runInputOutputCall(os.makeHandler<ClientSerializer>(), msg);
}

SyncInputStream<int> MockClient::runInputOutputCall(
    std::unique_ptr<OutputStreamCallbackBase>&& oc,
    const std::string& msg) {
  SyncInputStream<int> is;
  runInputOutputCall(is.makeHandler<ClientSerializer>(), std::move(oc), msg);
  base_.loopForever();
  return std::move(is);
}

void MockClient::runInputOutputCall(AsyncInputStream<int>& ais,
                                    AsyncOutputStream<int>& aos,
                                    const std::string& message) {
  runInputOutputCall(ais.makeHandler<ClientSerializer>(),
                     aos.makeHandler<ClientSerializer>(),
                     message);
}

void MockClient::runInputOutputCall(
    std::unique_ptr<InputStreamCallbackBase>&& icb,
    std::unique_ptr<OutputStreamCallbackBase>&& ocb,
    const std::string& msg) {

  bool isSync = false;

  StreamSource::StreamMap sourceMap;
  isSync = isSync || icb->isSync();
  icb->setType(TType::T_I32);
  sourceMap[ClientId::IN_STREAM] = std::move(icb);

  StreamSink::StreamMap sinkMap;
  isSync = isSync || ocb->isSync();
  ocb->setType(TType::T_I32);
  sinkMap[ClientId::OUT_STREAM] = std::move(ocb);

  TEventBase* eventBase = getChannel()->getEventBase();
  std::unique_ptr<StreamManager> stream(new StreamManager(
        eventBase,
        std::move(sourceMap),
        ClientSerializer::methodExceptionDeserializer,
        std::move(sinkMap),
        ClientSerializer::methodExceptionSerializer));

  ProtocolType type = static_cast<ProtocolType>(channel_->getProtocolId());
  stream->setOutputProtocol(type);

  runCall(std::move(stream), isSync, msg);
}

void MockClient::runInputCall(AsyncInputStream<int>& ais,
                              const std::string& msg) {
  runInputCall(ais.makeHandler<ClientSerializer>(), msg);
}

void MockClient::runInputCall(std::unique_ptr<InputStreamCallbackBase>&& icb,
                               const std::string& msg) {

  StreamSource::StreamMap sourceMap;
  bool isSync = icb->isSync();
  icb->setType(TType::T_I32);
  sourceMap[ClientId::IN_STREAM] = std::move(icb);

  StreamSink::StreamMap sinkMap;

  TEventBase* eventBase = getChannel()->getEventBase();
  std::unique_ptr<StreamManager> stream(new StreamManager(
        eventBase,
        std::move(sourceMap),
        ClientSerializer::methodExceptionDeserializer,
        std::move(sinkMap),
        ClientSerializer::methodExceptionSerializer));

  ProtocolType type = static_cast<ProtocolType>(channel_->getProtocolId());
  stream->setOutputProtocol(type);

  runCall(std::move(stream), isSync, msg);
}

void MockClient::runOutputCall(AsyncOutputStream<int>& aos,
                               const std::string& msg) {
  runOutputCall(aos.makeHandler<ClientSerializer>(), msg);
}

void MockClient::runOutputCall(std::unique_ptr<OutputStreamCallbackBase>&& ocb,
                               const std::string& msg) {
  StreamSource::StreamMap sourceMap;

  StreamSink::StreamMap sinkMap;
  bool isSync = ocb->isSync();
  ocb->setType(TType::T_I32);
  sinkMap[ClientId::OUT_STREAM] = std::move(ocb);

  TEventBase* eventBase = getChannel()->getEventBase();
  std::unique_ptr<StreamManager> stream(new StreamManager(
        eventBase,
        std::move(sourceMap),
        ClientSerializer::methodExceptionDeserializer,
        std::move(sinkMap),
        ClientSerializer::methodExceptionSerializer));

  ProtocolType type = static_cast<ProtocolType>(channel_->getProtocolId());
  stream->setOutputProtocol(type);

  runCall(std::move(stream), isSync);
}

void MockClient::runCall(std::unique_ptr<StreamManager>&& stream,
                         bool isSync,
                         const std::string& msg) {

  TEventBase* eventBase = getChannel()->getEventBase();

  for (auto callback: outputErrorCallbacks_) {
    callback->setStreamManager(stream.get());
  }
  outputErrorCallbacks_.clear();

  for (auto callback: inputErrorCallbacks_) {
    callback->setStreamManager(stream.get());
  }
  inputErrorCallbacks_.clear();

  std::unique_ptr<StreamCallback> callback(
      new StreamCallback(
        std::move(stream),
        eventBase,
        processReply,
        isSync));

  RpcOptions options;
  options.setStreaming(true);
  options.setTimeout(std::chrono::milliseconds(100));

  std::unique_ptr<apache::thrift::ContextStack> ctx = nullptr;

  std::unique_ptr<folly::IOBuf> buf;

  ProtocolType type = static_cast<ProtocolType>(channel_->getProtocolId());
  if (type == apache::thrift::protocol::T_BINARY_PROTOCOL) {
   buf = writeBuffer<BinaryProtocolWriter>(msg);
  } else if (type == apache::thrift::protocol::T_COMPACT_PROTOCOL) {
   buf = writeBuffer<CompactProtocolWriter>(msg);
  } else {
    EXPECT_TRUE(false);
  }

  channel_->sendRequest(options,
                        std::move(callback),
                        std::move(ctx),
                        std::move(buf));
}

void MockClient::runEventBaseLoop() {
  base_.loop();
}

std::shared_ptr<TAsyncSocket> MockClient::makeSocket(int port) {
  return TAsyncSocket::newSocket(&base_, "127.0.0.1", port);
}

void MockClient::registerOutputErrorCallback(OutputError* callback) {
  outputErrorCallbacks_.push_back(callback);
}

void MockClient::registerInputErrorCallback(InputError* callback) {
  inputErrorCallbacks_.push_back(callback);
}

MockClient::~MockClient() {
}

template <typename Reader>
std::string MockClient::readBuffer(std::unique_ptr<folly::IOBuf>&& buffer) {
  Reader reader;
  reader.setInput(buffer.get());

  std::string messageName;
  MessageType messageType;
  int32_t seqId;
  apache::thrift::test::cpp2::Param param;

  reader.readMessageBegin(messageName, messageType, seqId);
  param.read(&reader);
  reader.readMessageEnd();

  EXPECT_EQ(messageName, "TEST_REPLY");
  EXPECT_EQ(messageType, T_REPLY);
  EXPECT_EQ(seqId, 0);

  return param.msg;
}

template <typename Writer>
std::unique_ptr<folly::IOBuf> MockClient::writeBuffer(const std::string& msg) {
  Writer writer;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  apache::thrift::test::cpp2::Param param;

  param.msg = msg;

  writer.setOutput(&queue);
  writer.writeMessageBegin("TEST_CALL", T_CALL, 0);
  param.write(&writer);
  writer.writeMessageEnd();

  return queue.move();
}

void MockClient::processReply(std::unique_ptr<StreamManager>&& streamManager,
                              ClientReceiveState& state) {
  ProtocolType type = static_cast<ProtocolType>(state.protocolId());
  streamManager->setInputProtocol(type);

  std::string msg;
  if (type == apache::thrift::protocol::T_BINARY_PROTOCOL) {
    msg = readBuffer<BinaryProtocolReader>(state.extractBuf());
  } else if (type == apache::thrift::protocol::T_COMPACT_PROTOCOL) {
    msg = readBuffer<CompactProtocolReader>(state.extractBuf());
  } else {
    EXPECT_TRUE(false);
  }

  EXPECT_EQ(msg, "You talkin' to me?");

  state.setStreamManager(std::move(streamManager));
}
