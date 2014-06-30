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
#include "MockServer.h"

#include <gtest/gtest.h>
#include <memory>
#include <utility>

#include <thrift/lib/cpp2/test/stream/gen-cpp2/stream_types.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::test::cpp2;

MockProcessor::MockProcessor(const StreamMaker& streamMaker)
  : streamMaker_(streamMaker) {
}

void MockProcessor::process(std::unique_ptr<ResponseChannel::Request> req,
                            std::unique_ptr<folly::IOBuf> requestBuf,
                            PROTOCOL_TYPES protType,
                            Cpp2ConnContext* context,
                            TEventBase* eb,
                            ThreadManager* tm) {
  ProtocolType type = static_cast<ProtocolType>(protType);

  std::string msg;
  if (type == apache::thrift::protocol::T_BINARY_PROTOCOL) {
    msg = readBuffer<BinaryProtocolReader>(std::move(requestBuf));
  } else if (type == apache::thrift::protocol::T_COMPACT_PROTOCOL) {
    msg = readBuffer<CompactProtocolReader>(std::move(requestBuf));
  } else {
    EXPECT_TRUE(false);
  }

  auto streams = streamMaker_(msg, protType, context, eb, tm);

  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());

  std::string reply = "You talkin' to me?";
  std::unique_ptr<folly::IOBuf> replyBuf;
  if (type == apache::thrift::protocol::T_BINARY_PROTOCOL) {
    replyBuf = writeBuffer<BinaryProtocolWriter>(reply);
  } else if (type == apache::thrift::protocol::T_COMPACT_PROTOCOL) {
    replyBuf = writeBuffer<CompactProtocolWriter>(reply);
  } else {
    EXPECT_TRUE(false);
  }

  queue.append(std::move(replyBuf));
  req->sendReplyWithStreams(queue.move(), std::move(streams));
}

bool MockProcessor::isOnewayMethod(
    const folly::IOBuf* buf,
    const apache::thrift::transport::THeader* header) {
  return false;
}

template <typename Reader>
std::string MockProcessor::readBuffer(std::unique_ptr<folly::IOBuf>&& buffer) {
  Reader reader;
  reader.setInput(buffer.get());

  std::string messageName;
  MessageType messageType;
  int32_t seqId;
  Param param;

  reader.readMessageBegin(messageName, messageType, seqId);
  param.read(&reader);
  reader.readMessageEnd();

  EXPECT_EQ(messageName, "TEST_CALL");
  EXPECT_EQ(messageType, T_CALL);
  EXPECT_EQ(seqId, 0);

  return param.msg;
}

template <typename Writer>
std::unique_ptr<folly::IOBuf> MockProcessor::writeBuffer(const std::string& m) {
  Writer writer;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  apache::thrift::test::cpp2::Param param;

  param.msg = m;

  writer.setOutput(&queue);
  writer.writeMessageBegin("TEST_REPLY", T_REPLY, 0);
  param.write(&writer);
  writer.writeMessageEnd();

  return queue.move();
}

MockService::MockService(const StreamMaker& streamMaker)
  : streamMaker_(streamMaker) {
}

std::unique_ptr<AsyncProcessor> MockService::getProcessor() {
  return std::unique_ptr<MockProcessor>(new MockProcessor(streamMaker_));
}

MockServer::MockServer(const StreamMaker& streamMaker,
                       std::chrono::milliseconds idleTimeout)
  : server_(std::make_shared<ThriftServer>()) {

  std::unique_ptr<ServerInterface> unique(new MockService(streamMaker));
  server_->setPort(0);
  server_->setIdleTimeout(idleTimeout);
  server_->setInterface(std::move(unique));
  server_->setSaslEnabled(true);
  server_->setSaslServerFactory([] (TEventBase* evb) {
      return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
  });

  serverThread_.start(server_);
}

void MockServer::setTaskTimeout(std::chrono::milliseconds timeout) {
  server_->setTaskExpireTime(timeout);
}

int MockServer::getPort() {
  return serverThread_.getAddress()->getPort();
}
