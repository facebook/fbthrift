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
#ifndef THRIFT_ASYNC_TZLIBASYNCCHANNEL_H_
#define THRIFT_ASYNC_TZLIBASYNCCHANNEL_H_ 1

#include <thrift/lib/cpp/async/TAsyncEventChannel.h>
#include <thrift/lib/cpp/transport/TZlibTransport.h>

namespace apache { namespace thrift { namespace async {

class TZlibAsyncChannel : public TAsyncEventChannel {
 public:
  explicit TZlibAsyncChannel(
      const std::shared_ptr<TAsyncEventChannel>& channel);

  /**
   * Helper function to create a shared_ptr<TZlibAsyncChannel>.
   *
   * This passes in the correct destructor object, since TZlibAsyncChannel's
   * destructor is protected and cannot be invoked directly.
   */
  static std::shared_ptr<TZlibAsyncChannel> newChannel(
      const std::shared_ptr<TAsyncEventChannel>& channel) {
    return std::shared_ptr<TZlibAsyncChannel>(
        new TZlibAsyncChannel(channel), Destructor());
  }
  bool readable() const override { return channel_->readable(); }
  bool good() const override { return channel_->good(); }
  bool error() const override { return channel_->error(); }
  bool timedOut() const override { return channel_->timedOut(); }
  bool isIdle() const override { return channel_->isIdle(); }

  void sendMessage(const VoidCallback& cob,
                   const VoidCallback& errorCob,
                   transport::TMemoryBuffer* message) override;
  void recvMessage(const VoidCallback& cob,
                   const VoidCallback& errorCob,
                   transport::TMemoryBuffer* message) override;
  void sendAndRecvMessage(const VoidCallback& cob,
                          const VoidCallback& errorCob,
                          transport::TMemoryBuffer* sendBuf,
                          transport::TMemoryBuffer* recvBuf) override;

  std::shared_ptr<TAsyncTransport> getTransport() override {
    return channel_->getTransport();
  }

  void attachEventBase(TEventBase* eventBase) override {
    channel_->attachEventBase(eventBase);
  }
  void detachEventBase() override { channel_->detachEventBase(); }

  uint32_t getRecvTimeout() const override {
    return channel_->getRecvTimeout();
  }

  void setRecvTimeout(uint32_t milliseconds) override {
    channel_->setRecvTimeout(milliseconds);
  }

  void cancelCallbacks() override {
    sendRequest_.cancelCallbacks();
    recvRequest_.cancelCallbacks();
  }

 protected:
  /**
   * Protected destructor.
   *
   * Users of TZlibAsyncChannel must never delete it directly.  Instead,
   * invoke destroy().
   */
  ~TZlibAsyncChannel() override {}

 private:
  class SendRequest {
   public:
    SendRequest();

    bool isSet() const {
      return static_cast<bool>(callback_);
    }

    void set(const VoidCallback& callback,
             const VoidCallback& errorCallback,
             transport::TMemoryBuffer* message);

    void send(TAsyncEventChannel* channel);

    void cancelCallbacks() {
      callback_ = VoidCallback();
      errorCallback_ = VoidCallback();
    }

   private:
    void invokeCallback(VoidCallback callback);
    void sendSuccess();
    void sendError();

    std::shared_ptr<transport::TMemoryBuffer> compressedBuffer_;
    transport::TZlibTransport zlibTransport_;
    VoidCallback sendSuccess_;
    VoidCallback sendError_;

    VoidCallback callback_;
    VoidCallback errorCallback_;
  };

  class RecvRequest {
   public:
    RecvRequest();

    bool isSet() const {
      return static_cast<bool>(callback_);
    }

    void set(const VoidCallback& callback,
             const VoidCallback& errorCallback,
             transport::TMemoryBuffer* message);

    void recv(TAsyncEventChannel* channel);

    void cancelCallbacks() {
      callback_ = VoidCallback();
      errorCallback_ = VoidCallback();
    }

   private:
    void invokeCallback(VoidCallback callback);
    void recvSuccess();
    void recvError();

    std::shared_ptr<transport::TMemoryBuffer> compressedBuffer_;
    transport::TZlibTransport zlibTransport_;
    VoidCallback recvSuccess_;
    VoidCallback recvError_;

    VoidCallback callback_;
    VoidCallback errorCallback_;
    transport::TMemoryBuffer *callbackBuffer_;
  };

  std::shared_ptr<TAsyncEventChannel> channel_;

  // TODO: support multiple pending send requests
  SendRequest sendRequest_;
  RecvRequest recvRequest_;
};

}}} // apache::thrift::async

#endif // THRIFT_ASYNC_TZLIBASYNCCHANNEL_H_
