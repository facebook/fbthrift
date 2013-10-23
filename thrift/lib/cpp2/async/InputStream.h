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

#ifndef THRIFT_ASYNC_INPUTSTREAM_H_
#define THRIFT_ASYNC_INPUTSTREAM_H_ 1

#include <exception>
#include <memory>
#include <unordered_map>


#include "folly/io/IOBuf.h"
#include "thrift/lib/cpp2/protocol/StreamSerializers.h"

using apache::thrift::protocol::TType;

namespace apache { namespace thrift {

class StreamManager;
class StreamSource;
class StreamSink;

template <typename T>
class InputStreamController;

template <typename T>
class AsyncInputStream;

template <typename T>
class SyncInputStream;

template <typename T>
class InputStreamControllable {
  public:
    virtual bool isClosed() = 0;
    virtual void close() = 0;
    virtual ~InputStreamControllable();
};

template <typename T>
class InputStreamController {
  public:
    virtual bool isClosed() = 0;
    virtual void close() = 0;
    virtual ~InputStreamController();
};

template <typename T>
class InputStreamControllerImpl : public InputStreamController<T> {
  public:
    InputStreamControllerImpl();

    void setControllable(InputStreamControllable<T>* controllable);

    bool isClosed();
    void close();
    void markAsClosed();

  private:
    InputStreamControllable<T>* controllable_;
    bool closed_;

    bool hasControllable();
};

template <typename T>
class InputStreamCallback {
  public:
    InputStreamCallback();
    virtual void onFinish() noexcept = 0;
    virtual void onReceive(T& value) noexcept = 0;
    virtual void onException(const std::exception_ptr& exception) noexcept = 0;
    virtual void onError(const std::exception_ptr& error) noexcept = 0;
    virtual void onClose() noexcept = 0;
    virtual ~InputStreamCallback();

  protected:
    bool isClosed();
    void close();

  private:
    InputStreamControllable<T>* controllable_;

    template <typename Deserializer, typename S>
    friend class InputStreamCallbackDeserializer;

    void setControllable(InputStreamControllable<T>* controllable);
};

class InputStreamCallbackBase {
  public:
    explicit InputStreamCallbackBase(bool isSync);
    virtual void notifyException(const std::exception_ptr& exception) = 0;
    virtual void notifyClose() = 0;
    virtual void notifyFinish() = 0;
    virtual void notifyError(const std::exception_ptr& error) = 0;
    virtual ~InputStreamCallbackBase();

    bool isSync();
    void link(StreamSink* sink,
              StreamManager* manager,
              StreamSource* source,
              int16_t paramId);

    int16_t getParamId();

    void setType(TType type);
    TType getType();

    bool isClosed();
    void markAsClosed();

    virtual void readItem(StreamReader& reader) = 0;

  protected:
    void close();
    void sendAcknowledgement();
    StreamManager* getStreamManager();
    void setBlockedOnItem(bool isBlocked);
    void readBuffer();
    bool bufferHasUnreadData();

  private:
    StreamSink* sink_;
    StreamManager* manager_;
    StreamSource* source_;
    int16_t paramId_;
    TType type_;
    bool isSync_;
    bool closed_;
};

template <typename Deserializer, typename T>
class InputStreamCallbackDeserializer : public InputStreamCallbackBase,
                                        public InputStreamControllable<T> {
  public:
    InputStreamCallbackDeserializer(
        std::unique_ptr<InputStreamCallback<T>>&& callback,
        const std::shared_ptr<InputStreamControllerImpl<T>>& controller,
        bool isSync);

    void notifyException(const std::exception_ptr& exception);
    void notifyClose();
    void notifyFinish();
    void notifyError(const std::exception_ptr& error);

    bool isClosed();
    void close();
    ~InputStreamCallbackDeserializer();

  private:
    std::unique_ptr<InputStreamCallback<T>> callback_;
    std::shared_ptr<InputStreamControllerImpl<T>> controller_;
    bool hasCalledOnClose_;

    void readItem(StreamReader& reader);

  protected:
    bool hasCallback();
    bool hasController();
    bool hasCalledOnClose();
};

template <typename T>
class AsyncInputStream {
  public:
    AsyncInputStream();
    AsyncInputStream(std::unique_ptr<InputStreamCallback<T>>&& callback);

    AsyncInputStream(const AsyncInputStream<T>& another) = delete;
    AsyncInputStream(AsyncInputStream<T>&& another) = delete;

    AsyncInputStream& operator=(const AsyncInputStream<T>& another) = delete;
    AsyncInputStream& operator=(AsyncInputStream<T>&& another) = delete;

    void setCallback(std::unique_ptr<InputStreamCallback<T>>&& callback);
    std::shared_ptr<InputStreamController<T>> makeController();

    template <typename Deserializer>
    std::unique_ptr<InputStreamCallbackBase> makeHandler();

