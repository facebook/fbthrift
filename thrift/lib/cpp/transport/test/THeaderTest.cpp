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

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/util/THttpParser.h>

#include <memory>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <gtest/gtest.h>

using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::transport;

TEST(THeaderTest, largetransform) {
  THeader header;
  header.setTransform(THeader::ZLIB_TRANSFORM); // ZLib flag

  size_t buf_size = 1000000;
  std::unique_ptr<folly::IOBuf> buf = IOBuf::create(buf_size);
  buf->append(buf_size);
  auto body = transport::THeaderBody::wrapUntransformed(std::move(buf));

  std::map<std::string, std::string> persistentHeaders;
  buf = header.addHeader(std::move(body), persistentHeaders);
  buf_size = buf->computeChainDataLength();
  std::unique_ptr<IOBufQueue> queue(new IOBufQueue);
  std::unique_ptr<IOBufQueue> queue2(new IOBufQueue);
  queue->append(std::move(buf));
  queue2->append(queue->split(buf_size/4));
  queue2->append(queue->split(buf_size/4));
  queue2->append(IOBuf::create(0)); // Empty buffer should work
  queue2->append(queue->split(buf_size/4));
  queue2->append(queue->move());

  size_t needed;

  body = header.removeHeader(queue2.get(), needed, persistentHeaders);
}

TEST(THeaderTest, minCompressBytes) {
  THeader header;
  header.setTransform(THeader::ZLIB_TRANSFORM); // ZLib flag
  header.setMinCompressBytes(0);

  char srcbuf[100];
  for (int i = 0; i < sizeof(srcbuf); i++) {
    srcbuf[i] = (char) (i % 8);
  }

  size_t buf_size = 100;
  std::unique_ptr<folly::IOBuf> buf;
  std::vector<uint16_t> transforms;

  buf = IOBuf::copyBuffer(srcbuf, sizeof(srcbuf));
  auto compressedBody = header.transform(std::move(buf));
  auto compressedBuf = compressedBody->getTransformed(transforms);

  EXPECT_EQ(1, transforms.size());
  EXPECT_LT(0, compressedBuf->computeChainDataLength());
  EXPECT_GT(buf_size, compressedBuf->computeChainDataLength());

  header.setMinCompressBytes(1000);

  buf = IOBuf::copyBuffer(srcbuf, sizeof(srcbuf));
  auto uncompressedBody = header.transform(std::move(buf));
  auto uncompressedBuf = uncompressedBody->getTransformed(transforms);

  EXPECT_EQ(0, transforms.size());
  EXPECT_LT(0, uncompressedBuf->computeChainDataLength());
  EXPECT_EQ(buf_size, uncompressedBuf->computeChainDataLength());
}

TEST(THeaderTest, http_clear_header) {
  THeader header;
  header.setClientType(THRIFT_HTTP_CLIENT_TYPE);
  auto parser = std::make_shared<apache::thrift::util::THttpClientParser>(
      "testhost", "testuri");
  header.setHttpClientParser(parser);
  header.setHeader("WriteHeader", "foo");

  size_t buf_size = 1000000;
  std::unique_ptr<IOBuf> buf = IOBuf::create(buf_size);
  auto body = transport::THeaderBody::wrapUntransformed(std::move(buf));
  std::map<std::string, std::string> persistentHeaders;
  buf = header.addHeader(std::move(body), persistentHeaders);

  EXPECT_TRUE(header.isWriteHeadersEmpty());
}
