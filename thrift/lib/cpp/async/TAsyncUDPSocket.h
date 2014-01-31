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
#pragma once

#include "folly/io/IOBuf.h"
#include "folly/ScopeGuard.h"
#include "thrift/lib/cpp/async/TEventHandler.h"
#include "thrift/lib/cpp/transport/TTransportException.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"

#include <memory>

namespace apache { namespace thrift { namespace async {

/**
 * UDP socket
 */
class TAsyncUDPSocket : public TEventHandler {
 public:
  enum class FDOwnership {
    OWNS,
    SHARED
  };

  class ReadCallback {
   public:
    /**
     * Invoked when the socket becomes readable and we want buffer
     * to write to.
     *
     * NOTE: From socket we will end up reading at most `len` bytes
     *       and if there were more bytes in datagram, we will end up
     *       dropping them.
     */
     virtual void getReadBuffer(void** buf, size_t* len) noexcept = 0;

    /**
     * Invoked when a new datagraom is available on the socket. `len`
     * is the number of bytes read and `truncated` is true if we had
     * to drop few bytes because of running out of buffer space.
     */
    virtual void onDataAvailable(const transport::TSocketAddress& client,
                                 size_t len,
                                 bool truncated) noexcept = 0;

    /**
     * Invoked when there is an error reading from the socket.
     *
     * NOTE: Since UDP is connectionless, you can still read from the socket.
     *       But you have to re-register readCallback yourself after
     *       onReadError.
     */
    virtual void onReadError(const transport::TTransportException& ex)
        noexcept = 0;

    /**
     * Invoked when socket is closed and a read callback is registered.
     */
    virtual void onReadClosed() noexcept = 0;

    virtual ~ReadCallback() {}
  };

  /**
   * Create a new UDP socket that will run in the
   * given eventbase
   */
  explicit TAsyncUDPSocket(TEventBase* evb);
  ~TAsyncUDPSocket();

  /**
   * Returns the address server is listening on
   */
  const transport::TSocketAddress& address() const {
    CHECK_NE(-1, fd_) << "Server not yet bound to an address";
    return localAddress_;
  }

  /**
   * Bind the socket to the following address. If port is not
   * set in the `address` an ephemeral port is chosen and you can
   * use `address()` method above to get it after this method successfully
   * returns.
   */
  void bind(const transport::TSocketAddress& address);

  /**
   * Use an already bound file descriptor. You can either transfer ownership
   * of this FD by using ownership = FDOwnership::OWNS or share it using
   * FDOwnership::SHARED. In case FD is shared, it will not be `close`d in
   * destructor.
   */
  void setFD(int fd, FDOwnership ownership);

  /**
   * Send the data in buffer to destination. Returns the return code from
   * ::sendto.
   */
  ssize_t write(const transport::TSocketAddress& address,
                const std::unique_ptr<folly::IOBuf>& buf);

  /**
   * Start reading datagrams
   */
  void resumeRead(ReadCallback* cob);

  /**
   * Pause reading datagrams
   */
  void pauseRead();

  /**
   * Stop listening on the socket.
   */
  void close();

  /**
   * Get internal FD used by this socket
   */
  int getFD() const {
    CHECK_NE(-1, fd_) << "Need to bind before getting FD out";
    return fd_;
  }
 private:
  TAsyncUDPSocket(const TAsyncUDPSocket&) = delete;
  TAsyncUDPSocket& operator=(const TAsyncUDPSocket&) = delete;

  // TEventHandler
  void handlerReady(uint16_t events) noexcept;

  void handleRead() noexcept;
  bool updateRegistration() noexcept;

  TEventBase* eventBase_;
  transport::TSocketAddress localAddress_;

  int fd_;
  FDOwnership ownership_;

  // Temp space to receive client address
  transport::TSocketAddress clientAddress_;

  // Non-null only when we are reading
  ReadCallback* readCallback_;
};

}}}