  private:
    std::unique_ptr<InputStreamCallback<T>> callback_;
    std::shared_ptr<InputStreamControllerImpl<T>> controller_;
    bool hasMadeHandler_;
};

template <typename T>
class SyncInputStreamControllable {
  public:
    virtual void setStream(SyncInputStream<T>* stream) = 0;
    virtual void waitForItem() = 0;
    virtual void get(T& value) = 0;
    virtual void close() = 0;
    virtual ~SyncInputStreamControllable();
};

template <typename Deserializer, typename T>
class SyncInputStreamCallback :
    public InputStreamCallbackDeserializer<Deserializer, T>,
    public SyncInputStreamControllable<T> {

  public:
    explicit SyncInputStreamCallback(SyncInputStream<T>* stream);

    void notifyException(const std::exception_ptr& exception);
    void notifyClose();
    void notifyFinish();
    void notifyError(const std::exception_ptr& error);

    void readItem(StreamReader& reader);

    void setStream(SyncInputStream<T>* stream);
    void waitForItem();
    void get(T& value);
    void close();
    // the object may be deleted after this function, so do not access any
    // member variables after calling this function
    void runEventBaseLoop();

    bool hasItem();
    void readItem(T& value);
    void skipItem();

    ~SyncInputStreamCallback();

  private:
    SyncInputStream<T>* stream_;
    bool thisStartedTheEventBaseLoop_;
    std::exception_ptr* errorPtr_;
    bool* deletedFlagPtr_;
    StreamReader* reader_;
    bool hasGottenItem_;

    void sendAcknowledgementIfGottenItems();
    bool needToRunEventBaseLoop();
    void notifyOutOfLoopError(const std::exception_ptr& error);
};

template <typename T>
class SyncInputStream {
  public:
    SyncInputStream();

    SyncInputStream(const SyncInputStream<T>& another) = delete;
    SyncInputStream(SyncInputStream<T>&& another) noexcept;

    SyncInputStream& operator=(const SyncInputStream<T>& another) = delete;
    SyncInputStream& operator=(SyncInputStream<T>&& another) noexcept;

    bool isDone();
    void get(T& value);
    void close();

    bool isFinished(); // finished means there are no more input to read
                       // finished_ implies closed_, but not vice versa
    bool isClosed();   // closed means either you have closed the stream
                       // or the output stream on the other side has
                       // closed the stream
                       // if you closed it, then the output stream may
                       // not be finished
    ~SyncInputStream();

    template <typename Serializer>
    std::unique_ptr<InputStreamCallbackBase> makeHandler();

  private:
    SyncInputStreamControllable<T>* controllable_;
    bool finished_;
    bool closed_;
    std::exception_ptr exception_;

    bool hasControllable();
    bool hasBeenClosed();
    bool hasException();
    void rethrowException();
    void clearException();

    template <typename Deserializer, typename S>
    friend class SyncInputStreamCallback;
    void setException(const std::exception_ptr& exception);
    void markAsFinished();
    void markAsClosed();
    void clearControllable();
};

template <typename T>
class StreamSingleton {
  public:
    StreamSingleton();

    StreamSingleton(const StreamSingleton<T>& another) = delete;
    StreamSingleton(StreamSingleton<T>&& another) = default;

    StreamSingleton& operator=(const StreamSingleton<T>& another) = delete;
    StreamSingleton& operator=(StreamSingleton<T>&& another) = default;

    void get(T& value);

    template <typename Deserializer>
    std::unique_ptr<InputStreamCallbackBase> makeHandler();

  private:
    SyncInputStream<T> stream_;
};

class StreamSource {
  private:
    typedef std::unordered_map<int16_t, TType> ParamIdToTypeMap;

  public:
    typedef std::unordered_map<int16_t,
            std::unique_ptr<InputStreamCallbackBase>> StreamMap;

    typedef std::exception_ptr (*ExceptionDeserializer)(StreamReader&);

    StreamSource(StreamSink* sink,
                 StreamManager* manager,
                 StreamMap&& streamMap,
                 ExceptionDeserializer exceptionDeserializer);

    ~StreamSource();

    void setProtocolType(ProtocolType type);

    void setBlockedOnItem(bool isBlocked);
    bool isBlockedOnItem();
    void readBuffer();
    bool bufferHasUnreadData();

    void onCancel();
    bool hasOpenStreams();
    bool hasOpenSyncStreams();
    bool hasReceivedEnd();
    bool hasError();
    void onReceive(std::unique_ptr<folly::IOBuf>&& newBuffer);
    void onError(const std::exception_ptr& error);

    void onStreamClose(InputStreamCallbackBase* stream);

  private:
    StreamSink* sink_;
    StreamManager* manager_;
    StreamMap streamMap_;
    ExceptionDeserializer exceptionDeserializer_;
    bool isSync_;
    uint32_t numOpenStreams_;
    bool hasReadDescription_;
    ParamIdToTypeMap unexpectedStreamMap_;
    bool blockedOnItem_;
    bool hasError_;
    StreamReader streamReader_;

    bool isSync();

    InputStreamCallbackBase* getStream(int16_t paramId);
    TType getUnexpectedStreamType(int16_t paramId);

    bool hasReadStreamDescription();
    void readStreamDescription();
    void notifyStreamReadItem(int16_t paramId);
    void notifyStreamFinished(int16_t paramId);
    void notifyStreamReadException(int16_t paramId);
};

}} // apache::thrift

#endif // THRIFT_ASYNC_INPUTSTREAM_H_
