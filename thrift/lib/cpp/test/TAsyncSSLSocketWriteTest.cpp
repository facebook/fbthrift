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

#include "common/logging/logging.h"
#include "folly/Foreach.h"
#include "folly/io/Cursor.h"
#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/Util.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

using apache::thrift::async::TEventBase;
using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::SSLContext;
using std::string;
using namespace testing;

namespace apache { namespace thrift { namespace async {

class MockTAsyncSSLSocket : public TAsyncSSLSocket{
 public:
  static std::shared_ptr<MockTAsyncSSLSocket> newSocket(
    const std::shared_ptr<transport::SSLContext>& ctx,
    TEventBase* evb) {
    auto sock = std::shared_ptr<MockTAsyncSSLSocket>(
      new MockTAsyncSSLSocket(ctx, evb),
      Destructor());
    sock->ssl_ = SSL_new(ctx->getSSLCtx());
    SSL_set_fd(sock->ssl_, -1);
    return sock;
  }

  // Fake constructor sets the state to established without call to connect
  // or accept
  MockTAsyncSSLSocket(const std::shared_ptr<transport::SSLContext>& ctx,
                      TEventBase* evb)
      : TAsyncSSLSocket(ctx, evb) {
    state_ = TAsyncSocket::StateEnum::ESTABLISHED;
    sslState_ = TAsyncSSLSocket::SSLStateEnum::STATE_ESTABLISHED;
  }

  // mock the calls to SSL_write to see the buffer length and contents
  MOCK_METHOD3(sslWriteImpl, int(SSL *ssl, const void *buf, int n));

  // mock the calls to getRawBytesWritten()
  MOCK_CONST_METHOD0(getRawBytesWritten, size_t());

  // public wrapper for protected interface
  ssize_t testPerformWrite(const iovec* vec, uint32_t count, WriteFlags flags,
                           uint32_t* countWritten, uint32_t* partialWritten) {
    return performWrite(vec, count, flags, countWritten, partialWritten);
  }

  void checkEor(size_t appEor, size_t rawEor) {
    EXPECT_EQ(appEor, appEorByteNo_);
    EXPECT_EQ(rawEor, minEorRawByteNo_);
  }

  void setAppBytesWritten(size_t n) {
    appBytesWritten_ = n;
  }
};

class TAsyncSSLSocketWriteTest : public testing::Test {
 public:
  TAsyncSSLSocketWriteTest() :
      sslContext_(new SSLContext()),
      sock_(MockTAsyncSSLSocket::newSocket(sslContext_, &eventBase_)) {
    for (int i = 0; i < 500; i++) {
      memcpy(source_ + i * 26, "abcdefghijklmnopqrstuvwxyz", 26);
    }
  }

  // Make an iovec containing chunks of the reference text with requested sizes
  // for each chunk
  iovec *makeVec(std::vector<uint32_t> sizes) {
    iovec *vec = new iovec[sizes.size()];
    int i = 0;
    int pos = 0;
    for (auto size: sizes) {
      vec[i].iov_base = (void *)(source_ + pos);
      vec[i++].iov_len = size;
      pos += size;
    }
    return vec;
  }

  // Verify that the given buf/pos matches the reference text
  void verifyVec(const void *buf, int n, int pos) {
    ASSERT_EQ(memcmp(source_ + pos, buf, n), 0);
  }

  // Update a vec on partial write
  void consumeVec(iovec *vec, uint32_t countWritten, uint32_t partialWritten) {
    vec[countWritten].iov_base =
      ((char *)vec[countWritten].iov_base) + partialWritten;
    vec[countWritten].iov_len -= partialWritten;
  }

  TEventBase eventBase_;
  std::shared_ptr<SSLContext> sslContext_;
  std::shared_ptr<MockTAsyncSSLSocket> sock_;
  char source_[26 * 500];
};


// The entire vec fits in one packet
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing1) {
  int n = 3;
  iovec *vec = makeVec({3, 3, 3});
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 9))
    .WillOnce(Invoke([this] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, 0);
          return 9; }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
}

// First packet is full, second two go in one packet
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing2) {
  int n = 3;
  iovec *vec = makeVec({1500, 3, 3});
  int pos = 0;
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 6))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
}

// Two exactly full packets (coalesce ends midway through second chunk)
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing3) {
  int n = 3;
  iovec *vec = makeVec({1000, 1000, 1000});
  int pos = 0;
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .Times(2)
    .WillRepeatedly(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
}

// Partial write success midway through a coalesced vec
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing4) {
  int n = 5;
  iovec *vec = makeVec({300, 300, 300, 300, 300});
  int pos = 0;
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += 1000;
          return 1000; /* 500 bytes "pending" */ }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, 3);
  EXPECT_EQ(partialWritten, 100);
  consumeVec(vec, countWritten, partialWritten);
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return 500; }));
  sock_->testPerformWrite(vec + countWritten, n - countWritten,
                          WriteFlags::NONE,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, 2);
  EXPECT_EQ(partialWritten, 0);
}

// coalesce ends exactly on a buffer boundary
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing5) {
  int n = 3;
  iovec *vec = makeVec({1000, 500, 500});
  int pos = 0;
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, 3);
  EXPECT_EQ(partialWritten, 0);
}

