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

#include <thrift/lib/cpp/transport/TServerSocket.h>

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <thrift/lib/cpp/transport/TSocket.h>
#include <thrift/lib/cpp/util/FdUtils.h>
#include <memory>

namespace apache { namespace thrift { namespace transport {

using namespace std;
using std::shared_ptr;

TServerSocket::TServerSocket(int port) :
  port_(port),
  serverSocket_(-1),
  acceptBacklog_(1024),
  sendTimeout_(0),
  recvTimeout_(0),
  accTimeout_(-1),
  retryLimit_(0),
  retryDelay_(0),
  tcpSendBuffer_(0),
  tcpRecvBuffer_(0),
  intSock1_(-1),
  intSock2_(-1),
  closeOnExec_(true) {}

TServerSocket::TServerSocket(int port, int sendTimeout, int recvTimeout) :
  port_(port),
  serverSocket_(-1),
  acceptBacklog_(1024),
  sendTimeout_(sendTimeout),
  recvTimeout_(recvTimeout),
  accTimeout_(-1),
  retryLimit_(0),
  retryDelay_(0),
  tcpSendBuffer_(0),
  tcpRecvBuffer_(0),
  intSock1_(-1),
  intSock2_(-1),
  closeOnExec_(true) {}

TServerSocket::TServerSocket(string path)
  : port_(0),
    path_(path),
    serverSocket_(-1),
    acceptBacklog_(1024),
    sendTimeout_(0),
    recvTimeout_(0),
    accTimeout_(-1),
    retryLimit_(0),
    retryDelay_(0),
    tcpSendBuffer_(0),
    tcpRecvBuffer_(0),
    intSock1_(-1),
    intSock2_(-1) {
}

TServerSocket::~TServerSocket() {
  close();
}

void TServerSocket::setSendTimeout(int sendTimeout) {
  sendTimeout_ = sendTimeout;
}

void TServerSocket::setRecvTimeout(int recvTimeout) {
  recvTimeout_ = recvTimeout;
}

void TServerSocket::setAcceptTimeout(int accTimeout) {
  accTimeout_ = accTimeout;
}

void TServerSocket::setRetryLimit(int retryLimit) {
  retryLimit_ = retryLimit;
}

void TServerSocket::setRetryDelay(int retryDelay) {
  retryDelay_ = retryDelay;
}

void TServerSocket::setTcpSendBuffer(int tcpSendBuffer) {
  tcpSendBuffer_ = tcpSendBuffer;
}

void TServerSocket::setTcpRecvBuffer(int tcpRecvBuffer) {
  tcpRecvBuffer_ = tcpRecvBuffer;
}

void TServerSocket::setCloseOnExec(bool closeOnExec) {
  closeOnExec_ = closeOnExec;
}

void TServerSocket::listen() {
  int sv[2];
  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sv)) {
    GlobalOutput.perror("TServerSocket::listen() socketpair() ", errno);
    intSock1_ = -1;
    intSock2_ = -1;
  } else {
    intSock1_ = sv[1];
    intSock2_ = sv[0];
  }

  struct addrinfo hints, *res, *res0;
  int error;
  char port[sizeof("65536") + 1];
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  sprintf(port, "%d", port_);

  // Wildcard address
  error = getaddrinfo(nullptr, port, &hints, &res0);
  if (error) {
    GlobalOutput.printf("getaddrinfo %d: %s", error, gai_strerror(error));
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not resolve host for server socket.");
  }

  // Pick the ipv6 address first since ipv4 addresses can be mapped
  // into ipv6 space.
  for (res = res0; res; res = res->ai_next) {
    if (res->ai_family == AF_INET6 || res->ai_next == nullptr)
      break;
  }

  if (!path_.empty()) {
    serverSocket_ = socket(PF_UNIX, SOCK_STREAM, IPPROTO_IP);
  } else {
    serverSocket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  }

  if (serverSocket_ == -1) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() socket() ", errno_copy);
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not create server socket.", errno_copy);
  }

