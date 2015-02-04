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

#include <signal.h>
#include <pthread.h>

#include <folly/io/async/AsyncServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncTimeout.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp/transport/TSocketAddress.h>

#include <gtest/gtest.h>
#include <iostream>
#include <list>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

enum StateEnum {
  STATE_WAITING,
  STATE_SUCCEEDED,
  STATE_FAILED
};

// The destructors of all callback classes assert that the state is
// STATE_SUCCEEDED, for both possitive and negative tests. The tests
// are responsible for setting the succeeded state properly before the
// destructors are called.

class WriteCallbackBase :
public apache::thrift::async::TAsyncTransport::WriteCallback {
public:
  WriteCallbackBase()
      : state(STATE_WAITING)
      , bytesWritten(0)
      , exception() {}

  ~WriteCallbackBase() {
    EXPECT_EQ(state, STATE_SUCCEEDED);
  }

  void setSocket(
    const std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> &socket) {
    socket_ = socket;
  }

  virtual void writeSuccess() noexcept {
    std::cerr << "writeSuccess" << std::endl;
    state = STATE_SUCCEEDED;
  }

  virtual void writeError(
    size_t bytesWritten,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "writeError: bytesWritten " << bytesWritten
         << ", exception " << ex.what() << std::endl;

    state = STATE_FAILED;
    this->bytesWritten = bytesWritten;
    exception = ex;
    socket_->close();
    socket_->detachEventBase();
  }

  std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> socket_;
  StateEnum state;
  size_t bytesWritten;
  apache::thrift::transport::TTransportException exception;
};

class ReadCallbackBase :
public apache::thrift::async::TAsyncTransport::ReadCallback {
public:
  explicit ReadCallbackBase(WriteCallbackBase *wcb)
      : wcb_(wcb)
      , state(STATE_WAITING) {}

  ~ReadCallbackBase() {
    EXPECT_EQ(state, STATE_SUCCEEDED);
  }

  void setSocket(
    const std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> &socket) {
    socket_ = socket;
  }

  void setState(StateEnum s) {
    state = s;
    if (wcb_) {
      wcb_->state = s;
    }
  }

  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "readError " << ex.what() << std::endl;
    state = STATE_FAILED;
    socket_->close();
    socket_->detachEventBase();
  }

  virtual void readEOF() noexcept {
    std::cerr << "readEOF" << std::endl;

    socket_->close();
    socket_->detachEventBase();
  }

  std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> socket_;
  WriteCallbackBase *wcb_;
  StateEnum state;
};

class ReadCallback : public ReadCallbackBase {
public:
  explicit ReadCallback(WriteCallbackBase *wcb)
      : ReadCallbackBase(wcb)
      , buffers() {}

  ~ReadCallback() {
    for (std::vector<Buffer>::iterator it = buffers.begin();
         it != buffers.end();
         ++it) {
      it->free();
    }
    currentBuffer.free();
  }

  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    if (!currentBuffer.buffer) {
      currentBuffer.allocate(4096);
    }
    *bufReturn = currentBuffer.buffer;
    *lenReturn = currentBuffer.length;
  }

  virtual void readDataAvailable(size_t len) noexcept {
    std::cerr << "readDataAvailable, len " << len << std::endl;

    currentBuffer.length = len;

    wcb_->setSocket(socket_);

    // Write back the same data.
    socket_->write(wcb_, currentBuffer.buffer, len);

    buffers.push_back(currentBuffer);
    currentBuffer.reset();
    state = STATE_SUCCEEDED;
  }

  class Buffer {
  public:
    Buffer() : buffer(nullptr), length(0) {}
    Buffer(char* buf, size_t len) : buffer(buf), length(len) {}

    void reset() {
      buffer = nullptr;
      length = 0;
    }
    void allocate(size_t length) {
      assert(buffer == nullptr);
      this->buffer = static_cast<char*>(malloc(length));
      this->length = length;
    }
    void free() {
      ::free(buffer);
      reset();
    }

    char* buffer;
    size_t length;
  };

  std::vector<Buffer> buffers;
  Buffer currentBuffer;
};

class ReadErrorCallback : public ReadCallbackBase {
public:
  explicit ReadErrorCallback(WriteCallbackBase *wcb)
      : ReadCallbackBase(wcb) {}

  // Return nullptr buffer to trigger readError()
  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *bufReturn = nullptr;
    *lenReturn = 0;
  }

  virtual void readDataAvailable(size_t len) noexcept {
    // This should never to called.
    FAIL();
  }

  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ReadCallbackBase::readError(ex);
    std::cerr << "ReadErrorCallback::readError" << std::endl;
    setState(STATE_SUCCEEDED);
  }
};

class WriteErrorCallback : public ReadCallback {
public:
  explicit WriteErrorCallback(WriteCallbackBase *wcb)
      : ReadCallback(wcb) {}

