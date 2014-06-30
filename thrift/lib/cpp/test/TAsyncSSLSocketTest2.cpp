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
#include <thrift/lib/cpp/test/TAsyncSSLSocketTest.h>

#include <gtest/gtest.h>
#include <pthread.h>

#include <thrift/lib/cpp/async/TAsyncSSLServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>

using std::string;
using std::vector;
using std::min;
using std::cerr;
using std::endl;
using std::list;
using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::async::TAsyncSSLServerSocket;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::async::TEventBase;
using apache::thrift::concurrency::Util;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::TTransportException;
using apache::thrift::transport::SSLContext;
using apache::thrift::transport::TSSLSocket;
using apache::thrift::transport::TSSLException;

class AttachDetachClient : public TAsyncSocket::ConnectCallback,
                           public TAsyncTransport::WriteCallback,
                           public TAsyncTransport::ReadCallback {
 private:
  TEventBase *eventBase_;
  std::shared_ptr<TAsyncSSLSocket> sslSocket_;
  std::shared_ptr<SSLContext> ctx_;
  TSocketAddress address_;
  char buf_[128];
  char readbuf_[128];
  uint32_t bytesRead_;
 public:
  AttachDetachClient(TEventBase *eventBase, const TSocketAddress& address)
      : eventBase_(eventBase), address_(address), bytesRead_(0) {
    ctx_.reset(new SSLContext());
    ctx_->setOptions(SSL_OP_NO_TICKET);
    ctx_->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  }

  void connect() {
    sslSocket_ = TAsyncSSLSocket::newSocket(ctx_, eventBase_);
    sslSocket_->connect(this, address_);
  }

  virtual void connectSuccess() noexcept {
    cerr << "client SSL socket connected" << endl;

    for (int i = 0; i < 1000; ++i) {
      sslSocket_->detachSSLContext();
      sslSocket_->attachSSLContext(ctx_);
    }

    EXPECT_EQ(ctx_->getSSLCtx()->references, 2);

    sslSocket_->write(this, buf_, sizeof(buf_));
    sslSocket_->setReadCallback(this);
    memset(readbuf_, 'b', sizeof(readbuf_));
    bytesRead_ = 0;
  }

  virtual void connectError(const TTransportException& ex) noexcept
  {
    cerr << "AttachDetachClient::connectError: " << ex.what() << endl;
    sslSocket_.reset();
  }

  virtual void writeSuccess() noexcept {
    cerr << "client write success" << endl;
  }

  virtual void writeError(size_t bytesWritten, const TTransportException&
  ex)
    noexcept {
    cerr << "client writeError: " << ex.what() << endl;
  }

  virtual void getReadBuffer(void** bufReturn, size_t* lenReturn) {
    *bufReturn = readbuf_ + bytesRead_;
    *lenReturn = sizeof(readbuf_) - bytesRead_;
  }
  virtual void readEOF() noexcept {
    cerr << "client readEOF" << endl;
  }

  virtual void readError(const TTransportException& ex) noexcept {
    cerr << "client readError: " << ex.what() << endl;
  }

  virtual void readDataAvailable(size_t len) noexcept {
    cerr << "client read data: " << len << endl;
    bytesRead_ += len;
    if (len == sizeof(buf_)) {
      EXPECT_EQ(memcmp(buf_, readbuf_, bytesRead_), 0);
      sslSocket_->closeNow();
    }
  }
};

/**
 * Test passing contexts between threads
 */
TEST(TAsyncSSLSocketTest2, AttachDetachSSLContext) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallbackDelay acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  TEventBase eventBase;
  EventBaseAborter eba(&eventBase, 3000);
  std::shared_ptr<AttachDetachClient> client(
    new AttachDetachClient(&eventBase, server.getAddress()));

  client->connect();
  eventBase.loop();
}


///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

namespace {
struct Initializer {
  Initializer() {
    signal(SIGPIPE, SIG_IGN);
    apache::thrift::transport::SSLContext::setSSLLockTypes({
        {CRYPTO_LOCK_EVP_PKEY, SSLContext::LOCK_NONE},
        {CRYPTO_LOCK_SSL_SESSION, SSLContext::LOCK_SPINLOCK},
        {CRYPTO_LOCK_SSL_CTX, SSLContext::LOCK_NONE}});
  }
};
Initializer initializer;
} // anonymous
