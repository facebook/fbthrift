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

#ifndef THRIFT_TRANSPORT_TSSLSOCKET_H_
#define THRIFT_TRANSPORT_TSSLSOCKET_H_ 1

#include <errno.h>
#include <string>
#include <memory>
#include <folly/io/async/SSLContext.h>
#include <thrift/lib/cpp/transport/TSocket.h>

namespace folly {
class SocketAddress;
}

namespace apache { namespace thrift { namespace transport {

typedef folly::SSLContext SSLContext;
typedef folly::PasswordCollector PasswordCollector;

/**
 * OpenSSL implementation for SSL socket interface.
 */
class TSSLSocket: public TVirtualTransport<TSSLSocket, TSocket> {
 public:
  /**
   * Constructor.
   */
  explicit TSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx);
  /**
   * Constructor, create an instance of TSSLSocket given an existing socket.
   *
   * @param socket An existing socket
   */
  TSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx, int socket);
  /**
   * Constructor.
   *
   * @param host  Remote host name
   * @param port  Remote port number
   */
  TSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx,
             const std::string& host,
             int port);
  /**
   * Constructor.
   */
  TSSLSocket(const std::shared_ptr<folly::SSLContext>& ctx,
             const folly::SocketAddress& address);
  /**
   * Destructor.
   */
  ~TSSLSocket();

  /**
   * TTransport interface.
   */
  bool     isOpen();
  bool     peek();
  void     open();
  void     close();
  uint32_t read(uint8_t* buf, uint32_t len);
  void     write(const uint8_t* buf, uint32_t len);
  void     flush();

  /**
   * Set whether to use client or server side SSL handshake protocol.
   *
   * @param flag  Use server side handshake protocol if true.
   */
  void server(bool flag) { server_ = flag; }
  /**
   * Determine whether the SSL socket is server or client mode.
   */
  bool server() const { return server_; }

protected:
  /**
   * Verify peer certificate after SSL handshake completes.
   */
  virtual void verifyCertificate();

  /**
   * Possibly validate the peer's certificate name, depending on how this
   * folly::SSLContext was configured by authenticate().
   *
   * @return True if the peer's name is acceptable, false otherwise
   */
  bool validatePeerName(SSL* ssl);

  /**
   * Initiate SSL handshake if not already initiated.
   */
  void checkHandshake();

  bool server_;
  SSL* ssl_;
  std::shared_ptr<folly::SSLContext> ctx_;
};

/**
 * SSL socket factory. SSL sockets should be created via SSL factory.
 */
class TSSLSocketFactory {
 public:
  /**
   * Constructor/Destructor
   */
  explicit TSSLSocketFactory(const std::shared_ptr<folly::SSLContext>& context);
  virtual ~TSSLSocketFactory();

  /**
   * Create an instance of TSSLSocket with a fresh new socket.
   */
  virtual std::shared_ptr<TSSLSocket> createSocket();
  /**
   * Create an instance of TSSLSocket with the given socket.
   *
   * @param socket An existing socket.
   */
  virtual std::shared_ptr<TSSLSocket> createSocket(int socket);
   /**
   * Create an instance of TSSLSocket.
   *
   * @param host  Remote host to be connected to
   * @param port  Remote port to be connected to
   */
  virtual std::shared_ptr<TSSLSocket> createSocket(const std::string& host,
                                                     int port);
  /**
   * Set/Unset server mode.
   *
   * @param flag  Server mode if true
   */
  virtual void server(bool flag) { server_ = flag; }
  /**
   * Determine whether the socket is in server or client mode.
   *
   * @return true, if server mode, or, false, if client mode
   */
  virtual bool server() const { return server_; }

 private:
  std::shared_ptr<folly::SSLContext> ctx_;
  bool server_;
};

/**
 * SSL exception.
 */
class TSSLException: public TTransportException {
 public:
  explicit TSSLException(const std::string& message):
    TTransportException(TTransportException::INTERNAL_ERROR, message) {}

  virtual const char* what() const throw() {
    if (message_.empty()) {
      return "TSSLException";
    } else {
      return message_.c_str();
    }
  }
};

}}}

#endif
