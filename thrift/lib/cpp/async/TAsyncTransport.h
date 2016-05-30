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

#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/portability/SysUio.h>

#include <thrift/lib/cpp/thrift_config.h>
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
class TAsyncTransport : virtual public folly::AsyncTransportWrapper {
 public:
  typedef std::unique_ptr<TAsyncTransport, Destructor> UniquePtr;

  class ReadCallback : public folly::AsyncSocket::ReadCallback {
   public:
    virtual void readError(
      const transport::TTransportException& ex) noexcept = 0;
   private:
    void readErr(const folly::AsyncSocketException& ex) noexcept override {
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
    void writeErr(size_t bytes,
                  const folly::AsyncSocketException& ex) noexcept override {
      transport::TTransportException tex(
        transport::TTransportException::TTransportExceptionType(ex.getType()),
        ex.what(), ex.getErrno());

      writeError(bytes, tex);

    }
  };

  // Read/write methods that aren't part of folly::AsyncTransport
  virtual void setReadCallback(TAsyncTransport::ReadCallback* callback) {
    setReadCB(callback);
  }
  TAsyncTransport::ReadCallback* getReadCallback() const override = 0;
};

}}} // apache::thrift::async

#endif // #ifndef THRIFT_ASYNC_TASYNCTRANSPORT_H_
