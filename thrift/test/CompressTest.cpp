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

#include <gflags/gflags.h>
#include <folly/Benchmark.h>

#include <cstdlib>
#include <ctime>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <gtest/gtest.h>

#include <thrift/test/gen-cpp2/ThriftTest.h>

using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace thrift::test;

void testMessage(uint8_t flag,
                 int iters,
                 bool easyMessage,
                 bool binary = false) {
  Bonk b;
  Bonk bin;
  b.message = "";

  std::shared_ptr<TMemoryBuffer> buf(new TMemoryBuffer());
  std::shared_ptr<THeaderProtocol> prot(new THeaderProtocol(buf));
  if (flag) {
    prot->setTransform(flag);
  }
  if (binary) {
    prot->setProtocolId(T_BINARY_PROTOCOL);
  }

  std::shared_ptr<TMemoryBuffer> bufin(new TMemoryBuffer());
  std::shared_ptr<THeaderProtocol> protin(new THeaderProtocol(bufin));
  if (binary) {
    // Normally this would get set correctly by readMessageBegin,
    // but just set it manually for a unittest
    protin->setProtocolId(T_BINARY_PROTOCOL);
  }

  for (int i = 0; i < iters; i++) {
    if (easyMessage) {
      b.message += "t";
    } else {
      b.message += 66 + rand() % 24;
    }
    buf->resetBuffer();
    b.write(prot.get());
    prot->getTransport()->flush();

    uint8_t* data;
    uint32_t datasize;
    buf->getBuffer(&data, &datasize);
    bufin->resetBuffer(data, datasize);
    bin.read(protin.get());
  }

}

void testChainedCompression(uint8_t flag, int iters) {
  THeader header;
  std::map<std::string, std::string> persistentHeaders;
  if (flag) {
    header.setTransform(flag);
  }

  auto head = folly::IOBuf::create(0);

  for (int i = 0; i < iters; i++) {
    auto buf = folly::IOBuf::create(1);
    buf->append(1);
    *(buf->writableData()) = 66 + rand() % 24;
    head->prependChain(std::move(buf));
  }

  auto cloned = head->clone();

  auto compressed = header.addHeader(std::move(head), persistentHeaders);
  EXPECT_NE(compressed, nullptr);
  printf("%i\n", (int)compressed->length());

  size_t needed = 0;
  folly::IOBufQueue q;
  q.append(std::move(compressed));

  auto uncompressed = header.removeHeader(&q, needed, persistentHeaders);
  EXPECT_NE(uncompressed, nullptr);
  EXPECT_EQ(needed, 0);
  EXPECT_TRUE(q.empty());

  cloned->coalesce();
  uncompressed->coalesce();
  printf("%i, %i\n", (int)cloned->length(), (int)uncompressed->length());
  EXPECT_EQ(cloned->length(), uncompressed->length());
  EXPECT_EQ(0, memcmp(cloned->data(), uncompressed->data(), cloned->length()));
}

BENCHMARK(BM_UncompressedBinary, iters) {
  testMessage(0, iters, true, true);
}

BENCHMARK(BM_Uncompressed, iters) {
  testMessage(0, iters, true);
}

BENCHMARK(BM_Zlib, iters) {
  testMessage(0x01, iters, true);
}

BENCHMARK(BM_Snappy, iters) {
  testMessage(3, iters, true);
}

BENCHMARK(BM_Zstd, iters) {
  testMessage(5, iters, true);
}

// Test a 'hard' to compress message, more random.

BENCHMARK(BM_UncompressedBinaryHard, iters) {
  testMessage(0, iters, false, true);
}

BENCHMARK(BM_UncompressedHard, iters) {
  testMessage(0, iters, false);
}

BENCHMARK(BM_ZlibHard, iters) {
  testMessage(0x01, iters, false);
}

BENCHMARK(BM_SnappyHard, iters) {
  testMessage(3, iters, false);
}

BENCHMARK(BM_ZstdHard, iters) {
  testMessage(5, iters, false);
}

TEST(chained, none) {
  testChainedCompression(0, 1000);
}

TEST(chained, zlib) {
  testChainedCompression(1, 1000);
}

TEST(chained, snappy) {
  testChainedCompression(3, 1000);
}