  virtual void readDataAvailable(size_t len) noexcept {
    std::cerr << "readDataAvailable, len " << len << std::endl;

    currentBuffer.length = len;

    // close the socket before writing to trigger writeError().
    ::close(socket_->getFd());

    wcb_->setSocket(socket_);

    // Write back the same data.
    socket_->write(wcb_, currentBuffer.buffer, len);

    if (wcb_->state == STATE_FAILED) {
      setState(STATE_SUCCEEDED);
    } else {
      state = STATE_FAILED;
    }

    buffers.push_back(currentBuffer);
    currentBuffer.reset();
  }

  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "readError " << ex.what() << std::endl;
    // do nothing since this is expected
  }
};

class EmptyReadCallback : public ReadCallback {
public:
  explicit EmptyReadCallback()
      : ReadCallback(nullptr) {}

  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "readError " << ex.what() << std::endl;
    state = STATE_FAILED;
    tcpSocket_->close();
    tcpSocket_->detachEventBase();
  }

  virtual void readEOF() noexcept {
    std::cerr << "readEOF" << std::endl;

    tcpSocket_->close();
    tcpSocket_->detachEventBase();
    state = STATE_SUCCEEDED;
  }

  std::shared_ptr<apache::thrift::async::TAsyncSocket> tcpSocket_;
};

class HandshakeCallback :
public apache::thrift::async::TAsyncSSLSocket::HandshakeCallback {
public:
  enum ExpectType {
    EXPECT_SUCCESS,
    EXPECT_ERROR
  };

  explicit HandshakeCallback(ReadCallbackBase *rcb,
                             ExpectType expect = EXPECT_SUCCESS):
      state(STATE_WAITING),
      rcb_(rcb),
      expect_(expect) {}

  void setSocket(
    const std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> &socket) {
    socket_ = socket;
  }

  void setState(StateEnum s) {
    state = s;
    rcb_->setState(s);
  }

  // Functions inherited from TAsyncSSLSocket::HandshakeCallback
  virtual void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket *sock)
    noexcept {
    EXPECT_EQ(sock, socket_.get());
    std::cerr << "HandshakeCallback::connectionAccepted" << std::endl;
    rcb_->setSocket(socket_);
    sock->setReadCallback(rcb_);
    state = (expect_ == EXPECT_SUCCESS) ? STATE_SUCCEEDED : STATE_FAILED;
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket *sock,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "HandshakeCallback::handshakeError " << ex.what() << std::endl;
    state = (expect_ == EXPECT_ERROR) ? STATE_SUCCEEDED : STATE_FAILED;
    if (expect_ == EXPECT_ERROR) {
      // rcb will never be invoked
      rcb_->setState(STATE_SUCCEEDED);
    }
  }

  ~HandshakeCallback() {
    EXPECT_EQ(state, STATE_SUCCEEDED);
  }

  void closeSocket() {
    socket_->close();
    state = STATE_SUCCEEDED;
  }

  StateEnum state;
  std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> socket_;
  ReadCallbackBase *rcb_;
  ExpectType expect_;
};

class SSLServerAcceptCallbackBase:
public folly::AsyncServerSocket::AcceptCallback {
public:
  explicit SSLServerAcceptCallbackBase(::HandshakeCallback *hcb):
  state(STATE_WAITING), hcb_(hcb) {}

  ~SSLServerAcceptCallbackBase() {
    EXPECT_EQ(state, STATE_SUCCEEDED);
  }

  virtual void acceptError(const std::exception& ex) noexcept {
    std::cerr << "SSLServerAcceptCallbackBase::acceptError "
              << ex.what() << std::endl;
    state = STATE_FAILED;
  }

  virtual void connectionAccepted(int fd,
                                  const folly::SocketAddress& clientAddr) noexcept {
    printf("Connection accepted\n");
    std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> sslSock;
    try {
      // Create a AsyncSSLSocket object with the fd. The socket should be
      // added to the event base and in the state of accepting SSL connection.
      sslSock = apache::thrift::async::TAsyncSSLSocket::newSocket(ctx_, base_, fd);
    } catch (const std::exception &e) {
      LOG(ERROR) << "Exception %s caught while creating a AsyncSSLSocket "
        "object with socket " << e.what() << fd;
      ::close(fd);
      acceptError(e);
      return;
    }

    connAccepted(sslSock);
  }

  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s) = 0;

  StateEnum state;
  ::HandshakeCallback *hcb_;
  std::shared_ptr<folly::SSLContext> ctx_;
  folly::EventBase* base_;
};

class SSLServerAcceptCallback: public SSLServerAcceptCallbackBase {
public:
  uint32_t timeout_;

  explicit SSLServerAcceptCallback(::HandshakeCallback *hcb,
                                   uint32_t timeout = 0):
      SSLServerAcceptCallbackBase(hcb),
      timeout_(timeout) {}

  virtual ~SSLServerAcceptCallback() {
    if (timeout_ > 0) {
      // if we set a timeout, we expect failure
      EXPECT_EQ(hcb_->state, STATE_FAILED);
      hcb_->setState(STATE_SUCCEEDED);
    }
  }

