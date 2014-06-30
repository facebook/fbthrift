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
#include <thrift/lib/cpp/async/TAsyncUDPSocket.h>

#include <thrift/lib/cpp/async/TEventBase.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
using apache::thrift::transport::TTransportException;

namespace apache { namespace thrift { namespace async {

TAsyncUDPSocket::TAsyncUDPSocket(TEventBase* evb)
    : TEventHandler(CHECK_NOTNULL(evb)),
      eventBase_(evb),
      fd_(-1),
      readCallback_(nullptr) {
  DCHECK(evb->isInEventBaseThread());
}

TAsyncUDPSocket::~TAsyncUDPSocket() {
  if (fd_ != -1) {
    close();
  }
}

void TAsyncUDPSocket::bind(const transport::TSocketAddress& address) {
  int socket = ::socket(address.getFamily(), SOCK_DGRAM, IPPROTO_UDP);
  if (socket == -1) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "error creating async udp socket",
                              errno);
  }

  auto g = folly::makeGuard([&] { ::close(socket); });

  // put the socket in non-blocking mode
  int ret = fcntl(socket, F_SETFL, O_NONBLOCK);
  if (ret != 0) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "failed to put socket in non-blocking mode",
                              errno);
  }

  // put the socket in reuse mode
  int value = 1;
  if (setsockopt(socket,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 &value,
                 sizeof(value)) != 0) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "failed to put socket in reuse mode",
                              errno);
  }

  // bind to the address
  if (::bind(socket, address.getAddress(), address.getActualSize()) != 0) {
    throw TTransportException(
        TTransportException::NOT_OPEN,
        "failed to bind the async udp socket for:" + address.describe(),
        errno);
  }

  // success
  g.dismiss();
  fd_ = socket;
  ownership_ = FDOwnership::OWNS;

  // attach to TEventHandler
  TEventHandler::changeHandlerFD(fd_);

  if (address.getPort() != 0) {
    localAddress_ = address;
  } else {
    localAddress_.setFromLocalAddress(fd_);
  }
}

void TAsyncUDPSocket::setFD(int fd, FDOwnership ownership) {
  CHECK_EQ(-1, fd_) << "Already bound to another FD";

  fd_ = fd;
  ownership_ = ownership;

  TEventHandler::changeHandlerFD(fd_);
  localAddress_.setFromLocalAddress(fd_);
}

ssize_t TAsyncUDPSocket::write(const transport::TSocketAddress& address,
                               const std::unique_ptr<folly::IOBuf>& buf) {
  CHECK_NE(-1, fd_) << "Socket not yet bound";

  // XXX: Use `sendmsg` instead of coalescing here
  buf->coalesce();

  return ::sendto(fd_,
                  buf->data(),
                  buf->length(),
                  MSG_DONTWAIT,
                  address.getAddress(),
                  address.getActualSize());
}

void TAsyncUDPSocket::resumeRead(ReadCallback* cob) {
  CHECK(!readCallback_) << "Another read callback already installed";
  CHECK_NE(-1, fd_) << "UDP server socket not yet bind to an address";

  readCallback_ = CHECK_NOTNULL(cob);
  if (!updateRegistration()) {
    TTransportException ex(TTransportException::NOT_OPEN,
                           "failed to register for accept events");

    readCallback_ = nullptr;
    cob->onReadError(ex);
    return;
  }
}

void TAsyncUDPSocket::pauseRead() {
  // It is ok to pause an already paused socket
  readCallback_ = nullptr;
  updateRegistration();
}

void TAsyncUDPSocket::close() {
  DCHECK(eventBase_->isInEventBaseThread());

  if (readCallback_) {
    auto cob = readCallback_;
    readCallback_ = nullptr;

    cob->onReadClosed();
  }

  // Unregister any events we are registered for
  unregisterHandler();

  if (fd_ != -1 && ownership_ == FDOwnership::OWNS) {
    ::close(fd_);
  }

  fd_ = -1;
}

void TAsyncUDPSocket::handlerReady(uint16_t events) noexcept {
  if (events & TEventHandler::READ) {
    DCHECK(readCallback_);
    handleRead();
  }
}

void TAsyncUDPSocket::handleRead() noexcept {
  void* buf{nullptr};
  size_t len{0};

  readCallback_->getReadBuffer(&buf, &len);
  if (buf == nullptr || len == 0) {
    TTransportException ex(
        TTransportException::BAD_ARGS,
        "TAsyncUDPSocket::getReadBuffer() returned empty buffer");


    auto cob = readCallback_;
    readCallback_ = nullptr;

    cob->onReadError(ex);
    updateRegistration();
    return;
  }

  socklen_t addrLen;
  struct sockaddr* rawAddr =
      clientAddress_.getMutableAddress(localAddress_.getFamily(), &addrLen);

  ssize_t bytesRead = ::recvfrom(fd_, buf, len, MSG_TRUNC, rawAddr, &addrLen);
  if (bytesRead >= 0) {
    clientAddress_.addressUpdated(localAddress_.getFamily(), addrLen);

    if (bytesRead > 0) {
      bool truncated = false;
      if (bytesRead > len) {
        truncated = true;
        bytesRead = len;
      }

      readCallback_->onDataAvailable(clientAddress_, bytesRead, truncated);
    }
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // No data could be read without blocking the socket
      return;
    }

    TTransportException ex(TTransportException::INTERNAL_ERROR,
                           "::recvfrom() failed",
                           errno);

    // In case of UDP we can continue reading from the socket
    // even if the current request fails. We notify the user
    // so that he can do some logging/stats collection if he wants.
    auto cob = readCallback_;
    readCallback_ = nullptr;

    cob->onReadError(ex);
    updateRegistration();
  }
}

bool TAsyncUDPSocket::updateRegistration() noexcept {
  uint16_t flags = NONE;

  if (readCallback_) {
    flags |= READ;
  }

  return registerHandler(flags | PERSIST);
}

}}}