// partial write midway through first chunk
TEST_F(TAsyncSSLSocketWriteTest, write_coalescing6) {
  int n = 2;
  iovec *vec = makeVec({1000, 500});
  int pos = 0;
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += 700;
          return 700; }));
  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::NONE, &countWritten,
                          &partialWritten);
  EXPECT_EQ(countWritten, 0);
  EXPECT_EQ(partialWritten, 700);
  consumeVec(vec, countWritten, partialWritten);
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 800))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  sock_->testPerformWrite(vec + countWritten, n - countWritten,
                          WriteFlags::NONE,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, 2);
  EXPECT_EQ(partialWritten, 0);
}

// Repeat coalescing2 with WriteFlags::EOR
TEST_F(TAsyncSSLSocketWriteTest, write_with_eor1) {
  int n = 3;
  iovec *vec = makeVec({1500, 3, 3});
  int pos = 0;
  const size_t initAppBytesWritten = 500;
  const size_t appEor = initAppBytesWritten + 1506;

  sock_->setAppBytesWritten(initAppBytesWritten);
  sock_->setEorTracking(true);

  EXPECT_CALL(*(sock_.get()), getRawBytesWritten())
    // rawBytesWritten after writting initAppBytesWritten + 1500
    // + some random SSL overhead
    .WillOnce(Return(3600))
    // rawBytesWritten after writting last 6 bytes
    // + some random SSL overhead
    .WillOnce(Return(3728));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([=, &pos] (SSL *, const void *buf, int n) {
          // the first 1500 does not have the EOR byte
          sock_->checkEor(0, 0);
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 6))
    .WillOnce(Invoke([=, &pos] (SSL *, const void *buf, int n) {
          sock_->checkEor(appEor, 3600 + n);
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));

  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n , WriteFlags::EOR,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
  sock_->checkEor(0, 0);
}

// coalescing with left over at the last chunk
// WriteFlags::EOR turned on
TEST_F(TAsyncSSLSocketWriteTest, write_with_eor2) {
  int n = 3;
  iovec *vec = makeVec({600, 600, 600});
  int pos = 0;
  const size_t initAppBytesWritten = 500;
  const size_t appEor = initAppBytesWritten + 1800;

  sock_->setAppBytesWritten(initAppBytesWritten);
  sock_->setEorTracking(true);

  EXPECT_CALL(*(sock_.get()), getRawBytesWritten())
    // rawBytesWritten after writting initAppBytesWritten +  1500 bytes
    // + some random SSL overhead
    .WillOnce(Return(3600))
    // rawBytesWritten after writting last 300 bytes
    // + some random SSL overhead
    .WillOnce(Return(4100));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1500))
    .WillOnce(Invoke([=, &pos] (SSL *, const void *buf, int n) {
          // the first 1500 does not have the EOR byte
          sock_->checkEor(0, 0);
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 300))
    .WillOnce(Invoke([=, &pos] (SSL *, const void *buf, int n) {
          sock_->checkEor(appEor, 3600 + n);
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));

  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::EOR,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
  sock_->checkEor(0, 0);
}

// WriteFlags::EOR set
// One buf in iovec
// Partial write at 1000-th byte
TEST_F(TAsyncSSLSocketWriteTest, write_with_eor3) {
  int n = 1;
  iovec *vec = makeVec({1600});
  int pos = 0;
  const size_t initAppBytesWritten = 500;
  const size_t appEor = initAppBytesWritten + 1600;

  sock_->setAppBytesWritten(initAppBytesWritten);
  sock_->setEorTracking(true);

  EXPECT_CALL(*(sock_.get()), getRawBytesWritten())
    // rawBytesWritten after the initAppBytesWritten
    // + some random SSL overhead
    .WillOnce(Return(2000))
    // rawBytesWritten after the initAppBytesWritten + 1000 (with 100 overhead)
    // + some random SSL overhead
    .WillOnce(Return(3100));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 1600))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          sock_->checkEor(appEor, 2000 + n);
          verifyVec(buf, n, pos);
          pos += 1000;
          return 1000; }));

  uint32_t countWritten = 0;
  uint32_t partialWritten = 0;
  sock_->testPerformWrite(vec, n, WriteFlags::EOR,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, 0);
  EXPECT_EQ(partialWritten, 1000);
  sock_->checkEor(appEor, 2000 + 1600);
  consumeVec(vec, countWritten, partialWritten);

  EXPECT_CALL(*(sock_.get()), getRawBytesWritten())
    .WillOnce(Return(3100))
    .WillOnce(Return(3800));
  EXPECT_CALL(*(sock_.get()), sslWriteImpl(_, _, 600))
    .WillOnce(Invoke([this, &pos] (SSL *, const void *buf, int n) {
          sock_->checkEor(appEor, 3100 + n);
          verifyVec(buf, n, pos);
          pos += n;
          return n; }));
  sock_->testPerformWrite(vec + countWritten, n - countWritten,
                          WriteFlags::EOR,
                          &countWritten, &partialWritten);
  EXPECT_EQ(countWritten, n);
  EXPECT_EQ(partialWritten, 0);
  sock_->checkEor(0, 0);
}

}}}
