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
#ifndef THRIFT_TEST_STREAM_MOCKCLIENTS_H_
#define THRIFT_TEST_STREAM_MOCKCLIENTS_H_ 1

#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/test/stream/MockCallbacks.h>

#include <string>

namespace apache { namespace thrift {

class MockClient : public apache::thrift::TClientBase {
  private:
    template <typename Reader>
    static std::string readBuffer(std::unique_ptr<folly::IOBuf>&& buffer);

    template <typename Writer>
    static std::unique_ptr<folly::IOBuf> writeBuffer(const std::string& msg);

    static void processReply(std::unique_ptr<StreamManager>&& streamManager,
                             ClientReceiveState& state);
  public:
    typedef std::unique_ptr<HeaderClientChannel,
                            TDelayedDestruction::Destructor> channel_ptr;

    explicit MockClient(int port);

    RequestChannel* getChannel();

    void runInputOutputCall(AsyncInputStream<int>& ais,
                            AsyncOutputStream<int>& aos,
                            const std::string& msg = "InputOutputCall");
    void runInputOutputCall(std::unique_ptr<InputStreamCallbackBase>&& ic,
                            std::unique_ptr<OutputStreamCallbackBase>&& oc,
                            const std::string& msg = "InputOutputCall");

    SyncInputStream<int> runInputOutputCall(AsyncOutputStream<int>& aos,
            const std::string& msg = "InputOutputCall");
    SyncInputStream<int> runInputOutputCall(
        std::unique_ptr<OutputStreamCallbackBase>&& oc,
            const std::string& msg = "InputOutputCall");

    SyncInputStream<int> runInputOutputCall(SyncOutputStream<int>& os,
        const std::string& msg = "InputOutputCall");

    void runInputCall(AsyncInputStream<int>& ais,
                      const std::string& msg = "InputCall");
    void runInputCall(std::unique_ptr<InputStreamCallbackBase>&& ic,
                      const std::string& msg = "InputCall");

    void runOutputCall(AsyncOutputStream<int>& aos,
                       const std::string& msg = "OutputCall");
    void runOutputCall(std::unique_ptr<OutputStreamCallbackBase>&& oc,
                       const std::string& msg = "OutputCall");

    void runCall(std::unique_ptr<StreamManager>&& stream,
                 bool isSync,
                 const std::string& msg = "E.T. phone home.");

    void runEventBaseLoop();

    void registerOutputErrorCallback(OutputError* callback);
    void registerInputErrorCallback(InputError* callback);

    ~MockClient();

  private:
    apache::thrift::async::TEventBase base_;
    channel_ptr channel_;
    std::vector<OutputError*> outputErrorCallbacks_;
    std::vector<InputError*> inputErrorCallbacks_;

    std::shared_ptr<apache::thrift::async::TAsyncSocket> makeSocket(int port);
};

}}

#endif