  // Functions inherited from TAsyncSSLServerSocket::SSLAcceptCallback
  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s)
    noexcept {
    auto sock = std::static_pointer_cast<apache::thrift::async::TAsyncSSLSocket>(s);
    std::cerr << "SSLServerAcceptCallback::connAccepted" << std::endl;

    hcb_->setSocket(sock);
    sock->sslAccept(hcb_, timeout_);
    EXPECT_EQ(sock->getSSLState(),
                      apache::thrift::async::TAsyncSSLSocket::STATE_ACCEPTING);

    state = STATE_SUCCEEDED;
  }
};

class SSLServerAcceptCallbackDelay: public SSLServerAcceptCallback {
public:
  explicit SSLServerAcceptCallbackDelay(::HandshakeCallback *hcb):
      SSLServerAcceptCallback(hcb) {}

  // Functions inherited from TAsyncSSLServerSocket::SSLAcceptCallback
  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s)
    noexcept {

    auto sock = std::static_pointer_cast<apache::thrift::async::TAsyncSSLSocket>(s);

    std::cerr << "SSLServerAcceptCallbackDelay::connAccepted"
              << std::endl;
    int fd = sock->getFd();

#ifndef TCP_NOPUSH
    {
    // The accepted connection should already have TCP_NODELAY set
    int value;
    socklen_t valueLength = sizeof(value);
    int rc = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, &valueLength);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(value, 1);
    }
#endif

    // Unset the TCP_NODELAY option.
    int value = 0;
    socklen_t valueLength = sizeof(value);
    int rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, valueLength);
    EXPECT_EQ(rc, 0);

    rc = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, &valueLength);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(value, 0);

    SSLServerAcceptCallback::connAccepted(sock);
  }
};

class SSLServerAsyncCacheAcceptCallback: public SSLServerAcceptCallback {
public:
  explicit SSLServerAsyncCacheAcceptCallback(::HandshakeCallback *hcb,
                                             uint32_t timeout = 0):
    SSLServerAcceptCallback(hcb, timeout) {}

  // Functions inherited from TAsyncSSLServerSocket::SSLAcceptCallback
  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s)
    noexcept {
    auto sock = std::static_pointer_cast<apache::thrift::async::TAsyncSSLSocket>(s);

    std::cerr << "SSLServerAcceptCallback::connAccepted" << std::endl;

    hcb_->setSocket(sock);
    sock->sslAccept(hcb_, timeout_);
    ASSERT_TRUE((sock->getSSLState() ==
                 apache::thrift::async::TAsyncSSLSocket::STATE_ACCEPTING) ||
                (sock->getSSLState() ==
                 apache::thrift::async::TAsyncSSLSocket::STATE_CACHE_LOOKUP));

    state = STATE_SUCCEEDED;
  }
};


class HandshakeErrorCallback: public SSLServerAcceptCallbackBase {
public:
  explicit HandshakeErrorCallback(::HandshakeCallback *hcb):
  SSLServerAcceptCallbackBase(hcb)  {}

  // Functions inherited from TAsyncSSLServerSocket::SSLAcceptCallback
  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s)
    noexcept {
    auto sock = std::static_pointer_cast<apache::thrift::async::TAsyncSSLSocket>(s);

    std::cerr << "HandshakeErrorCallback::connAccepted" << std::endl;

    // The first call to sslAccept() should succeed.
    hcb_->setSocket(sock);
    sock->sslAccept(hcb_);
    EXPECT_EQ(sock->getSSLState(),
                      apache::thrift::async::TAsyncSSLSocket::STATE_ACCEPTING);

    // The second call to sslAccept() should fail.
    ::HandshakeCallback callback2(hcb_->rcb_);
    callback2.setSocket(sock);
    sock->sslAccept(&callback2);
    EXPECT_EQ(sock->getSSLState(),
                      apache::thrift::async::TAsyncSSLSocket::STATE_ERROR);

    // Both callbacks should be in the error state.
    EXPECT_EQ(hcb_->state, STATE_FAILED);
    EXPECT_EQ(callback2.state, STATE_FAILED);

    sock->detachEventBase();

    state = STATE_SUCCEEDED;
    hcb_->setState(STATE_SUCCEEDED);
    callback2.setState(STATE_SUCCEEDED);
  }
};

class HandshakeTimeoutCallback: public SSLServerAcceptCallbackBase {
public:
  explicit HandshakeTimeoutCallback(::HandshakeCallback *hcb):
  SSLServerAcceptCallbackBase(hcb)  {}

