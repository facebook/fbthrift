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

#include <folly/io/async/AsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/SocketAddress.h>

namespace apache { namespace thrift { namespace transport {
typedef folly::SSLContext SSLContext;
}}}

namespace apache { namespace thrift { namespace async {

// Wrapper around folly's AsyncSSLSocket to maintain backwards compatibility:
// Converts exceptions to thrift's TTransportException type.
class TAsyncSSLSocket : public folly::AsyncSSLSocket, public TAsyncSocket {
 public:
  typedef std::unique_ptr<TAsyncSSLSocket, Destructor> UniquePtr;

  explicit TAsyncSSLSocket(const std::shared_ptr<folly::SSLContext> &ctx,
                           folly::EventBase* evb)
      : AsyncSocket(evb)
      , folly::AsyncSSLSocket(ctx, evb)
      , TAsyncSocket(evb) {}

  TAsyncSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx,
                  folly::EventBase* evb,
                  int fd,
                  bool server = true)
      : AsyncSocket(evb, fd)
      , folly::AsyncSSLSocket(ctx, evb, fd, server)
      , TAsyncSocket(evb, fd) {}

  static std::shared_ptr<TAsyncSSLSocket> newSocket(
    const std::shared_ptr<folly::SSLContext> &ctx, TEventBase* evb) {
    return std::shared_ptr<TAsyncSSLSocket>(new TAsyncSSLSocket(ctx, evb),
                                           Destructor());
  }

  static std::shared_ptr<TAsyncSSLSocket> newSocket(
    const std::shared_ptr<folly::SSLContext>& ctx,
    folly::EventBase* evb,
    int fd,
    bool server = true) {
    return std::shared_ptr<TAsyncSSLSocket>(
      new TAsyncSSLSocket(ctx, evb, fd, server),
      Destructor());
  }

#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)
  TAsyncSSLSocket(const std::shared_ptr<folly::SSLContext> &ctx,
                  folly::EventBase* evb,
                  const std::string& serverName)
      : AsyncSocket(evb)
      , folly::AsyncSSLSocket(ctx, evb, serverName)
      , TAsyncSocket(evb) {}

  TAsyncSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx,
                 folly::EventBase* evb,
                  int fd,
                 const std::string& serverName)
      : AsyncSocket(evb, fd)
      , folly::AsyncSSLSocket(ctx, evb, fd, serverName)
      , TAsyncSocket(evb, fd) {}

  static std::shared_ptr<TAsyncSSLSocket> newSocket(
    const std::shared_ptr<folly::SSLContext>& ctx,
    folly::EventBase* evb,
    const std::string& serverName) {
    return std::shared_ptr<TAsyncSSLSocket>(
      new TAsyncSSLSocket(ctx, evb, serverName),
      Destructor());
  }
#endif


  class HandshakeCallback : public folly::AsyncSSLSocket::HandshakeCB {
   public:
    virtual bool handshakeVerify(TAsyncSSLSocket* /*sock*/,
                                 bool preverifyOk,
                                 X509_STORE_CTX* /*ctx*/) noexcept {
      return preverifyOk;
    }
    virtual void handshakeSuccess(TAsyncSSLSocket *sock) noexcept = 0;
    virtual void handshakeError(
      TAsyncSSLSocket *sock,
      const transport::TTransportException& ex)
      noexcept = 0;

   private:
    bool handshakeVer(AsyncSSLSocket* sock,
                      bool preverifyOk,
                      X509_STORE_CTX* ctx) noexcept override {
      return handshakeVerify(static_cast<TAsyncSSLSocket*>(sock), preverifyOk, ctx);
    }
    void handshakeSuc(folly::AsyncSSLSocket* sock) noexcept override {
      handshakeSuccess(static_cast<TAsyncSSLSocket*>(sock));
    }
    void handshakeErr(folly::AsyncSSLSocket* sock,
                      const folly::AsyncSocketException& ex) noexcept override {
      transport::TTransportException tex(
        transport::TTransportException::TTransportExceptionType(ex.getType()),
        ex.what(), ex.getErrno());

      handshakeError(static_cast<TAsyncSSLSocket*>(sock), tex);
    }
  };

  static TAsyncSSLSocket* getFromSSL(const SSL *ssl) {
    return static_cast<TAsyncSSLSocket*>(folly::AsyncSSLSocket::getFromSSL(ssl));
  }

  virtual void sslConnect(HandshakeCallback *callback, uint64_t timeout = 0,
                  const folly::SSLContext::SSLVerifyPeerEnum& verifyPeer =
                  folly::SSLContext::SSLVerifyPeerEnum::USE_CTX) {
    AsyncSSLSocket::sslConn(callback, timeout, verifyPeer);
  }
};

typedef folly::WriteFlags WriteFlags;

}}}