  // Set reusaddress to prevent 2MSL delay on accept
  int one = 1;
  if (-1 == setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR,
                       &one, sizeof(one))) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() setsockopt() SO_REUSEADDR ", errno_copy);
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not set SO_REUSEADDR", errno_copy);
  }

  // Setup FD_CLOEXEC flag
  if (closeOnExec_ &&
      (-1 == apache::thrift::util::setCloseOnExec(serverSocket_,
                                                  closeOnExec_))) {
    GlobalOutput.perror("fcntl/F_SETFD (TServerSocket)", errno);
  }

  // Set TCP buffer sizes
  if (tcpSendBuffer_ > 0) {
    if (-1 == setsockopt(serverSocket_, SOL_SOCKET, SO_SNDBUF,
                         &tcpSendBuffer_, sizeof(tcpSendBuffer_))) {
      int errno_copy = errno;
      GlobalOutput.perror("TServerSocket::listen() setsockopt() SO_SNDBUF ", errno_copy);
      close();
      throw TTransportException(TTransportException::NOT_OPEN, "Could not set SO_SNDBUF", errno_copy);
    }
  }

  if (tcpRecvBuffer_ > 0) {
    if (-1 == setsockopt(serverSocket_, SOL_SOCKET, SO_RCVBUF,
                         &tcpRecvBuffer_, sizeof(tcpRecvBuffer_))) {
      int errno_copy = errno;
      GlobalOutput.perror("TServerSocket::listen() setsockopt() SO_RCVBUF ", errno_copy);
      close();
      throw TTransportException(TTransportException::NOT_OPEN, "Could not set SO_RCVBUF", errno_copy);
    }
  }

  // Defer accept
  #ifdef TCP_DEFER_ACCEPT
  if (path_.empty()) {
    if (-1 == setsockopt(serverSocket_, SOL_SOCKET, TCP_DEFER_ACCEPT,
                         &one, sizeof(one))) {
      int errno_copy = errno;
      GlobalOutput.perror(
          "TServerSocket::listen() setsockopt() TCP_DEFER_ACCEPT ",
          errno_copy);
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set TCP_DEFER_ACCEPT",
                                errno_copy);
    }
  }
  #endif // #ifdef TCP_DEFER_ACCEPT

#if 0 // XXX: Until this supports 2 sockets, we can't bind only to IPv6
  #ifdef IPV6_V6ONLY
  if (res->ai_family == AF_INET6) {
    int zero = 0;
    if (-1 == setsockopt(serverSocket_, IPPROTO_IPV6, IPV6_V6ONLY,
          &zero, sizeof(zero))) {
      GlobalOutput.perror("TServerSocket::listen() IPV6_V6ONLY ", errno);
    }
  }
  #endif // #ifdef IPV6_V6ONLY
#endif

  // Turn linger off, don't want to block on calls to close
  struct linger ling = {0, 0};
  if (-1 == setsockopt(serverSocket_, SOL_SOCKET, SO_LINGER,
                       &ling, sizeof(ling))) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() setsockopt() SO_LINGER ", errno_copy);
    close();
    throw TTransportException(TTransportException::NOT_OPEN, "Could not set SO_LINGER", errno_copy);
  }

  // TCP Nodelay, speed over bandwidth
  if (path_.empty()) {
    if (-1 == setsockopt(serverSocket_, IPPROTO_TCP, TCP_NODELAY,
                         &one, sizeof(one))) {
      int errno_copy = errno;
      GlobalOutput.perror("TServerSocket::listen() setsockopt() TCP_NODELAY ",
                          errno_copy);
      close();
      throw TTransportException(TTransportException::NOT_OPEN,
                                "Could not set TCP_NODELAY",
                                errno_copy);
    }
  }

  // Set NONBLOCK on the accept socket
  int flags = fcntl(serverSocket_, F_GETFL, 0);
  if (flags == -1) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() fcntl() F_GETFL ", errno_copy);
    throw TTransportException(TTransportException::NOT_OPEN, "fcntl() failed", errno_copy);
  }

  if (-1 == fcntl(serverSocket_, F_SETFL, flags | O_NONBLOCK)) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() fcntl() O_NONBLOCK ", errno_copy);
    throw TTransportException(TTransportException::NOT_OPEN, "fcntl() failed", errno_copy);
  }

  // prepare the port information
  // we may want to try to bind more than once, since SO_REUSEADDR doesn't
  // always seem to work. The client can configure the retry variables.
  int retries = 0;

  if (!path_.empty()) {
    // Unix Domain Socket
    size_t len = path_.size() + 1;
    if (len > sizeof(((sockaddr_un*)nullptr)->sun_path)) {
      int errno_copy = errno;
      GlobalOutput.perror("TSocket::listen() Unix Domain socket path too long",
                          errno_copy);
      throw TTransportException(TTransportException::NOT_OPEN,
                                " Unix Domain socket path too long");
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    memcpy(address.sun_path, path_.c_str(), len);
    socklen_t structlen = static_cast<socklen_t>(sizeof(address));

    do {
      if (0 == ::bind(serverSocket_, (struct sockaddr*)&address, structlen)) {
        break;
      }
      // use short circuit evaluation here to only sleep if we need to
    } while ((retries++ < retryLimit_) && (sleep(retryDelay_) == 0));
  } else {
    do {
      if (0 == ::bind(serverSocket_, res->ai_addr, res->ai_addrlen)) {
        break;
      }

      // use short circuit evaluation here to only sleep if we need to
    } while ((retries++ < retryLimit_) && (sleep(retryDelay_) == 0));

    // free addrinfo
    freeaddrinfo(res0);
  }

  // throw an error if we failed to bind properly
  if (retries > retryLimit_) {
    char errbuf[1024];
    if (!path_.empty()) {
      sprintf(errbuf, "TServerSocket::listen() PATH %s", path_.c_str());
    } else {
      sprintf(errbuf, "TServerSocket::listen() BIND %d", port_);
    }
    GlobalOutput(errbuf);
    int errno_copy = errno;
    close();
    throw TTransportException(TTransportException::COULD_NOT_BIND,
                              "Could not bind", errno_copy);
  }

  // Call listen
  if (-1 == ::listen(serverSocket_, acceptBacklog_)) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::listen() listen() ", errno_copy);
    close();
    throw TTransportException(TTransportException::NOT_OPEN,
                              "Could not listen", errno_copy);
  }

  // The socket is now listening!
}