  // Functions inherited from TAsyncSSLServerSocket::SSLAcceptCallback
  virtual void connAccepted(
    const std::shared_ptr<folly::AsyncSSLSocket> &s)
    noexcept {
    std::cerr << "HandshakeErrorCallback::connAccepted" << std::endl;

    auto sock = std::static_pointer_cast<apache::thrift::async::TAsyncSSLSocket>(s);

    hcb_->setSocket(sock);
    sock->getEventBase()->tryRunAfterDelay([=] {
        std::cerr << "Delayed SSL accept, client will have close by now"
                  << std::endl;
        // SSL accept will fail
        EXPECT_EQ(
          sock->getSSLState(),
          apache::thrift::async::TAsyncSSLSocket::STATE_UNINIT);
        hcb_->socket_->sslAccept(hcb_);
        // This registers for an event
        EXPECT_EQ(
          sock->getSSLState(),
          apache::thrift::async::TAsyncSSLSocket::STATE_ACCEPTING);

        state = STATE_SUCCEEDED;
      }, 100);
  }
};


class TestSSLServer {
 protected:
  apache::thrift::async::TEventBase evb_;
  std::shared_ptr<folly::SSLContext> ctx_;
  SSLServerAcceptCallbackBase *acb_;
  folly::AsyncServerSocket *socket_;
  folly::SocketAddress address_;
  pthread_t thread_;

  static void *Main(void *ctx) {
    TestSSLServer *self = static_cast<TestSSLServer*>(ctx);
    self->evb_.loop();
    std::cerr << "Server thread exited event loop" << std::endl;
    return nullptr;
  }

 public:
  // Create a TestSSLServer.
  // This immediately starts listening on the given port.
  explicit TestSSLServer(SSLServerAcceptCallbackBase *acb);

  // Kill the thread.
  ~TestSSLServer() {
    evb_.runInEventBaseThread([&](){
      socket_->stopAccepting();
    });
    std::cerr << "Waiting for server thread to exit" << std::endl;
    pthread_join(thread_, nullptr);
  }

  apache::thrift::async::TEventBase &getEventBase() { return evb_; }

  const folly::SocketAddress& getAddress() const {
    return address_;
  }
};

TestSSLServer::TestSSLServer(SSLServerAcceptCallbackBase *acb) :
ctx_(new folly::SSLContext),
    acb_(acb),
  socket_(new folly::AsyncServerSocket(&evb_)) {
  // Set up the SSL context
  ctx_->loadCertificate("thrift/lib/cpp/test/ssl/tests-cert.pem");
  ctx_->loadPrivateKey("thrift/lib/cpp/test/ssl/tests-key.pem");
  ctx_->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  acb_->ctx_ = ctx_;
  acb_->base_ = &evb_;

  //set up the listening socket
  socket_->bind(0);
  socket_->getAddress(&address_);
  socket_->listen(100);
  socket_->addAcceptCallback(acb_, &evb_);
  socket_->startAccepting();

  int ret = pthread_create(&thread_, nullptr, Main, this);
  assert(ret == 0);

  std::cerr << "Accepting connections on " << address_ << std::endl;
}


class TestSSLAsyncCacheServer : public TestSSLServer {
 public:
  explicit TestSSLAsyncCacheServer(SSLServerAcceptCallbackBase *acb,
        int lookupDelay = 100) :
      TestSSLServer(acb) {
    SSL_CTX *sslCtx = ctx_->getSSLCtx();
    SSL_CTX_sess_set_get_cb(sslCtx,
                            TestSSLAsyncCacheServer::getSessionCallback);
    SSL_CTX_set_session_cache_mode(
      sslCtx, SSL_SESS_CACHE_NO_INTERNAL | SSL_SESS_CACHE_SERVER);
    asyncCallbacks_ = 0;
    asyncLookups_ = 0;
    lookupDelay_ = lookupDelay;
  }

  uint32_t getAsyncCallbacks() const { return asyncCallbacks_; }
  uint32_t getAsyncLookups() const { return asyncLookups_; }

 private:
  static uint32_t asyncCallbacks_;
  static uint32_t asyncLookups_;
  static uint32_t lookupDelay_;

  static SSL_SESSION *getSessionCallback(SSL *ssl,
                                         unsigned char *sess_id,
                                         int id_len,
                                         int *copyflag) {
    *copyflag = 0;
    asyncCallbacks_++;
#ifdef SSL_ERROR_WANT_SESS_CACHE_LOOKUP
    if (!SSL_want_sess_cache_lookup(ssl)) {
      // libssl.so mismatch
      std::cerr << "no async support" << std::endl;
      return nullptr;
    }

    apache::thrift::async::TAsyncSSLSocket *sslSocket =
        apache::thrift::async::TAsyncSSLSocket::getFromSSL(ssl);
    assert(sslSocket != nullptr);
    // Going to simulate an async cache by just running delaying the miss 100ms
    if (asyncCallbacks_ % 2 == 0) {
      // This socket is already blocked on lookup, return miss
      std::cerr << "returning miss" << std::endl;
    } else {
      // fresh meat - block it
      std::cerr << "async lookup" << std::endl;
      sslSocket->getEventBase()->tryRunAfterDelay(
        std::bind(&apache::thrift::async::TAsyncSSLSocket::restartSSLAccept,
                  sslSocket), lookupDelay_);
      *copyflag = SSL_SESSION_CB_WOULD_BLOCK;
      asyncLookups_++;
    }
#endif
    return nullptr;
  }
};

