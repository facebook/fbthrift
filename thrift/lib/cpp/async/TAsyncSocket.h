/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>

namespace apache { namespace thrift { namespace async {

// Wrapper around folly's AsyncSocket to maintain backwards compatibility:
// Converts exceptions to thrift's TTransportException type.
class TAsyncSocket : public virtual folly::AsyncSocket, public TAsyncTransport {
 public:
  typedef std::unique_ptr<TAsyncSocket, Destructor> UniquePtr;

  explicit TAsyncSocket(folly::EventBase* evb) : folly::AsyncSocket(evb) {}

  TAsyncSocket(folly::EventBase* evb,
               const folly::SocketAddress& address,
               uint32_t connectTimeout = 0)
    : folly::AsyncSocket(evb, address, connectTimeout) {}

  TAsyncSocket(folly::EventBase* evb, int fd) : folly::AsyncSocket(evb, fd) {}

  TAsyncSocket(folly::EventBase* evb,
               const std::string& ip,
               uint16_t port,
                 uint32_t connectTimeout = 0)
      : folly::AsyncSocket(evb, ip, port, connectTimeout) {}


  static std::shared_ptr<TAsyncSocket> newSocket(folly::EventBase* evb) {
    return std::shared_ptr<TAsyncSocket>(new TAsyncSocket(evb),
                                           Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(
      folly::EventBase* evb,
      const folly::SocketAddress& address,
      uint32_t connectTimeout = 0) {
    return std::shared_ptr<TAsyncSocket>(
        new TAsyncSocket(evb, address, connectTimeout),
        Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(
      folly::EventBase* evb,
      const std::string& ip,
      uint16_t port,
      uint32_t connectTimeout = 0) {
    return std::shared_ptr<TAsyncSocket>(
        new TAsyncSocket(evb, ip, port, connectTimeout),
        Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(folly::EventBase* evb, int fd) {
    return std::shared_ptr<TAsyncSocket>(new TAsyncSocket(evb, fd),
                                           Destructor());
  }

  class ConnectCallback : public folly::AsyncSocket::ConnectCallback {
   public:
    ~ConnectCallback() override {}

    /**
     * connectSuccess() will be invoked when the connection has been
     * successfully established.
     */
    void connectSuccess() noexcept override = 0;

    /**
     * connectError() will be invoked if the connection attempt fails.
     *
     * @param ex        An exception describing the error that occurred.
     */
    virtual void connectError(const transport::TTransportException& ex)
      noexcept = 0;

   private:
    void connectErr(const folly::AsyncSocketException& ex) noexcept override {
      transport::TTransportException tex(
        transport::TTransportException::TTransportExceptionType(ex.getType()),
        ex.what(), ex.getErrno());

      connectError(tex);
    }
  };

  // Read and write methods that aren't part of folly::AsyncTransport
  void setReadCallback(TAsyncTransport::ReadCallback* callback) override {
    AsyncSocket::setReadCB(callback);
  }

  TAsyncTransport::ReadCallback* getReadCallback() const override {
    return dynamic_cast<TAsyncTransport::ReadCallback*>(
      AsyncSocket::getReadCallback());
  }

  void setIsAccepted(bool accepted) {
    accepted_ = accepted;
  }

  // Returns true if this socket was created as a result of an accept() call,
  // rather than a connect() call. Used to check if our end is the client or
  // the server. This can't be inferred automatically from the socket fd,
  // so we set it manually with setIsAccepted() after accepting.
  bool isAccepted() const {
    return accepted_;
  }

 private:
  bool accepted_ = false;
};

typedef folly::WriteFlags WriteFlags;

}}}