shared_ptr<TRpcTransport> TServerSocket::acceptImpl() {
  if (serverSocket_ < 0) {
    throw TTransportException(TTransportException::NOT_OPEN, "TServerSocket not listening");
  }

  struct pollfd fds[2];

  int maxEintrs = 5;
  int numEintrs = 0;

  while (true) {
    std::memset(fds, 0 , sizeof(fds));
    fds[0].fd = serverSocket_;
    fds[0].events = POLLIN;
    if (intSock2_ >= 0) {
      fds[1].fd = intSock2_;
      fds[1].events = POLLIN;
    }
    /*
      TODO: (weichen) if EINTR is received, we'll restart the timeout.
      To be accurate, we need to fix this in the future.
     */
    int ret = poll(fds, 2, accTimeout_);

    if (ret < 0) {
      // error cases
      if (errno == EINTR && (numEintrs++ < maxEintrs)) {
        // EINTR needs to be handled manually and we can tolerate
        // a certain number
        continue;
      }
      int errno_copy = errno;
      GlobalOutput.perror("TServerSocket::acceptImpl() poll() ", errno_copy);
      throw TTransportException(TTransportException::UNKNOWN, "Unknown", errno_copy);
    } else if (ret > 0) {
      // Check for an interrupt signal
      if (intSock2_ >= 0 && (fds[1].revents & POLLIN)) {
        int8_t buf;
        if (-1 == recv(intSock2_, &buf, sizeof(int8_t), 0)) {
          GlobalOutput.perror("TServerSocket::acceptImpl() recv() interrupt ", errno);
        }
        throw TTransportException(TTransportException::INTERRUPTED);
      }

      // Check for the actual server socket being ready
      if (fds[0].revents & POLLIN) {
        break;
      }
    } else {
      GlobalOutput("TServerSocket::acceptImpl() poll 0");
      throw TTransportException(TTransportException::UNKNOWN);
    }
  }

  struct sockaddr_storage clientAddress;
  int size = sizeof(clientAddress);
  int clientSocket = ::accept(serverSocket_,
                              (struct sockaddr *) &clientAddress,
                              (socklen_t *) &size);

  if (clientSocket < 0) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::acceptImpl() ::accept() ", errno_copy);
    throw TTransportException(TTransportException::UNKNOWN, "accept()", errno_copy);
  }

  // Make sure client socket is blocking
  int flags = fcntl(clientSocket, F_GETFL, 0);
  if (flags == -1) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::acceptImpl() fcntl() F_GETFL ", errno_copy);
    throw TTransportException(TTransportException::UNKNOWN, "fcntl(F_GETFL)", errno_copy);
  }

  if (-1 == fcntl(clientSocket, F_SETFL, flags & ~O_NONBLOCK)) {
    int errno_copy = errno;
    GlobalOutput.perror("TServerSocket::acceptImpl() fcntl() F_SETFL ~O_NONBLOCK ", errno_copy);
    throw TTransportException(TTransportException::UNKNOWN, "fcntl(F_SETFL)", errno_copy);
  }

  shared_ptr<TSocket> client = createSocket(clientSocket);
  if (sendTimeout_ > 0) {
    client->setSendTimeout(sendTimeout_);
  }
  if (recvTimeout_ > 0) {
    client->setRecvTimeout(recvTimeout_);
  }
  client->setCachedAddress((sockaddr*) &clientAddress, size);

  return client;
}

shared_ptr<TSocket> TServerSocket::createSocket(int clientSocket) {
  return shared_ptr<TSocket>(new TSocket(clientSocket));
}

void TServerSocket::interrupt() {
  if (intSock1_ >= 0) {
    int8_t byte = 0;
    if (-1 == send(intSock1_, &byte, sizeof(int8_t), 0)) {
      GlobalOutput.perror("TServerSocket::interrupt() send() ", errno);
    }
  }
}

void TServerSocket::close() {
  if (serverSocket_ >= 0) {
    shutdown(serverSocket_, SHUT_RDWR);
    ::close(serverSocket_);
  }
  if (intSock1_ >= 0) {
    ::close(intSock1_);
  }
  if (intSock2_ >= 0) {
    ::close(intSock2_);
  }
  serverSocket_ = -1;
  intSock1_ = -1;
  intSock2_ = -1;
}

void TServerSocket::getAddress(folly::SocketAddress* address) {
  if (serverSocket_ < 0) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "attempted to get address of closed "
                              "TServerSocket");
  }

  // Call folly::SocketAddress::setFromLocalAddress().
  // We could cache the local address, but we don't expect getAddress() to
  // called often enough to make it worthwhile.  (Note that we can't cache the
  // value that we use to bind(), in case the port was 0 and the kernel picked
  // an ephemeral port for us to use.)
  address->setFromLocalAddress(serverSocket_);
}

}}} // apache::thrift::transport