void getfds(int fds[2]) {
  if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) != 0) {
    FAIL() << "failed to create socketpair: " << strerror(errno);
  }
  for (int idx = 0; idx < 2; ++idx) {
    int flags = fcntl(fds[idx], F_GETFL, 0);
    if (flags == -1) {
      FAIL() << "failed to get flags for socket " << idx << ": "
             << strerror(errno);
    }
    if (fcntl(fds[idx], F_SETFL, flags | O_NONBLOCK) != 0) {
      FAIL() << "failed to put socket " << idx << " in non-blocking mode: "
             << strerror(errno);
    }
  }
}

void getctx(
  std::shared_ptr<folly::SSLContext> clientCtx,
  std::shared_ptr<folly::SSLContext> serverCtx) {
  clientCtx->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  serverCtx->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  serverCtx->loadCertificate(
      "thrift/lib/cpp/test/ssl/tests-cert.pem");
  serverCtx->loadPrivateKey(
      "thrift/lib/cpp/test/ssl/tests-key.pem");
}

void sslsocketpair(
  apache::thrift::async::TEventBase* eventBase,
  apache::thrift::async::TAsyncSSLSocket::UniquePtr* clientSock,
  apache::thrift::async::TAsyncSSLSocket::UniquePtr* serverSock) {
  std::shared_ptr<folly::SSLContext> clientCtx(
    new folly::SSLContext);
  std::shared_ptr<folly::SSLContext> serverCtx(
    new folly::SSLContext);
  int fds[2];
  getfds(fds);
  getctx(clientCtx, serverCtx);
  clientSock->reset(new apache::thrift::async::TAsyncSSLSocket(
                      clientCtx, eventBase, fds[0], false));
  serverSock->reset(new apache::thrift::async::TAsyncSSLSocket(
                      serverCtx, eventBase, fds[1], true));

  // (*clientSock)->setSendTimeout(100);
  // (*serverSock)->setSendTimeout(100);
}

class BlockingWriteClient :
  private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
  private apache::thrift::async::TAsyncTransport::WriteCallback {
 public:
  explicit BlockingWriteClient(
    apache::thrift::async::TAsyncSSLSocket::UniquePtr socket)
    : socket_(std::move(socket)),
      bufLen_(2500),
      iovCount_(2000) {
    // Fill buf_
    buf_.reset(new uint8_t[bufLen_]);
    for (uint32_t n = 0; n < sizeof(buf_); ++n) {
      buf_[n] = n % 0xff;
    }

    // Initialize iov_
    iov_.reset(new struct iovec[iovCount_]);
    for (uint32_t n = 0; n < iovCount_; ++n) {
      iov_[n].iov_base = buf_.get() + n;
      if (n & 0x1) {
        iov_[n].iov_len = n % bufLen_;
      } else {
        iov_[n].iov_len = bufLen_ - (n % bufLen_);
      }
    }

    socket_->sslConnect(this, 100);
  }

  struct iovec* getIovec() const {
    return iov_.get();
  }
  uint32_t getIovecCount() const {
    return iovCount_;
  }

 private:
  virtual void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket*)
    noexcept {
    socket_->writev(this, iov_.get(), iovCount_);
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client handshake error: " << ex.what();
  }
  virtual void writeSuccess() noexcept {
    socket_->close();
  }
  virtual void writeError(
    size_t bytesWritten,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client write error after " << bytesWritten << " bytes: "
                  << ex.what();
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
  uint32_t bufLen_;
  uint32_t iovCount_;
  std::unique_ptr<uint8_t[]> buf_;
  std::unique_ptr<struct iovec[]> iov_;
};

class BlockingWriteServer :
    private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
    private apache::thrift::async::TAsyncTransport::ReadCallback {
 public:
  explicit BlockingWriteServer(
    apache::thrift::async::TAsyncSSLSocket::UniquePtr socket)
    : socket_(std::move(socket)),
      bufSize_(2500 * 2000),
      bytesRead_(0) {
    buf_.reset(new uint8_t[bufSize_]);
    socket_->sslAccept(this, 100);
  }

  void checkBuffer(struct iovec* iov, uint32_t count) const {
    uint32_t idx = 0;
    for (uint32_t n = 0; n < count; ++n) {
      size_t bytesLeft = bytesRead_ - idx;
      int rc = memcmp(buf_.get() + idx, iov[n].iov_base,
                      std::min(iov[n].iov_len, bytesLeft));
      if (rc != 0) {
        FAIL() << "buffer mismatch at iovec " << n << "/" << count
               << ": rc=" << rc;

      }
      if (iov[n].iov_len > bytesLeft) {
        FAIL() << "server did not read enough data: "
               << "ended at byte " << bytesLeft << "/" << iov[n].iov_len
               << " in iovec " << n << "/" << count;
      }

      idx += iov[n].iov_len;
    }
    if (idx != bytesRead_) {
      ADD_FAILURE() << "server read extra data: " << bytesRead_
                    << " bytes read; expected " << idx;
    }
  }

 private:
  virtual void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket*)
    noexcept {
    // Wait 10ms before reading, so the client's writes will initially block.
    socket_->getEventBase()->tryRunAfterDelay(
        [this] { socket_->setReadCallback(this); }, 10);
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server handshake error: " << ex.what();
  }
  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *bufReturn = buf_.get() + bytesRead_;
    *lenReturn = bufSize_ - bytesRead_;
  }
  virtual void readDataAvailable(size_t len) noexcept {
    bytesRead_ += len;
    socket_->setReadCallback(nullptr);
    socket_->getEventBase()->tryRunAfterDelay(
        [this] { socket_->setReadCallback(this); }, 2);
  }
  virtual void readEOF() noexcept {
    socket_->close();
  }
  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server read error: " << ex.what();
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
  uint32_t bufSize_;
  uint32_t bytesRead_;
  std::unique_ptr<uint8_t[]> buf_;
};