TEST(chained, zstd) {
  testChainedCompression(5, 1000);
}

TEST(sdf, sdfsd) {
  Bonk b;
  Bonk bin;
  b.message = "";
  for (int i = 0; i < 10000; i++) {
    b.message += 66 + rand() % 24;
  }

  std::shared_ptr<TMemoryBuffer> bufout(new TMemoryBuffer());
  std::shared_ptr<THeaderProtocol> protout(new THeaderProtocol(bufout));
  //prot->setTransform(ZLIB_TRANSFORM);

  std::shared_ptr<TMemoryBuffer> bufin(new TMemoryBuffer());
  std::shared_ptr<THeaderProtocol> protin(new THeaderProtocol(bufin));

  bufout->resetBuffer();
  b.write(protout.get());
  protout->getTransport()->flush();

  uint32_t uncompressedSize = bufout->available_read();
  protout->setTransform(THeader::ZLIB_TRANSFORM);

  bufout->resetBuffer();
  b.write(protout.get());
  protout->getTransport()->flush();

  EXPECT_LT(bufout->available_read(), uncompressedSize);

  std::dynamic_pointer_cast<THeaderTransport>(
    protout->getTransport())->setMinCompressBytes(uncompressedSize);
  bufout->resetBuffer();
  b.write(protout.get());
  protout->getTransport()->flush();

  EXPECT_EQ(bufout->available_read(), uncompressedSize);

  // Reset Transforms
  std::vector<uint16_t> trans;
  std::dynamic_pointer_cast<THeaderTransport>(
    protout->getTransport())->setTransforms(trans);
  // Tell _receiver_ to zlib the response only if response is
  // more than 100 bytes
  protout->setTransform(THeader::ZLIB_TRANSFORM);
  std::dynamic_pointer_cast<THeaderTransport>(
    protout->getTransport())->setMinCompressBytes(100);
  std::dynamic_pointer_cast<THeaderTransport>(
    protin->getTransport())->setMinCompressBytes(100);
  bufout->resetBuffer();
  b.write(protout.get());
  protout->getTransport()->flush();

  // Uncompress
  uint8_t* data;
  uint32_t datasize;
  bufout->getBuffer(&data, &datasize);
  bufin->resetBuffer(data, datasize);
  bin.read(protin.get());

  // Recompress
  bufin->resetBuffer(uncompressedSize);
  bin.write(protin.get());
  protin->getTransport()->flush();

  EXPECT_LT(bufin->available_read(), uncompressedSize);

  // Reset Transforms
  std::dynamic_pointer_cast<THeaderTransport>(
    protout->getTransport())->setTransforms(trans);
  protout->setTransform(THeader::ZLIB_TRANSFORM);
  std::dynamic_pointer_cast<THeaderTransport>(
    protout->getTransport())->setMinCompressBytes(20000);
  std::dynamic_pointer_cast<THeaderTransport>(
    protin->getTransport())->setMinCompressBytes(20000);
  bufout->resetBuffer();
  b.write(protout.get());
  protout->getTransport()->flush();

  EXPECT_EQ(bufout->available_read(), uncompressedSize );

  // Uncompress
  bufout->getBuffer(&data, &datasize);
  bufin->resetBuffer(data, datasize);
  bin.read(protin.get());

  // Recompress
  bufin->resetBuffer(uncompressedSize);
  bin.write(protin.get());
  protin->getTransport()->flush();

  EXPECT_EQ(bufin->available_read(), uncompressedSize);
  std::string buffer = bufin->getBufferAsString();
  EXPECT_EQ(buffer[15], 0x00); // Verify there were no transforms

  bin.message = "";
  for (int i = 0; i < 20000; i++) {
    bin.message += 66 + rand() % 24;
  }

  // Recompress x2, _should_compress
  bufin->resetBuffer(uncompressedSize);
  bin.write(protin.get());
  protin->getTransport()->flush();

  EXPECT_LT(bufin->available_read(), 20000);
  buffer = bufin->getBufferAsString();
  EXPECT_EQ(buffer[15], 0x01); // Verify there was only one transform
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);

  srand(time(0));

  auto ret = RUN_ALL_TESTS();

  // Run the benchmarks
  if (!ret) {
    folly::runBenchmarksOnFlag();
  }

  return 0;
}
