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

#ifndef THRIFT_ASYNC_TASYNCTRANSPORT_H_
#define THRIFT_ASYNC_TASYNCTRANSPORT_H_ 1

#include <thrift/lib/cpp/async/TDelayedDestruction.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp/thrift_config.h>
#include <sys/uio.h>
#include <inttypes.h>
#include <memory>

namespace folly {
class IOBuf;
class SocketAddress;
}

namespace apache { namespace thrift {

namespace async {

typedef folly::WriteFlags WriteFlags;

// Wrapper around folly::AsyncTransport, that converts read/write
// exceptions to TTransportExceptions
class TAsyncTransport : virtual public folly::AsyncTransport {
 public:
  typedef std::unique_ptr<TAsyncTransport, Destructor> UniquePtr;

  class ReadCallback : public folly::AsyncSocket::ReadCallback {
   public:
    virtual void readError(
      const transport::TTransportException& ex) noexcept = 0;
   private:
    virtual void readErr(const folly::AsyncSocketException& ex) noexcept {
      transport::TTransportException tex(
        transport::TTransportException::TTransportExceptionType(ex.getType()),
        ex.what(), ex.getErrno());

      readError(tex);

    }
  };

  class WriteCallback : public folly::AsyncSocket::WriteCallback {
   public:
    virtual void writeError(
      size_t bytes, const transport::TTransportException& ex) noexcept = 0;

   private:
    virtual void writeErr(
      size_t bytes, const folly::AsyncSocketException& ex) noexcept {
      transport::TTransportException tex(
        transport::TTransportException::TTransportExceptionType(ex.getType()),
        ex.what(), ex.getErrno());

      writeError(bytes, tex);

    }
  };

  // Read/write methods that aren't part of folly::AsyncTransport
  virtual void setReadCallback(TAsyncTransport::ReadCallback* callback) = 0;
  virtual TAsyncTransport::ReadCallback* getReadCallback() const = 0;

  virtual void write(
    TAsyncTransport::WriteCallback* callback, const void* buf, size_t bytes,
    WriteFlags flags = WriteFlags::NONE) = 0;
  virtual void writev(
    TAsyncTransport::WriteCallback* callback, const iovec* vec, size_t count,
    WriteFlags flags = WriteFlags::NONE) = 0;
  virtual void writeChain(
    TAsyncTransport::WriteCallback* callback,
    std::unique_ptr<folly::IOBuf>&& buf,
    WriteFlags flags = WriteFlags::NONE) = 0;
};

}}} // apache::thrift::async

#endif // #ifndef THRIFT_ASYNC_TASYNCTRANSPORT_H_
