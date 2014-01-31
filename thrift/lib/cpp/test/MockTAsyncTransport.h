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

#include <gmock/gmock.h>

#include "thrift/lib/cpp/async/TAsyncTransport.h"

namespace apache { namespace thrift { namespace test {

class MockTAsyncTransport: public apache::thrift::async::TAsyncTransport {
 public:
  using TEventBase = apache::thrift::async::TEventBase;
  using ReadCallback = apache::thrift::async::TAsyncTransport::ReadCallback;
  using WriteCallback = apache::thrift::async::TAsyncTransport::WriteCallback;
  using TSocketAddress = apache::thrift::transport::TSocketAddress;

  MOCK_METHOD1(setReadCallback, void(ReadCallback*));
  MOCK_CONST_METHOD0(getReadCallback, ReadCallback*());
  MOCK_METHOD3(write, void(WriteCallback*, const void*, size_t));
  MOCK_METHOD3(writev, void(WriteCallback*, const iovec*, size_t));
  MOCK_METHOD3(writeChain,
               void(WriteCallback*, std::shared_ptr<folly::IOBuf>,
                    apache::thrift::async::WriteFlags));


  void writeChain(WriteCallback* callback,
                  std::unique_ptr<folly::IOBuf>&& iob,
                  apache::thrift::async::WriteFlags flags =
                  apache::thrift::async::WriteFlags::NONE) override {
    writeChain(callback, std::shared_ptr<folly::IOBuf>(iob.release()), flags);
  }

  MOCK_METHOD0(close, void());
  MOCK_METHOD0(closeNow, void());
  MOCK_METHOD0(closeWithReset, void());
  MOCK_METHOD0(shutdownWrite, void());
  MOCK_METHOD0(shutdownWriteNow, void());
  MOCK_CONST_METHOD0(good, bool());
  MOCK_CONST_METHOD0(readable, bool());
  MOCK_CONST_METHOD0(connecting, bool());
  MOCK_CONST_METHOD0(error, bool());
  MOCK_METHOD1(attachEventBase, void(TEventBase*));
  MOCK_METHOD0(detachEventBase, void());
  MOCK_CONST_METHOD0(isDetachable, bool());
  MOCK_CONST_METHOD0(getEventBase, TEventBase*());
  MOCK_METHOD1(setSendTimeout, void(uint32_t));
  MOCK_CONST_METHOD0(getSendTimeout, uint32_t());
  MOCK_CONST_METHOD1(getLocalAddress, void(TSocketAddress*));
  MOCK_CONST_METHOD1(getPeerAddress, void(TSocketAddress*));
  MOCK_CONST_METHOD0(getAppBytesWritten, size_t());
  MOCK_CONST_METHOD0(getRawBytesWritten, size_t());
  MOCK_CONST_METHOD0(getAppBytesReceived, size_t());
  MOCK_CONST_METHOD0(getRawBytesReceived, size_t());
  MOCK_METHOD1(setEorTracking, void(bool));

};

class MockReadCallback:
      public apache::thrift::async::TAsyncTransport::ReadCallback {
 public:
  MOCK_METHOD2(getReadBuffer, void(void**, size_t*));
  GMOCK_METHOD1_(, noexcept, , readDataAvailable, void(size_t));
  GMOCK_METHOD0_(, noexcept, , readEOF, void());
  GMOCK_METHOD1_(, noexcept, , readError,
                 void(const transport::TTransportException&));
};

class MockWriteCallback:
      public apache::thrift::async::TAsyncTransport::WriteCallback {
 public:
  GMOCK_METHOD0_(, noexcept, , writeSuccess, void());
  GMOCK_METHOD2_(, noexcept, , writeError,
                 void(size_t, const transport::TTransportException&));
};

}}}
