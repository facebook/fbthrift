/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <memory>

#include <folly/io/Cursor.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleDecoder.h>

using namespace apache::thrift::detail;

// Note: Throughout this file, we pay some syntactic overheads to have
// weird-shaped IOBuf structures. This tests behavior under unfriendly pathways.
// Even though the current implementation does its I/O through friendly Cursor
// APIs now, this will likely not always be the case.

TEST(BufferingNimbleDecoder, ManyZeros) {
  // 11 bytes allocated
  auto control = folly::IOBuf::create(3);
  control->append(3);
  auto control2 = folly::IOBuf::create(7);
  control2->append(7);
  auto control3 = folly::IOBuf::create(1);
  control3->append(1);
  control->prependChain(std::move(control2));
  control->prependChain(std::move(control3));

  folly::io::RWPrivateCursor controlCursor(control.get());

  // And 11 bytes written.
  controlCursor.write<std::uint8_t>(0b00'00'00'01);
  for (int i = 0; i < 9; ++i) {
    controlCursor.write<std::uint8_t>(0);
  }
  controlCursor.write<std::uint8_t>(0b00'01'00'00);
  // Now, the chain starting at control has control bytes indicating that there
  // is a single byte chunk at the beginning and second from the end, and that
  // all other chunks are 0.

  auto data = folly::IOBuf::create(1);
  data->append(1);
  auto data2 = folly::IOBuf::create(1);
  data2->append(1);
  data->prependChain(std::move(data2));

  folly::io::RWPrivateCursor dataCursor(data.get());
  dataCursor.write<std::uint8_t>(123);
  dataCursor.write<std::uint8_t>(45);

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{control.get()});
  dec.setDataInput(folly::io::Cursor{data.get()});

  // We wrote 11 control bytes; that should map to 44=4*11 chunks.
  for (int i = 0; i < 44; ++i) {
    std::uint32_t chunk = dec.nextChunk();
    if (i == 0) {
      EXPECT_EQ(123, chunk);
    } else if (i == 42) {
      EXPECT_EQ(45, chunk);
    } else {
      EXPECT_EQ(0, chunk);
    }
  }
}

void runRefillTest(int controlGrowthAmount, int dataGrowthAmount) {
  auto controlBuf = folly::IOBuf::create(0);
  folly::io::Appender control(controlBuf.get(), controlGrowthAmount);
  auto dataBuf = folly::IOBuf::create(0);
  folly::io::Appender data(dataBuf.get(), dataGrowthAmount);
  // Write 0 through 255.
  for (int i = 0; i < 64; ++i) {
    control.write<std::uint8_t>(0b01'01'01'01);
    for (int j = 0; j < 4; ++j) {
      data.write<std::uint8_t>(i * 4 + j);
    }
  }
  // Write 256 through 65535.
  for (int i = 64; i < 16384; ++i) {
    control.write<std::uint8_t>(0b10'10'10'10);
    for (int j = 0; j < 4; ++j) {
      data.write<std::uint16_t>(i * 4 + j);
    }
  }
  // Write 65536 through 65536 + 10000;
  for (int i = 16384; i < 16384 + 2500; ++i) {
    control.write<std::uint8_t>(0b11'11'11'11);
    for (int j = 0; j < 4; ++j) {
      data.write<std::uint32_t>(i * 4 + j);
    }
  }

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{controlBuf.get()});
  dec.setDataInput(folly::io::Cursor{dataBuf.get()});

  for (int i = 0; i < 65536 + 10000; ++i) {
    std::uint32_t chunk = dec.nextChunk();
    EXPECT_EQ(i, chunk);
  }
}

TEST(BufferingNimbleDecoder, Refill) {
  runRefillTest(1, 1);
  runRefillTest(9, 13);
  runRefillTest(4, 17);
  runRefillTest(13, 3000);
  runRefillTest(11, 1000 * 1000);
  runRefillTest(100 * 1000, 1000);
}

TEST(BufferingNimbleDecoder, AlmostOutOfSpace) {
  auto controlBuf = folly::IOBuf::create(0);
  folly::io::Appender control(controlBuf.get(), 10);
  auto dataBuf = folly::IOBuf::create(0);
  folly::io::Appender data(dataBuf.get(), 10);
  // Corresponds to 8 1-byte chunks, followed by a 0-byte chunk.
  control.write<std::uint8_t>(0b01'01'01'01);
  control.write<std::uint8_t>(0b00'01'01'01);
  data.write<std::uint8_t>(1);
  data.write<std::uint8_t>(2);
  data.write<std::uint8_t>(3);
  data.write<std::uint8_t>(4);
  data.write<std::uint8_t>(5);
  data.write<std::uint8_t>(6);
  data.write<std::uint8_t>(7);

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{controlBuf.get()});
  dec.setDataInput(folly::io::Cursor{dataBuf.get()});

  EXPECT_EQ(1, dec.nextChunk());
  EXPECT_EQ(2, dec.nextChunk());
  EXPECT_EQ(3, dec.nextChunk());
  EXPECT_EQ(4, dec.nextChunk());
  EXPECT_EQ(5, dec.nextChunk());
  EXPECT_EQ(6, dec.nextChunk());
  EXPECT_EQ(7, dec.nextChunk());
  EXPECT_EQ(0, dec.nextChunk());
}

TEST(BufferingNimbleDecoder, OutOfControlSpace) {
  auto controlBuf = folly::IOBuf::create(0);
  auto dataBuf = folly::IOBuf::create(0);
  folly::io::Appender data(dataBuf.get(), 10);

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{controlBuf.get()});
  dec.setDataInput(folly::io::Cursor{dataBuf.get()});
  EXPECT_THROW({ (void)dec.nextChunk(); }, std::exception);
}

TEST(BufferingNimbleDecoder, OutOfDataSpace) {
  auto controlBuf = folly::IOBuf::create(0);
  folly::io::Appender control(controlBuf.get(), 10);
  auto dataBuf = folly::IOBuf::create(0);
  folly::io::Appender data(dataBuf.get(), 10);
  // Corresponds to 8 1-byte chunks.
  control.write<std::uint8_t>(0b01'01'01'01);
  control.write<std::uint8_t>(0b01'01'01'01);
  // But we only put 7 in the stream.
  data.write<std::uint8_t>(1);
  data.write<std::uint8_t>(2);
  data.write<std::uint8_t>(3);
  data.write<std::uint8_t>(4);
  data.write<std::uint8_t>(5);
  data.write<std::uint8_t>(6);
  data.write<std::uint8_t>(7);

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{controlBuf.get()});
  dec.setDataInput(folly::io::Cursor{dataBuf.get()});

  EXPECT_THROW(
      {
        for (int i = 0; i < 8; ++i) {
          (void)dec.nextChunk();
        }
      },
      std::exception);
}
