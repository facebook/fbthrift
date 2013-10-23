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

#ifndef THRIFT_ASYNC_OUTPUTSTREAM_H_
#define THRIFT_ASYNC_OUTPUTSTREAM_H_ 1

#include <exception>
#include <memory>
#include <unordered_map>


#include "thrift/lib/cpp2/protocol/StreamSerializers.h"
#include "thrift/lib/cpp/protocol/TProtocol.h"

using apache::thrift::protocol::TType;

namespace apache { namespace thrift {

class StreamChannelCallback;
class StreamManager;
class StreamSink;

template <typename T>
class OutputStreamController;

template <typename T>
class AsyncOutputStream;

template <typename T>
class SyncOutputStream;

template <typename T>
class OutputStreamControllable {
  public:
    virtual bool isClosed() = 0;
    virtual void close() = 0;
    virtual void put(const T& data) = 0;
    virtual void putException(const std::exception_ptr& exception) = 0;
    virtual ~OutputStreamControllable();
};

template <typename T>
class OutputStreamController {
  public:
    virtual bool isClosed() = 0;
    virtual void close() = 0;
    virtual void put(const T& data) = 0;
    virtual void putException(const std::exception_ptr& exception) = 0;
    virtual ~OutputStreamController();
};

template <typename T>
class OutputStreamControllerImpl : public OutputStreamController<T> {
  public:
    OutputStreamControllerImpl();

    void setControllable(OutputStreamControllable<T>* controllable);

    bool isClosed();
    void close();
    void put(const T& data);
    void putException(const std::exception_ptr& exception);

    void markAsClosed();

  private:
    OutputStreamControllable<T>* controllable_;
    bool closed_;

    bool hasControllable();
};

template <typename T>
class OutputStreamCallback {
  public:
    OutputStreamCallback();
    virtual void onSend() noexcept = 0;
    virtual void onError(const std::exception_ptr& error) noexcept = 0;
    virtual void onClose() noexcept = 0;
    virtual ~OutputStreamCallback();

  protected:
    bool isClosed();
    void close();
    void put(const T& data);
    void putException(const std::exception_ptr& exception);

  private:
    OutputStreamControllable<T>* controllable_;

    template <typename Serializer, typename S>
    friend class OutputStreamCallbackSerializer;
    void setControllable(OutputStreamControllable<T>* controllable);
};

class OutputStreamCallbackBase {
  public:
    enum class SendState {INIT, NONE, PENDING, SENT};

    explicit OutputStreamCallbackBase(bool isSync);
    virtual ~OutputStreamCallbackBase();

    virtual void notifySend() = 0;
    virtual void notifyError(const std::exception_ptr& error) = 0;
    virtual void notifyClose() = 0;

    bool isSync();
    void link(StreamManager* manager,
              StreamSink* sink,
              StreamWriter* writer,
              int16_t paramId);

    void setSendState(SendState state);
    SendState getSendState();

    bool isClosed();
    void markAsClosed();
    int16_t getParamId();

    void setType(TType type);
    TType getType();

  protected:
    void close();
    bool isLinked();

    StreamManager* getStreamManager();
    StreamWriter* getStreamWriter();
    StreamSink* getStreamSink();

  private:
    StreamManager* manager_;
    StreamSink* sink_;
    StreamWriter* writer_;
    TType type_;
    bool isSync_;
    int16_t paramId_;
    SendState sendState_;
    bool closed_;
};

template <typename Serializer, typename T>
class OutputStreamCallbackSerializer : public OutputStreamCallbackBase,
                                       public OutputStreamControllable<T> {
  public:
    OutputStreamCallbackSerializer(
        std::unique_ptr<OutputStreamCallback<T>>&&,
        const std::shared_ptr<OutputStreamControllerImpl<T>>&,
        bool isSync);

    void notifySend();
    void notifyError(const std::exception_ptr& error);
    void notifyClose();

    bool isClosed();
    void close();
    void put(const T& data);
    void putException(const std::exception_ptr& exception);

    ~OutputStreamCallbackSerializer();

  private:
    std::unique_ptr<OutputStreamCallback<T>> callback_;
    std::shared_ptr<OutputStreamControllerImpl<T>> controller_;
    bool hasCalledOnClose_;

  protected:
    bool hasCallback();
    bool hasController();
    bool hasCalledOnClose();
};

template <typename T>
class AsyncOutputStream {
  public:
    AsyncOutputStream();
    AsyncOutputStream(std::unique_ptr<OutputStreamCallback<T>>&& callback);

    AsyncOutputStream(const AsyncOutputStream<T>& another) = delete;
    AsyncOutputStream(AsyncOutputStream<T>&& another) = delete;

    AsyncOutputStream& operator=(const AsyncOutputStream<T>& another) = delete;
    AsyncOutputStream& operator=(AsyncOutputStream<T>&& another) = delete;