class NpnClient :
  private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
  private apache::thrift::async::TAsyncTransport::WriteCallback {
 public:
  explicit NpnClient(
    apache::thrift::async::TAsyncSSLSocket::UniquePtr socket)
      : nextProto(nullptr), nextProtoLength(0), socket_(std::move(socket)) {
    socket_->sslConnect(this);
  }

  const unsigned char* nextProto;
  unsigned nextProtoLength;
 private:
  virtual void handshakeSuccess(
    apache::thrift::async::TAsyncSSLSocket*) noexcept {
    socket_->getSelectedNextProtocol(&nextProto,
                                     &nextProtoLength);
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client handshake error: " << ex.what();
  }
  virtual void writeSuccess() noexcept {
    socket_->close();
  }
  virtual void writeError(
    size_t bytesWritten,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client write error after " << bytesWritten << " bytes: "
                  << ex.what();
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
};

class NpnServer :
    private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
    private apache::thrift::async::TAsyncTransport::ReadCallback {
 public:
  explicit NpnServer(apache::thrift::async::TAsyncSSLSocket::UniquePtr socket)
      : nextProto(nullptr), nextProtoLength(0), socket_(std::move(socket)) {
    socket_->sslAccept(this);
  }

  const unsigned char* nextProto;
  unsigned nextProtoLength;
 private:
  virtual void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket*)
    noexcept {
    socket_->getSelectedNextProtocol(&nextProto,
                                     &nextProtoLength);
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server handshake error: " << ex.what();
  }
  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *lenReturn = 0;
  }
  virtual void readDataAvailable(size_t len) noexcept {
  }
  virtual void readEOF() noexcept {
    socket_->close();
  }
  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server read error: " << ex.what();
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
};

#ifndef OPENSSL_NO_TLSEXT
class SNIClient :
  private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
  private apache::thrift::async::TAsyncTransport::WriteCallback {
 public:
  explicit SNIClient(
    apache::thrift::async::TAsyncSSLSocket::UniquePtr socket)
      : serverNameMatch(false), socket_(std::move(socket)) {
    socket_->sslConnect(this);
  }

  bool serverNameMatch;

 private:
  virtual void handshakeSuccess(
    apache::thrift::async::TAsyncSSLSocket*) noexcept {
    serverNameMatch = socket_->isServerNameMatch();
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client handshake error: " << ex.what();
  }
  virtual void writeSuccess() noexcept {
    socket_->close();
  }
  virtual void writeError(
    size_t bytesWritten,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client write error after " << bytesWritten << " bytes: "
                  << ex.what();
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
};

class SNIServer :
    private apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
    private apache::thrift::async::TAsyncTransport::ReadCallback {
 public:
  explicit SNIServer(
    apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
    const std::shared_ptr<folly::SSLContext>& ctx,
    const std::shared_ptr<folly::SSLContext>& sniCtx,
    const std::string& expectedServerName)
      : serverNameMatch(false), socket_(std::move(socket)), sniCtx_(sniCtx),
        expectedServerName_(expectedServerName) {
    ctx->setServerNameCallback(std::bind(&SNIServer::serverNameCallback, this,
                                         std::placeholders::_1));
    socket_->sslAccept(this);
  }

  bool serverNameMatch;

 private:
  virtual void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket* ssl)
    noexcept {
  }
  virtual void handshakeError(
    apache::thrift::async::TAsyncSSLSocket*,
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server handshake error: " << ex.what();
  }
  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *lenReturn = 0;
  }
  virtual void readDataAvailable(size_t len) noexcept {
  }
  virtual void readEOF() noexcept {
    socket_->close();
  }
  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "server read error: " << ex.what();
  }

  folly::SSLContext::ServerNameCallbackResult
    serverNameCallback(SSL *ssl) {
    const char *sn = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (sniCtx_ &&
        sn &&
        !strcasecmp(expectedServerName_.c_str(), sn)) {
      apache::thrift::async::TAsyncSSLSocket *sslSocket =
          apache::thrift::async::TAsyncSSLSocket::getFromSSL(ssl);
      sslSocket->switchServerSSLContext(sniCtx_);
      serverNameMatch = true;
      return folly::SSLContext::SERVER_NAME_FOUND;
    } else {
      serverNameMatch = false;
      return folly::SSLContext::SERVER_NAME_NOT_FOUND;
    }
  }

  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
  std::shared_ptr<folly::SSLContext> sniCtx_;
  std::string expectedServerName_;
};
#endif

