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
#include "thrift/lib/cpp/async/TAsyncSSLServerSocket.h"

#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/lib/cpp/transport/TTransportException.h"

using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::SSLContext;
using apache::thrift::transport::TTransportException;
using std::shared_ptr;

namespace apache { namespace thrift { namespace async {

TAsyncSSLServerSocket::TAsyncSSLServerSocket(
      const shared_ptr<SSLContext>& ctx,
      TEventBase* eventBase)
    : eventBase_(eventBase)
    , serverSocket_(new TAsyncServerSocket(eventBase))
    , ctx_(ctx)
    , sslCallback_(nullptr) {
}

TAsyncSSLServerSocket::~TAsyncSSLServerSocket() {
}

void TAsyncSSLServerSocket::destroy() {
  // Stop accepting on the underlying socket as soon as destroy is called
  if (sslCallback_ != nullptr) {
    serverSocket_->pauseAccepting();
    serverSocket_->removeAcceptCallback(this, nullptr);
  }
  serverSocket_->destroy();
  serverSocket_ = nullptr;
  sslCallback_ = nullptr;

  TDelayedDestruction::destroy();
}

void TAsyncSSLServerSocket::setSSLAcceptCallback(SSLAcceptCallback* callback) {
  SSLAcceptCallback *oldCallback = sslCallback_;
  sslCallback_ = callback;
  if (callback != nullptr && oldCallback == nullptr) {
    serverSocket_->addAcceptCallback(this, nullptr);
    serverSocket_->startAccepting();
  } else if (callback == nullptr && oldCallback != nullptr) {
    serverSocket_->removeAcceptCallback(this, nullptr);
    serverSocket_->pauseAccepting();
  }
}

void TAsyncSSLServerSocket::attachEventBase(TEventBase* eventBase) {
  assert(sslCallback_ == nullptr);
  eventBase_ = eventBase;
  serverSocket_->attachEventBase(eventBase);
}

void TAsyncSSLServerSocket::detachEventBase() {
  serverSocket_->detachEventBase();
  eventBase_ = nullptr;
}

void
TAsyncSSLServerSocket::connectionAccepted(
  int fd,
  const TSocketAddress& clientAddr) noexcept {
  shared_ptr<TAsyncSSLSocket> sslSock;
  try {
    // Create a TAsyncSSLSocket object with the fd. The socket should be
    // added to the event base and in the state of accepting SSL connection.
    sslSock = TAsyncSSLSocket::newSocket(ctx_, eventBase_, fd);
  } catch (const std::exception &e) {
    T_ERROR("Exception %s caught while creating a TAsyncSSLSocket "
            "object with socket %d", e.what(), fd);
    ::close(fd);
    sslCallback_->acceptError(e);
    return;
  }

  // TODO: Perform the SSL handshake before invoking the callback
  sslCallback_->connectionAccepted(sslSock);
}

void TAsyncSSLServerSocket::acceptError(const std::exception& ex)
  noexcept {
  T_ERROR("TAsyncSSLServerSocket accept error: %s", ex.what());
  sslCallback_->acceptError(ex);
}

}}} // apache::thrift::async
