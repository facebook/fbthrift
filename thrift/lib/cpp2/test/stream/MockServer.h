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
#ifndef THRIFT_TEST_STREAM_MOCKSERVER_H_
#define THRIFT_TEST_STREAM_MOCKSERVER_H_ 1

#include <thrift/lib/cpp2/async/Stream.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

#include <string>
#include <utility>
#include <vector>
#include <chrono>

namespace apache { namespace thrift {

typedef std::function<std::unique_ptr<StreamManager>(
    const std::string& msg,
    apache::thrift::protocol::PROTOCOL_TYPES protType,
    Cpp2ConnContext* context,
    apache::thrift::async::TEventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm)> StreamMaker;

class MockProcessor : public AsyncProcessor {
  private:
    template <typename Reader>
    static std::string readBuffer(std::unique_ptr<folly::IOBuf>&& buffer);

    template <typename Writer>
    static std::unique_ptr<folly::IOBuf> writeBuffer(const std::string& msg);

  public:
    explicit MockProcessor(const StreamMaker& streamMaker);

    void process(std::unique_ptr<ResponseChannel::Request> req,
                 std::unique_ptr<folly::IOBuf> buf,
                 apache::thrift::protocol::PROTOCOL_TYPES protType,
                 Cpp2ConnContext* context,
                 apache::thrift::async::TEventBase* eb,
                 apache::thrift::concurrency::ThreadManager* tm) override;

    bool isOnewayMethod(const folly::IOBuf* buf,
                 const apache::thrift::transport::THeader* header) override;

  private:
    StreamMaker streamMaker_;
};

class MockService : public ServerInterface {
  public:
    explicit MockService(const StreamMaker& sinkMaker);
    std::unique_ptr<AsyncProcessor> getProcessor() override;

  private:
    StreamMaker streamMaker_;
};

class MockServer {
  public:
    explicit MockServer(
        const StreamMaker& streamMaker,
        std::chrono::milliseconds idleTimeout = std::chrono::seconds(60));

    void setTaskTimeout(std::chrono::milliseconds timeout);
    int getPort();

  private:
    std::shared_ptr<ThriftServer> server_;
    apache::thrift::util::ScopedServerThread  serverThread_;
    void sleepALittle();
};

}}

#endif