class SSLClient : public apache::thrift::async::TAsyncSocket::ConnectCallback,
                  public apache::thrift::async::TAsyncTransport::WriteCallback,
                  public apache::thrift::async::TAsyncTransport::ReadCallback
{
 private:
  apache::thrift::async::TEventBase *eventBase_;
  std::shared_ptr<apache::thrift::async::TAsyncSSLSocket> sslSocket_;
  SSL_SESSION *session_;
  std::shared_ptr<folly::SSLContext> ctx_;
  uint32_t requests_;
  folly::SocketAddress address_;
  uint32_t timeout_;
  char buf_[128];
  char readbuf_[128];
  uint32_t bytesRead_;
  uint32_t hit_;
  uint32_t miss_;
  uint32_t errors_;
  uint32_t writeAfterConnectErrors_;

 public:
  SSLClient(apache::thrift::async::TEventBase *eventBase,
            const folly::SocketAddress& address,
            uint32_t requests, uint32_t timeout = 0)
      : eventBase_(eventBase),
        session_(nullptr),
        requests_(requests),
        address_(address),
        timeout_(timeout),
        bytesRead_(0),
        hit_(0),
        miss_(0),
        errors_(0),
        writeAfterConnectErrors_(0) {
    ctx_.reset(new folly::SSLContext());
    ctx_->setOptions(SSL_OP_NO_TICKET);
    ctx_->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    memset(buf_, 'a', sizeof(buf_));
  }

  ~SSLClient() {
    if (session_) {
      SSL_SESSION_free(session_);
    }
    if (errors_ == 0) {
      EXPECT_EQ(bytesRead_, sizeof(buf_));
    }
  }

  uint32_t getHit() const { return hit_; }

  uint32_t getMiss() const { return miss_; }

  uint32_t getErrors() const { return errors_; }

  uint32_t getWriteAfterConnectErrors() const {
    return writeAfterConnectErrors_;
  }

  void connect(bool writeNow = false) {
    sslSocket_ = apache::thrift::async::TAsyncSSLSocket::newSocket(
      ctx_, eventBase_);
    if (session_ != nullptr) {
      sslSocket_->setSSLSession(session_);
    }
    requests_--;
    sslSocket_->connect(this, address_, timeout_);
    if (sslSocket_ && writeNow) {
      // write some junk, used in an error test
      sslSocket_->write(this, buf_, sizeof(buf_));
    }
  }

  virtual void connectSuccess() noexcept {
    std::cerr << "client SSL socket connected" << std::endl;
    if (sslSocket_->getSSLSessionReused()) {
      hit_++;
    } else {
      miss_++;
      if (session_ != nullptr) {
        SSL_SESSION_free(session_);
      }
      session_ = sslSocket_->getSSLSession();
    }

    // write()
    sslSocket_->write(this, buf_, sizeof(buf_));
    sslSocket_->setReadCallback(this);
    memset(readbuf_, 'b', sizeof(readbuf_));
    bytesRead_ = 0;
  }

  virtual void connectError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "SSLClient::connectError: " << ex.what() << std::endl;
    errors_++;
    sslSocket_.reset();
  }

  virtual void writeSuccess() noexcept {
    std::cerr << "client write success" << std::endl;
  }

  virtual void writeError(
    size_t bytesWritten,
    const apache::thrift::transport::TTransportException& ex)
    noexcept {
    std::cerr << "client writeError: " << ex.what() << std::endl;
    if (!sslSocket_) {
      writeAfterConnectErrors_++;
    }
  }

  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *bufReturn = readbuf_ + bytesRead_;
    *lenReturn = sizeof(readbuf_) - bytesRead_;
  }

  virtual void readEOF() noexcept {
    std::cerr << "client readEOF" << std::endl;
  }

  virtual void readError(
    const apache::thrift::transport::TTransportException& ex) noexcept {
    std::cerr << "client readError: " << ex.what() << std::endl;
  }

  virtual void readDataAvailable(size_t len) noexcept {
    std::cerr << "client read data: " << len << std::endl;
    bytesRead_ += len;
    if (len == sizeof(buf_)) {
      EXPECT_EQ(memcmp(buf_, readbuf_, bytesRead_), 0);
      sslSocket_->closeNow();
      sslSocket_.reset();
      if (requests_ != 0) {
        connect();
      }
    }
  }

};

