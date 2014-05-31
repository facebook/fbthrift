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
#ifndef THRIFT_ASYNC_STREAM_H_
#define THRIFT_ASYNC_STREAM_H_ 1

#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp2/async/InputStream.h"
#include "thrift/lib/cpp2/async/OutputStream.h"

#include <memory>
#include <exception>

namespace apache { namespace thrift {

class StreamException : public apache::thrift::TLibraryException {
  public:
    explicit StreamException(const std::string& message);
};

class StreamChannelCallback {
  public:
    virtual void onStreamSend(std::unique_ptr<folly::IOBuf>&& data) = 0;
    virtual void onOutOfLoopStreamError(
        const folly::exception_wrapper& error) =0;
    virtual ~StreamChannelCallback() {};
};

class MockEventBaseCallback {
  public:
    virtual void startEventBaseLoop() = 0;
    virtual void stopEventBaseLoop() = 0;
    virtual ~MockEventBaseCallback() {};
};

class StreamEndCallback {
  public:
    virtual void onStreamComplete() noexcept = 0;
    virtual void onStreamCancel() noexcept = 0;
    virtual void onStreamError(const folly::exception_wrapper& ep) noexcept = 0;
    virtual ~StreamEndCallback() {};
};

// One restriction on using the class:
//  1. eventBase must point to a valid eventBase throughout the
//     lifetime of a StreamManager, even during deletion
class StreamManager {
  public:
    StreamManager(MockEventBaseCallback* eventBase,
                  StreamSource::StreamMap&& sourceMap,
                  StreamSource::ExceptionDeserializer exceptionDeserializer,
                  StreamSink::StreamMap&& sinkMap,
                  StreamSink::ExceptionSerializer exceptionSerializer,
                  std::unique_ptr<StreamEndCallback> endCallback = nullptr);

    StreamManager(apache::thrift::async::TEventBase* eventBase,
                  StreamSource::StreamMap&& sourceMap,
                  StreamSource::ExceptionDeserializer exceptionDeserializer,
                  StreamSink::StreamMap&& sinkMap,
                  StreamSink::ExceptionSerializer exceptionSerializer,
                  std::unique_ptr<StreamEndCallback> endCallback = nullptr);

    ~StreamManager();

    void notifySend();
    void notifyReceive(std::unique_ptr<folly::IOBuf>&& buffer);
    bool isDone();
    bool hasError();
    void setChannelCallback(StreamChannelCallback* channelCallback);
    void setInputProtocol(ProtocolType type);
    void setOutputProtocol(ProtocolType type);
    bool isSendingEnd();
    void cancel();
    bool isCancelled();

    void startEventBaseLoop();
    void stopEventBaseLoop();
    void sendData(std::unique_ptr<folly::IOBuf>&& data);
    void notifyError(const folly::exception_wrapper& error);
    void notifyOutOfLoopError(const folly::exception_wrapper& error);
    bool needToSendEnd();
    bool hasOpenSyncStreams();

  private:
    apache::thrift::async::TEventBase* eventBase_;
    MockEventBaseCallback* mockEventBase_;
    StreamChannelCallback* channel_;
    StreamSource source_;
    StreamSink sink_;
    std::unique_ptr<StreamEndCallback> endCallback_;
    bool cancelled_;
};

}} // apache::thrift

#include "InputStream.tcc"
#include "OutputStream.tcc"

#endif // THRIFT_ASYNC_STREAM_H_