    void setCallback(std::unique_ptr<OutputStreamCallback<T>>&& callback);
    std::shared_ptr<OutputStreamController<T>> makeController();

    template <typename Serializer>
    std::unique_ptr<OutputStreamCallbackBase> makeHandler();

  private:
    std::unique_ptr<OutputStreamCallback<T>> callback_;
    std::shared_ptr<OutputStreamControllerImpl<T>> controller_;
    bool hasMadeHandler_;
};

template <typename T>
class SyncOutputStreamControllable {
  public:
    virtual void setStream(SyncOutputStream<T>* stream) = 0;

    // the object may be deleted after this function, so do not access any
    // member variables after calling this function
    virtual void runEventBaseLoop() = 0;
    virtual void notifyOutOfLoopError(const std::exception_ptr& error) = 0;

    virtual ~SyncOutputStreamControllable();
};

template <typename Serializer, typename T>
class SyncOutputStreamCallback
  : public OutputStreamCallbackSerializer<Serializer, T>,
    public SyncOutputStreamControllable<T> {

  public:
    SyncOutputStreamCallback(
        SyncOutputStream<T>* stream,
        const std::shared_ptr<OutputStreamControllerImpl<T>>& controller);

    void notifySend();
    void notifyError(const std::exception_ptr& error);
    void notifyClose();

    void setStream(SyncOutputStream<T>* stream);
    void runEventBaseLoop();
    void notifyOutOfLoopError(const std::exception_ptr& error);

    ~SyncOutputStreamCallback();

  private:
    SyncOutputStream<T>* stream_;
    bool thisStartedTheEventBaseLoop_;
    std::exception_ptr* errorPtr_;
    bool* deletedFlagPtr_;

    bool needToRunEventBaseLoop();
};

template <typename T>
class SyncOutputStream {
  public:
    SyncOutputStream();

    SyncOutputStream(const SyncOutputStream<T>& another) = delete;
    SyncOutputStream(SyncOutputStream<T>&& another) noexcept;

    SyncOutputStream& operator=(const SyncOutputStream<T>& another) = delete;
    SyncOutputStream& operator=(SyncOutputStream<T>&& another) noexcept;

    void put(const T& value);
    void putException(const std::exception_ptr& exception);
    void close();
    bool isClosed();

    ~SyncOutputStream();

    template <typename Serializer>
    std::unique_ptr<OutputStreamCallbackBase> makeHandler();

  private:
    SyncOutputStreamControllable<T>* controllable_;
    std::shared_ptr<OutputStreamControllerImpl<T>> controller_;

    bool hasControllable();

    template <typename Serializer, typename S>
    friend class SyncOutputStreamCallback;
    void clearControllable();
};

class StreamSink {
  private:
    typedef OutputStreamCallbackBase::SendState SendState;

  public:
    typedef std::unordered_map<int16_t,
            std::unique_ptr<OutputStreamCallbackBase>> StreamMap;

    typedef void (*ExceptionSerializer)(StreamWriter&,
                                        const std::exception_ptr&);

    StreamSink(StreamManager* manager,
               StreamMap&& streamMap,
               ExceptionSerializer exceptionSerializer);

    ~StreamSink();

    void setChannelCallback(StreamChannelCallback* channel);
    void setProtocolType(ProtocolType type);

    void onStreamPut(OutputStreamCallbackBase* stream);
    void onStreamClose(OutputStreamCallbackBase* stream);
    void onStreamException(OutputStreamCallbackBase* stream,
                           const std::exception_ptr& exception);

    void onCancel();
    bool hasWrittenEnd();
    bool hasSentEnd();
    bool hasError();
    bool hasOpenStreams();
    bool hasOpenSyncStreams();
    void onSend();
    void onError(const std::exception_ptr& error);
    bool isSendingEnd();

    void sendAcknowledgement();
    void sendClose(int16_t paramId);
    void onStreamCloseFromInput(int16_t paramId);
    void sendEnd();

  private:
    StreamManager* manager_;
    StreamMap streamMap_;
    ExceptionSerializer exceptionSerializer_;

    // Since we only want to have at most one outstanding callback, and
    // each send must be associated with one callback, we ensure this by
    // ensuring that we have only one outstanding send request at a time.
    // There may be a better way of avoiding having multiple outstanding
    // send callbacks for one stream but this is the simplest.
    bool hasOutstandingSend_;

    uint32_t numOpenStreams_;
    uint32_t numOpenSyncStreams_;

    bool hasError_;
    StreamWriter streamWriter_;

    void writeDescription();

    void sendDataIfNecessary();
    bool shouldSendData();
    void sendData();

    OutputStreamCallbackBase* getStream(int16_t paramId);
};

}} // apache::thrift

#endif // THRIFT_ASYNC_OUTPUTSTREAM_H_