class SSLHandshakeBase :
  public apache::thrift::async::TAsyncSSLSocket::HandshakeCallback,
  private apache::thrift::async::TAsyncTransport::WriteCallback {
 public:
  explicit SSLHandshakeBase(
   apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
   bool preverifyResult,
   bool verifyResult) :
    handshakeVerify_(false),
    handshakeSuccess_(false),
    handshakeError_(false),
    socket_(std::move(socket)),
    preverifyResult_(preverifyResult),
    verifyResult_(verifyResult) {
  }

  bool handshakeVerify_;
  bool handshakeSuccess_;
  bool handshakeError_;

 protected:
  apache::thrift::async::TAsyncSSLSocket::UniquePtr socket_;
  bool preverifyResult_;
  bool verifyResult_;

  // HandshakeCallback
  bool handshakeVerify(
   apache::thrift::async::TAsyncSSLSocket* sock,
   bool preverifyOk,
   X509_STORE_CTX* ctx) noexcept {
    handshakeVerify_ = true;

    EXPECT_EQ(preverifyResult_, preverifyOk);
    return verifyResult_;
  }

  void handshakeSuccess(
   apache::thrift::async::TAsyncSSLSocket*) noexcept {
    handshakeSuccess_ = true;
  }

  void handshakeError(
   apache::thrift::async::TAsyncSSLSocket*,
   const apache::thrift::transport::TTransportException& ex) noexcept {
    handshakeError_ = true;
  }

  // WriteCallback
  void writeSuccess() noexcept {
    socket_->close();
  }

  void writeError(
   size_t bytesWritten,
   const apache::thrift::transport::TTransportException& ex) noexcept {
    ADD_FAILURE() << "client write error after " << bytesWritten << " bytes: "
                  << ex.what();
  }
};

class SSLHandshakeClient : public SSLHandshakeBase {
 public:
  SSLHandshakeClient(
   apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
   bool preverifyResult,
   bool verifyResult) :
    SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslConnect(this, 0);
  }
};

class SSLHandshakeClientNoVerify : public SSLHandshakeBase {
 public:
  SSLHandshakeClientNoVerify(
   apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
   bool preverifyResult,
   bool verifyResult) :
    SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslConnect(this, 0,
      folly::SSLContext::SSLVerifyPeerEnum::NO_VERIFY);
  }
};

class SSLHandshakeClientDoVerify : public SSLHandshakeBase {
 public:
  SSLHandshakeClientDoVerify(
   apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
   bool preverifyResult,
   bool verifyResult) :
    SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslConnect(this, 0,
      folly::SSLContext::SSLVerifyPeerEnum::VERIFY);
  }
};

class SSLHandshakeServer : public SSLHandshakeBase {
 public:
  SSLHandshakeServer(
      apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
      bool preverifyResult,
      bool verifyResult)
    : SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslAccept(this, 0);
  }
};

class SSLHandshakeServerParseClientHello : public SSLHandshakeBase {
 public:
  SSLHandshakeServerParseClientHello(
      apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
      bool preverifyResult,
      bool verifyResult)
      : SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->enableClientHelloParsing();
    socket_->sslAccept(this, 0);
  }

  std::string clientCiphers_, sharedCiphers_, serverCiphers_, chosenCipher_;

 protected:
  void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket* sock) noexcept {
    handshakeSuccess_ = true;
    sock->getSSLSharedCiphers(sharedCiphers_);
    sock->getSSLServerCiphers(serverCiphers_);
    sock->getSSLClientCiphers(clientCiphers_);
    chosenCipher_ = sock->getNegotiatedCipherName();
  }
};


class SSLHandshakeServerNoVerify : public SSLHandshakeBase {
 public:
  SSLHandshakeServerNoVerify(
      apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
      bool preverifyResult,
      bool verifyResult)
    : SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslAccept(this, 0,
      folly::SSLContext::SSLVerifyPeerEnum::NO_VERIFY);
  }
};

class SSLHandshakeServerDoVerify : public SSLHandshakeBase {
 public:
  SSLHandshakeServerDoVerify(
      apache::thrift::async::TAsyncSSLSocket::UniquePtr socket,
      bool preverifyResult,
      bool verifyResult)
    : SSLHandshakeBase(std::move(socket), preverifyResult, verifyResult) {
    socket_->sslAccept(this, 0,
      folly::SSLContext::SSLVerifyPeerEnum::VERIFY_REQ_CLIENT_CERT);
  }
};

class EventBaseAborter : public apache::thrift::async::TAsyncTimeout {
 public:
  EventBaseAborter(apache::thrift::async::TEventBase* eventBase,
                   uint32_t timeoutMS)
    : apache::thrift::async::TAsyncTimeout(
      eventBase, apache::thrift::async::TAsyncTimeout::InternalEnum::INTERNAL)
    , eventBase_(eventBase) {
    scheduleTimeout(timeoutMS);
  }

  virtual void timeoutExpired() noexcept {
    FAIL() << "test timed out";
    eventBase_->terminateLoopSoon();
  }

 private:
  apache::thrift::async::TEventBase* eventBase_;
};
